#define _DEFAULT_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "myalloc.h"

#define ALIGN 16
#define MIN_GROW (64 * 1024)
#define MAGIC_USED 0xA110C001
#define MAGIC_FREE 0xF00D0002

typedef struct block {
    size_t size;
    struct block *next;
    int free;
    int magic;
} block_t;

static size_t align_up(size_t n) {
    if (n > SIZE_MAX - (ALIGN - 1)) {
        return 0;
    }
    return (n + (ALIGN - 1)) & ~(size_t)(ALIGN - 1);
}

#define HDR align_up(sizeof(block_t))

static void *payload(block_t *block) {
    return (void *)((char *)block + HDR);
}

static block_t *header(void *ptr) {
    return (block_t *)((char *)ptr - HDR);
}

static block_t *head = NULL;
static block_t *tail = NULL;
static size_t heap_bytes = 0;

/* Task 2: Grow heap with sbrk() and append a free block */
static block_t *grow(size_t need) {
    size_t req = align_up(need + HDR);
    if (req == 0) {
        return NULL;
    }
    if (req < MIN_GROW) {
        req = MIN_GROW;
    }

    void *cur = sbrk(0);
    if (cur == (void *)-1) {
        return NULL;
    }

    /* Pad so payload ((char *)hdr + HDR) is 16-byte aligned */
    uintptr_t next_payload = (uintptr_t)cur + HDR;
    uintptr_t aligned_payload = (next_payload + (ALIGN - 1)) & ~(uintptr_t)(ALIGN - 1);
    size_t pad = (size_t)(aligned_payload - next_payload);

    if (req > SIZE_MAX - pad) {
        return NULL;
    }
    size_t total = pad + req;

    void *mem = sbrk((intptr_t)total);
    if (mem == (void *)-1) {
        return NULL;
    }

    heap_bytes += total;

    block_t *blk = (block_t *)((char *)mem + pad);
    blk->size = req - HDR;
    blk->next = NULL;
    blk->free = 1;
    blk->magic = MAGIC_FREE;

    if (head == NULL) {
        head = blk;
        tail = blk;
    } else {
        tail->next = blk;
        tail = blk;
    }

    return blk;
}

/* Task 3: First-fit search */
static block_t *find_fit(size_t need) {
    block_t *cur = head;
    while (cur != NULL) {
        if (cur->free && cur->size >= need) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

void *my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t need = align_up(size);
    if (need == 0) {
        return NULL;
    }

    block_t *blk = find_fit(need);
    if (blk == NULL) {
        blk = grow(need);
        if (blk == NULL) {
            return NULL;
        }
    }

    blk->free = 0;
    blk->magic = MAGIC_USED;
    return payload(blk);
}

/* Temporary placeholders so test_myalloc.c compiles for T1 and T2 */
void my_free(void *ptr) {
    (void)ptr;
}

void *my_calloc(size_t n, size_t size) {
    (void)n;
    (void)size;
    return NULL;
}

void *my_realloc(void *ptr, size_t size) {
    (void)ptr;
    (void)size;
    return NULL;
}

void heap_stats(heap_stats_t *out) {
    if (out != NULL) {
        memset(out, 0, sizeof(*out));
    }
}

void heap_dump(void) {
}

void set_policy(fit_policy_t p) {
    (void)p;
}

int heap_check(void) {
    return 1;
}
