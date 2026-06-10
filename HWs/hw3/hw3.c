/**
 * garage.c
 * 
 * Multi-threaded vehicle service center simulation.
 * 
 * Architecture:
 *   - K_CARS vehicles arrive every CARS_TIMING microseconds (car_creator_thread)
 *   - Each vehicle is a thread that enters a priority queue and waits
 *   - manager_thread pops the highest-priority vehicle and admits it (up to N_CAPACITY)
 *   - monitor_thread promotes vehicles that waited too long to VIP (aging)
 * 
 * Station flow per vehicle (car_thread):
 *   1. Enter priority queue -> wait for admission
 *   2. Mechanic station: acquire mechanic + diagnostic tablet + torque wrench -> work -> release tools
 *   3. Wait for slot in pollution queue -> release mechanic
 *   4. Pollution station: acquire tester -> work -> release tester
 *   5. Payment: acquire payment box -> pay -> release
 *   6. Exit: release capacity slot, signal completion
 * 
 * Synchronization:
 *   - slots_sem:           limits vehicles inside the institute (N_CAPACITY)
 *   - queue_sem:           counts vehicles waiting in priority queue
 *   - done_sem:            counts vehicles that completed treatment
 *   - mechanic_sem:        limits concurrent mechanics (MECHANIC)
 *   - diagnostic_sem:      limits diagnostic tablets (DIAGNOSTIC_TABLETS)
 *   - torque_sem:          limits torque wrenches (TORQUE_WRENCH)
 *   - pollution_queue_sem: limits transfer queue to pollution station (POLLUTION_QUEUE)
 *   - pollution_worker_sem:limits concurrent testers (POLLUTION_WORKERS)
 *   - payment_box:         single payment booth (1)
 *   - pq.mutex:            protects priority queue structure
 *   - mechanic_mutex:      protects mechanic_available[] array
 *   - tester_mutex:        protects tester_available[] array
 *   - print_mutex:         serializes stdout output across threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>

/* ── Configuration ──────────────────────────────────────────────────────── */
#define K_CARS                20       /* total vehicles to simulate         */
#define N_CAPACITY            10       /* max vehicles inside institute      */
#define CARS_TIMING           50000    /* microseconds between car arrivals  */
#define VIP_RATIO             4        /* 1-in-VIP_RATIO chance of VIP       */
#define MECHANIC              3        /* number of mechanics                */
#define DIAGNOSTIC_TABLETS    2        /* number of diagnostic tablets       */
#define TORQUE_WRENCH         2        /* number of torque wrenches          */
#define TESTER                2        /* number of pollution testers        */
#define POLLUTION_QUEUE       2        /* transfer queue size to station 2   */
#define POLLUTION_WORKERS     2        /* concurrent testers allowed         */
#define MONITOR_INTERVAL      100000   /* microseconds between aging checks  */
#define WAIT_THRESHOLD        0.4      /* seconds before regular -> VIP      */
#define RAND_100K             100000   /* mechanic work time lower bound     */
#define RAND_200K             100000   /* mechanic work time range           */
#define POLLUTION_WORKING_TIME 150000  /* pollution test duration (us)       */
#define PAYMENT_BOX           1        /* single payment booth               */

/* ── Semaphores ─────────────────────────────────────────────────────────── */
sem_t slots_sem, queue_sem, done_sem;
sem_t mechanic_sem, diagnostic_sem, torque_sem;
sem_t pollution_queue_sem, pollution_worker_sem;
sem_t payment_box;

/* ── Forward declarations ───────────────────────────────────────────────── */
typedef struct car_entry car_entry;
typedef struct priority_queue priority_queue;

/* ── Data structures ────────────────────────────────────────────────────── */

