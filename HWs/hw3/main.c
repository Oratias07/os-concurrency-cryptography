/**
 * main.c
 *
 * Entry point. Initializes all semaphores and mutexes,
 * spawns the creator/monitor/manager threads, and waits
 * for all K_CARS vehicles to complete treatment.
 */

#include "garage.h"

/* ── Semaphore definitions ──────────────────────────────────────────────── */
sem_t slots_sem, queue_sem, done_sem;
sem_t mechanic_sem, diagnostic_sem, torque_sem;
sem_t pollution_queue_sem, pollution_worker_sem;
sem_t payment_box;

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