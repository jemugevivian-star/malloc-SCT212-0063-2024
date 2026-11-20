#define _POSIX_C_SOURCE 199309L
#include "myalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define OPS 50000
#define SLOTS 1024
#define MAX_SZ 4096

static double ms_diff(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6;
}

static void run_custom(const char *name, fit_policy_t pol) {
    void *p[SLOTS] = {0};
    struct timespec t0, t1;
    heap_stats_t st;

    set_policy(pol);
    srand(42);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < OPS; i++) {
        int k = rand() % SLOTS;
        if (p[k]) {
            my_free(p[k]);
            p[k] = NULL;
        } else {
            p[k] = my_malloc((size_t)(rand() % MAX_SZ) + 1);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    heap_stats(&st);
    double frag = st.free_bytes ? 1.0 - (double)st.largest_free / st.free_bytes : 0.0;
    printf("%-10s | %7.2f ms | heap=%zu free=%zu blocks=%zu largest=%zu frag=%.3f\n",
           name, ms_diff(t0, t1), st.heap_bytes, st.free_bytes,
           st.free_blocks, st.largest_free, frag);

    for (int i = 0; i < SLOTS; i++) {
        my_free(p[i]);
    }
}

static void run_libc(void) {
    void *p[SLOTS] = {0};
    struct timespec t0, t1;

    srand(42);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < OPS; i++) {
        int k = rand() % SLOTS;
        if (p[k]) {
            free(p[k]);
            p[k] = NULL;
        } else {
            p[k] = malloc((size_t)(rand() % MAX_SZ) + 1);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    printf("%-10s | %7.2f ms | libc baseline\n", "LIBC", ms_diff(t0, t1));
    for (int i = 0; i < SLOTS; i++) {
        free(p[i]);
    }
}

int main(void) {
    run_custom("FIT_FIRST", FIT_FIRST);
    run_custom("FIT_BEST", FIT_BEST);
    run_libc();
    return 0;
}