/**
 * car_entry - represents a single vehicle and its thread state.
 * @car_id:     unique vehicle identifier (0-based)
 * @priority:   0 = regular, 1 = VIP (higher = served first)
 * @enter_time: wall-clock time vehicle entered the queue (for aging)
 * @sem:        personal semaphore; manager posts it to admit this vehicle
 * @pq:         pointer to the shared priority queue (for pq_insert)
 */
typedef struct car_entry {
    int car_id;
    int priority;
    time_t enter_time;
    sem_t sem;
    priority_queue* pq;
} car_entry;

/**
 * priority_queue - sorted array of car_entry pointers.
 * Sorted descending by priority, then ascending by enter_time (FIFO within tier).
 * @size:  current number of entries
 * @mutex: protects all reads/writes to this structure
 * @cars:  array of pointers (pointers are swapped during sort, not structs)
 */
typedef struct priority_queue {
    int size;
    pthread_mutex_t mutex;
    car_entry* cars[K_CARS];
} priority_queue;

/**
 * creator_args - passed to car_creator_thread.
 * Bundles all shared state the creator needs to spawn vehicle threads.
 */
typedef struct {
    priority_queue* pq;
    car_entry* cars;
    pthread_t* threads;
} creator_args;

/* ── Worker ID tracking ─────────────────────────────────────────────────── */

/**
 * mechanic_available[] / tester_available[]
 * Each slot represents one worker (1 = free, 0 = busy).
 * Protected by their respective mutexes.
 * Invariant: exactly as many slots are 0 as there are active sem holders.
 */
int mechanic_available[MECHANIC] = {1, 1, 1};
pthread_mutex_t mechanic_mutex = PTHREAD_MUTEX_INITIALIZER;

int tester_available[TESTER] = {1, 1};
pthread_mutex_t tester_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Serializes all printf output to prevent interleaved lines */
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ── Function declarations ──────────────────────────────────────────────── */
void* car_creator_thread(void*);
void* monitor_thread(void*);
void* car_thread(void*);
void* manager_thread(void*);
void  insertion_sort(priority_queue*);
void  pq_insert(priority_queue*, car_entry*);
car_entry* pq_pop(priority_queue*);
void  print_log(int, const char*);
int   is_vip(void);

/* ── main ───────────────────────────────────────────────────────────────── */
int main(void) {
    pthread_t threads[K_CARS], creator;
    car_entry cars[K_CARS];
    priority_queue pq = { .size = 0 };

    /* Initialize synchronization primitives */
    pthread_mutex_init(&pq.mutex, NULL);
    sem_init(&slots_sem,            0, N_CAPACITY);
    sem_init(&queue_sem,            0, 0);
    sem_init(&done_sem,             0, 0);
    sem_init(&mechanic_sem,         0, MECHANIC);
    sem_init(&diagnostic_sem,       0, DIAGNOSTIC_TABLETS);
    sem_init(&torque_sem,           0, TORQUE_WRENCH);
    sem_init(&pollution_queue_sem,  0, POLLUTION_QUEUE);
    sem_init(&pollution_worker_sem, 0, POLLUTION_WORKERS);
    sem_init(&payment_box,          0, PAYMENT_BOX);

    printf("--- Institute Open (N=%d, K=%d) ---\n", N_CAPACITY, K_CARS);

    creator_args args = { &pq, cars, threads };

    /* Spawn vehicle creator (produces one car every CARS_TIMING us) */
    if (pthread_create(&creator, NULL, car_creator_thread, &args) != 0) {
        fprintf(stderr, "Fatal: failed to create car_creator_thread\n");
        return 1;
    }

    /* Spawn aging monitor (promotes long-waiting regulars to VIP) */
    pthread_t monitor;
    if (pthread_create(&monitor, NULL, monitor_thread, &pq) != 0) {
        fprintf(stderr, "Fatal: failed to create monitor_thread\n");
        return 1;
    }

    /* Spawn admission manager (dequeues and admits vehicles) */
    pthread_t manager;
    if (pthread_create(&manager, NULL, manager_thread, &pq) != 0) {
        fprintf(stderr, "Fatal: failed to create manager_thread\n");
        return 1;
    }

    /* Wait until all K_CARS vehicles have completed treatment */
    for (int i = 0; i < K_CARS; i++)
        sem_wait(&done_sem);

    printf("--- Institute Closed ---\n");
    return 0;
}

