/**
 * queue.c
 *
 * Priority queue operations for the vehicle service center.
 *
 * The queue is a sorted array of car_entry pointers.
 * Sort order: descending priority, then ascending enter_time (FIFO within tier).
 * All operations are protected by pq->mutex.
 */

#include "garage.h"

/* ── insertion_sort ─────────────────────────────────────────────────────── */
/**
 * Sorts pq->cars[] in-place using insertion sort.
 * Called only while pq->mutex is held by the caller (monitor_thread).
 *
 * Sort criteria:
 *   1. Higher priority first (VIP=1 before Regular=0)
 *   2. Among equal priority: earlier enter_time first (FIFO)
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

/* ── pq_insert ──────────────────────────────────────────────────────────── */
/**
 * Inserts a car into the priority queue in sorted position.
 * Higher priority comes first; ties broken by earlier enter_time (FIFO).
 * Posts queue_sem to notify manager_thread that a vehicle is waiting.
 *
 * Returns without inserting if queue is full (logs error to stderr).
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