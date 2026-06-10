/**
 * utils.c
 *
 * Utility functions:
 *   - print_log: timestamped log prefix printer
 *   - is_vip:    randomizes vehicle type
 */

#include "garage.h"

/* Serializes all printf output to prevent interleaved lines across threads */
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ── print_log ──────────────────────────────────────────────────────────── */
/**
 * Prints a timestamped prefix: [HH:MM:SS.mmm] [ ROLE   #ID]
 * Role is left-padded to 6 chars for column alignment.
 * Caller must hold print_mutex before calling.
 */
void print_log(int car_id, const char* role) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* tm_info = localtime(&tv.tv_sec);
    printf("[%02d:%02d:%02d.%03ld] [ %-6s #%02d] ",
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec,
        tv.tv_usec / 1000,
        role, car_id);
}

/* ── is_vip ─────────────────────────────────────────────────────────────── */
/* Returns 1 (VIP) with probability 1/VIP_RATIO, else 0 (regular). */
int is_vip(void) { return rand() % VIP_RATIO == 0; }