/* ── car_creator_thread ─────────────────────────────────────────────────── */
/**
 * Spawns K_CARS vehicle threads, one every CARS_TIMING microseconds.
 * Initializes each car_entry before creating its thread.
 */
void* car_creator_thread(void* param) {
    creator_args* args = (creator_args*)param;
    for (int i = 0; i < K_CARS; i++) {
        args->cars[i].car_id     = i + 1;
        args->cars[i].priority   = is_vip();
        args->cars[i].enter_time = time(NULL);
        args->cars[i].pq         = args->pq;
        sem_init(&args->cars[i].sem, 0, 0);

        if (pthread_create(&args->threads[i], NULL, car_thread, &args->cars[i]) != 0) {
            fprintf(stderr, "Error: failed to create thread for car %d\n", i + 1);
            /* Signal done so main doesn't hang */
            sem_post(&done_sem);
        }
        usleep(CARS_TIMING);
    }
    return NULL;
}

/* ── manager_thread ─────────────────────────────────────────────────────── */
/**
 * Admission loop: waits for a vehicle in the queue AND a free capacity slot,
 * then wakes the highest-priority vehicle via its personal semaphore.
 *
 * Double sem_wait pattern:
 *   queue_sem  - ensures there is at least one vehicle to dequeue
 *   slots_sem  - ensures the institute has capacity to admit it
 * Both must be held before pq_pop to avoid admitting more than N_CAPACITY.
 */
void* manager_thread(void* param) {
    priority_queue* pq = (priority_queue*)param;
    while (1) {
        sem_wait(&queue_sem);
        sem_wait(&slots_sem);
        car_entry* car = pq_pop(pq);
        if (car)
            sem_post(&car->sem);
        else
            sem_post(&slots_sem); /* return slot if pop failed (shouldn't happen) */
    }
    return NULL;
}

/* ── monitor_thread ─────────────────────────────────────────────────────── */
/**
 * Aging thread: wakes every MONITOR_INTERVAL microseconds and scans the queue.
 * Any regular vehicle (priority == 0) that has waited >= WAIT_THRESHOLD seconds
 * is promoted to VIP (priority = 1). The queue is then re-sorted.
 *
 * Holds pq->mutex for the entire scan+sort to prevent races with pq_insert/pq_pop.
 */
void* monitor_thread(void* param) {
    priority_queue* pq = (priority_queue*)param;
    while (1) {
        usleep(MONITOR_INTERVAL);
        pthread_mutex_lock(&pq->mutex);
        time_t now = time(NULL);
        for (int i = 0; i < pq->size; i++) {
            if (pq->cars[i]->priority == 0 &&
                difftime(now, pq->cars[i]->enter_time) >= WAIT_THRESHOLD) {
                pq->cars[i]->priority = 1;
                pthread_mutex_lock(&print_mutex);
                print_log(pq->cars[i]->car_id, "System");
                printf("Aging Alert! Waited %.0fms -> Promoted To VIP\n",
                    difftime(time(NULL), pq->cars[i]->enter_time) * 1000);
                pthread_mutex_unlock(&print_mutex);
            }
        }
        insertion_sort(pq);
        pthread_mutex_unlock(&pq->mutex);
    }
    return NULL;
}

/* ── insertion_sort ─────────────────────────────────────────────────────── */
/**
 * Sorts pq->cars[] in-place.
 * Order: descending priority, then ascending enter_time (older = higher priority).
 * Called only while pq->mutex is held by the caller.
 */
