/**
 * garage.h
 *
 * Shared header for the vehicle service center simulation.
 * Included by all .c files.
 */

#ifndef GARAGE_H
#define GARAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>

/* ── Configuration ──────────────────────────────────────────────────────── */
#define K_CARS                 20       /* total vehicles to simulate         */
#define N_CAPACITY             10       /* max vehicles inside institute      */
#define CARS_TIMING            50000    /* microseconds between car arrivals  */
#define VIP_RATIO              4        /* 1-in-VIP_RATIO chance of VIP       */
#define MECHANIC               3        /* number of mechanics                */
#define DIAGNOSTIC_TABLETS     2        /* number of diagnostic tablets       */
#define TORQUE_WRENCH          2        /* number of torque wrenches          */
#define TESTER                 2        /* number of pollution testers        */
#define POLLUTION_QUEUE        2        /* transfer queue size to station 2   */
#define POLLUTION_WORKERS      2        /* concurrent testers allowed         */
#define MONITOR_INTERVAL       100000   /* microseconds between aging checks  */
#define WAIT_THRESHOLD         0.4      /* seconds before regular -> VIP      */
#define RAND_100K              100000   /* mechanic work time lower bound     */
#define RAND_200K              100000   /* mechanic work time range           */
#define POLLUTION_WORKING_TIME 150000   /* pollution test duration (us)       */
#define PAYMENT_BOX            1        /* single payment booth               */

/* ── Forward declarations ───────────────────────────────────────────────── */
typedef struct car_entry car_entry;
typedef struct priority_queue priority_queue;

/* ── Data structures ────────────────────────────────────────────────────── */

/**
 * car_entry - represents a single vehicle and its thread state.
 * @car_id:     unique vehicle identifier (1-based)
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

/* ── Semaphores (defined in main.c) ─────────────────────────────────────── */
extern sem_t slots_sem, queue_sem, done_sem;
extern sem_t mechanic_sem, diagnostic_sem, torque_sem;
extern sem_t pollution_queue_sem, pollution_worker_sem;
extern sem_t payment_box;

/* ── Worker ID tracking (defined in threads.c) ──────────────────────────── */
extern int mechanic_available[MECHANIC];
extern pthread_mutex_t mechanic_mutex;
extern int tester_available[TESTER];
extern pthread_mutex_t tester_mutex;

/* ── Print mutex (defined in utils.c) ───────────────────────────────────── */
extern pthread_mutex_t print_mutex;

/* ── Function declarations ──────────────────────────────────────────────── */
void*      car_creator_thread(void*);
void*      monitor_thread(void*);
void*      car_thread(void*);
void*      manager_thread(void*);
void       insertion_sort(priority_queue*);
void       pq_insert(priority_queue*, car_entry*);
car_entry* pq_pop(priority_queue*);
void       print_log(int, const char*);
int        is_vip(void);

#endif /* GARAGE_H */