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

static void coalesce(void) {
    block_t *cur = head;
    while (cur != NULL && cur->next != NULL) {
        block_t *nxt = cur->next;
        char *end_of_cur = (char *)cur + HDR + cur->size;

        if (cur->free && nxt->free && end_of_cur == (char *)nxt) {
            cur->size += HDR + nxt->size;
            cur->next = nxt->next;
            if (tail == nxt) {
                tail = cur;
            }
        } else {
            cur = cur->next;
        }
    }
}

void my_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    block_t *blk = header(ptr);

    if ((unsigned int)blk->magic != MAGIC_USED && (unsigned int)blk->magic != MAGIC_FREE) {
        const char msg[] = "my_free: invalid pointer\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }

    if (blk->free || (unsigned int)blk->magic == MAGIC_FREE) {
        const char msg[] = "my_free: double free detected\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }

    blk->free = 1;
    blk->magic = MAGIC_FREE;
    coalesce();
}

void *my_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }
    if (nmemb > SIZE_MAX / size) {
        return NULL;
    }

    size_t total = nmemb * size;
    void *p = my_malloc(total);
    if (p != NULL) {
        memset(p, 0, total);
    }
    return p;
}

void *my_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return my_malloc(size);
    }
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    size_t need = align_up(size);
    if (need == 0) {
        return NULL;
    }

    block_t *blk = header(ptr);

    if (blk->size >= need) {
        split_block(blk, need);
        return ptr;
    }

    block_t *nxt = blk->next;
    if (nxt != NULL && nxt->free &&
        (char *)blk + HDR + blk->size == (char *)nxt &&
        blk->size + HDR + nxt->size >= need) {
        blk->size += HDR + nxt->size;
        blk->next = nxt->next;
        if (tail == nxt) {
            tail = blk;
        }
        split_block(blk, need);
        return ptr;
    }

    void *new_ptr = my_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    size_t copy_sz = blk->size < size ? blk->size : size;
    memcpy(new_ptr, ptr, copy_sz);
    my_free(ptr);
    return new_ptr;
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