void insertion_sort(priority_queue* pq) {
    for (int i = 1; i < pq->size; i++) {
        car_entry* key = pq->cars[i];
        int j = i - 1;
        while (j >= 0 && (key->priority > pq->cars[j]->priority ||
               (key->priority == pq->cars[j]->priority &&
                key->enter_time < pq->cars[j]->enter_time))) {
            pq->cars[j + 1] = pq->cars[j];
            j--;
        }
        pq->cars[j + 1] = key;
    }
}

/* ── car_thread ─────────────────────────────────────────────────────────── */
/**
 * Main vehicle lifecycle. Each car runs this function.
 *
 * On error (missing worker slot), the thread releases all held resources
 * and signals done_sem so main() does not hang indefinitely.
 */
void* car_thread(void* param) {
    car_entry* car = (car_entry*)param;

    /* Use thread-local seed for rand_r (thread-safe alternative to rand) */
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self();

    /* ── Enter priority queue and wait for admission ── */
    pq_insert(car->pq, car);
    sem_wait(&car->sem);

    pthread_mutex_lock(&print_mutex);
    print_log(car->car_id, "Main");
    printf("Arrived (%s)\n", car->priority == 1 ? "VIP" : "Regular");
    pthread_mutex_unlock(&print_mutex);

    /* ── Station 1: Mechanics ── */

    /*
     * Acquire mechanic slot first (limits to MECHANIC concurrent),
     * then find which mechanic ID is free in the tracking array.
     * The sem guarantees a free slot exists when we scan the array.
     */
    sem_wait(&mechanic_sem);
    int my_mechanic = -1;
    pthread_mutex_lock(&mechanic_mutex);
    for (int i = 0; i < MECHANIC; i++) {
        if (mechanic_available[i]) {
            mechanic_available[i] = 0;
            my_mechanic = i + 1;
            break;
        }
    }
    pthread_mutex_unlock(&mechanic_mutex);

    /* Safety check: sem guarantees a slot, but guard anyway */
    if (my_mechanic == -1) {
        fprintf(stderr, "Error: car %d found no mechanic despite sem\n", car->car_id);
        sem_post(&mechanic_sem);
        sem_post(&slots_sem);
        sem_post(&done_sem);
        return NULL;
    }

    /* Acquire both tools before starting work */
    sem_wait(&diagnostic_sem);
    sem_wait(&torque_sem);

    pthread_mutex_lock(&print_mutex);
    print_log(car->car_id, "Mech");
    printf("Mech-%d: Working (Tools Acquired)\n", my_mechanic);
    pthread_mutex_unlock(&print_mutex);

    usleep(rand_r(&seed) % RAND_200K + RAND_100K);

    /* Release tools (mechanic stays busy until pollution queue has room) */
    sem_post(&torque_sem);
    sem_post(&diagnostic_sem);

    pthread_mutex_lock(&print_mutex);
    print_log(car->car_id, "Mech");
    printf("Mech-%d: Finished & Returned Tools\n", my_mechanic);
    pthread_mutex_unlock(&print_mutex);

    /*
     * Wait for a slot in the pollution transfer queue before releasing mechanic.
     * Mechanic is freed only after securing the slot, so he stays at the bay
     * but tools are already returned (available for other mechanics).
     */
    sem_wait(&pollution_queue_sem);
    mechanic_available[my_mechanic - 1] = 1;
    sem_post(&mechanic_sem);

    pthread_mutex_lock(&print_mutex);
    print_log(car->car_id, "Mech");
    printf("Mech-%d: Moved Car To Pollution Queue\n", my_mechanic);
    pthread_mutex_unlock(&print_mutex);

    /* ── Station 2: Air Pollution ── */

    /*
     * Acquire tester slot, then find which tester ID is free.
     * Release pollution_queue_sem once tester is assigned (slot is now taken).
     */
    sem_wait(&pollution_worker_sem);
    sem_post(&pollution_queue_sem);

    int my_tester = -1;
    pthread_mutex_lock(&tester_mutex);
    for (int i = 0; i < TESTER; i++) {
        if (tester_available[i]) {
            tester_available[i] = 0;
            my_tester = i + 1;
            break;
        }
    }
    pthread_mutex_unlock(&tester_mutex);

    if (my_tester == -1) {
        fprintf(stderr, "Error: car %d found no tester despite sem\n", car->car_id);
        sem_post(&pollution_worker_sem);
        sem_post(&slots_sem);
        sem_post(&done_sem);
        return NULL;
    }

    pthread_mutex_lock(&print_mutex);
    print_log(car->car_id, "Tester");
    printf("Tester-%d: Starting Pollution Test\n", my_tester);
    pthread_mutex_unlock(&print_mutex);

    usleep(POLLUTION_WORKING_TIME);

    tester_available[my_tester - 1] = 1;
    sem_post(&pollution_worker_sem);

    /* ── Station 3: Payment ── */
    sem_wait(&payment_box);

    pthread_mutex_lock(&print_mutex);
    print_log(car->car_id, "Tester");
    printf("Tester-%d: Done. Left Institute.\n", my_tester);
    pthread_mutex_unlock(&print_mutex);

    sem_post(&payment_box);

    /* ── Exit ── */
    sem_post(&slots_sem);  /* free capacity slot for next vehicle */
    sem_post(&done_sem);   /* signal main that one more car is done */
    return NULL;
}

