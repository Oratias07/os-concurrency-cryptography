/**
 * threads.c
 *
 * All thread functions:
 *   - car_creator_thread: spawns K_CARS vehicle threads
 *   - manager_thread:     admits vehicles from priority queue
 *   - monitor_thread:     promotes long-waiting vehicles to VIP (aging)
 *   - car_thread:         full vehicle lifecycle through all stations
 */

#include "garage.h"

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

/* ── car_creator_thread ─────────────────────────────────────────────────── */
/**
 * Spawns K_CARS vehicle threads, one every CARS_TIMING microseconds.
 * Initializes each car_entry before creating its thread.
 * On thread creation failure, posts done_sem so main() does not hang.
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
            sem_post(&done_sem);
        }
        usleep(CARS_TIMING);
    }
    printf("--- All cars generated. Waiting for completion... ---\n");
    return NULL;
}

/* ── manager_thread ─────────────────────────────────────────────────────── */
/**
 * Admission loop: waits for a vehicle in the queue AND a free capacity slot,
 * then wakes the highest-priority vehicle via its personal semaphore.
 *
 * Double sem_wait pattern:
 *   queue_sem - ensures there is at least one vehicle to dequeue
 *   slots_sem - ensures the institute has capacity to admit it
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

/* ── car_thread ─────────────────────────────────────────────────────────── */
/**
 * Main vehicle lifecycle. Each car runs this function.
 *
 * Station flow:
 *   1. Enter priority queue -> wait for admission (personal sem)
 *   2. Mechanic station: acquire mechanic + tools -> work -> release tools
 *      -> wait for pollution queue slot -> release mechanic
 *   3. Pollution station: acquire tester -> work -> release tester
 *   4. Payment: acquire payment box -> pay -> release
 *   5. Exit: release capacity slot, signal completion
 *
 * On error (missing worker slot), releases all held resources
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

    /* Release tools; mechanic stays busy until pollution queue has room */
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