/* ── is_vip ─────────────────────────────────────────────────────────────── */
/* Returns 1 (VIP) with probability 1/VIP_RATIO, else 0 (regular). */
int is_vip(void) { return rand() % VIP_RATIO == 0; }

/* ── print_log ──────────────────────────────────────────────────────────── */
/**
 * Prints a timestamped prefix: [HH:MM:SS.mmm] [ ROLE #ID]
 * Caller must hold print_mutex before calling.
 */
void print_log(int car_id, const char* role) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* tm_info = localtime(&tv.tv_sec);
    printf("[%02d:%02d:%02d.%03ld] [ %s #%02d] ",
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec,
        tv.tv_usec / 1000,
        role, car_id);
}

/* ── pq_insert ──────────────────────────────────────────────────────────── */
/**
 * Inserts a car into the priority queue in sorted position.
 * Higher priority comes first; ties broken by earlier enter_time (FIFO).
 * Posts queue_sem to notify manager_thread that a vehicle is waiting.
 */
void pq_insert(priority_queue* pq, car_entry* car) {
    pthread_mutex_lock(&pq->mutex);
    if (pq->size >= K_CARS) {
        pthread_mutex_unlock(&pq->mutex);
        fprintf(stderr, "Error: priority queue overflow for car %d\n", car->car_id);
        return;
    }
    int i = pq->size - 1;
    while (i >= 0 && (car->priority > pq->cars[i]->priority ||
           (car->priority == pq->cars[i]->priority &&
            car->enter_time < pq->cars[i]->enter_time))) {
        pq->cars[i + 1] = pq->cars[i];
        i--;
    }
    pq->cars[i + 1] = car;
    pq->size++;
    sem_post(&queue_sem);
    pthread_mutex_unlock(&pq->mutex);
}

/* ── pq_pop ─────────────────────────────────────────────────────────────── */
/**
 * Removes and returns the highest-priority vehicle (index 0).
 * Returns NULL if queue is empty (should not happen if queue_sem is correct).
 */
car_entry* pq_pop(priority_queue* pq) {
    pthread_mutex_lock(&pq->mutex);
    if (pq->size == 0) {
        pthread_mutex_unlock(&pq->mutex);
        return NULL;
    }
    car_entry* car = pq->cars[0];
    for (int i = 0; i < pq->size - 1; i++)
        pq->cars[i] = pq->cars[i + 1];
    pq->size--;
    pthread_mutex_unlock(&pq->mutex);
    return car;
}