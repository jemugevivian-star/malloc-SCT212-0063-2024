# myalloc

- **Name:** Vivian Jemuge
- **Registration Number:** SCT212-0063/2024
- **Repository:** https://github.com/jemugevivian-star/malloc-SCT212-0063-2024

## Build and Run

```bash
# Build and run unit tests
make

# Run with AddressSanitizer and UBSan
gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined -o t_asan myalloc.c test_myalloc.c && ./t_asan && rm -f t_asan

# Run benchmark
gcc -std=c11 -Wall -Wextra -O2 -o bench myalloc.c bench.c && ./bench
```

## Progress

| Task | Summary | Status |
| :--- | :--- | :--- |
| Task 1 | 16-byte alignment and `block_t` layout | Done |
| Task 2 | Heap setup and `sbrk` growth | Done |
| Task 3 | Free-block reuse and splitting | Done |
| Task 4 | Adjacent free-block coalescing | Done |
| Task 5 | `my_calloc` and `my_realloc` | Done |
| Task 6 | Magic values and `heap_check` | Done |
| Task 7 | Fit policies, `heap_stats`, and `heap_dump` | Done |
| Task 8 | `bench.c`, `REPORT.md`, and `README.md` | Done |

## Test Output

```text
T1 basic allocation
  PASS  returns non-NULL
  PASS  16-byte aligned
  PASS  heap consistent
  PASS  malloc(0) is NULL
  PASS  free(NULL) is safe
T2 blocks do not overlap
  PASS  data intact in all blocks
  PASS  heap consistent after frees
T3 reuse and splitting
  PASS  freed space is reused (no growth)
  PASS  heap did not grow
  PASS  block was split (remainder is free)
T4 coalescing
  PASS  all memory is free
  PASS  adjacent free blocks merged into one
  PASS  merged block satisfies a 550-byte request
T5 calloc and realloc
  PASS  calloc zeroes memory
  PASS  calloc overflow returns NULL
  PASS  realloc preserves contents
  PASS  shrinking keeps prefix
  PASS  realloc(NULL, n) acts like malloc
  PASS  realloc(p, 0) frees and returns NULL
  PASS  heap consistent
T6 heap growth via sbrk
  PASS  large request succeeds
  PASS  heap grew
  PASS  heap consistent
T7 best-fit vs first-fit
  PASS  first-fit takes the first hole
  PASS  best-fit takes the tightest hole
T8 randomized stress test
  PASS  no data corruption during 20000 operations
  PASS  everything freed
  PASS  fully coalesced (few free blocks remain)
  PASS  heap consistent

Result: 29 passed, 0 failed
```

## How It Works
`myalloc` keeps an intrusive singly linked list of `block_t` headers in address order, each placed right before a 16-byte aligned payload. The initial program break is aligned to 16 bytes with `sbrk`, and every requested size is rounded up to a multiple of 16 bytes. `my_malloc` searches free blocks using either `FIT_FIRST` or `FIT_BEST`, splits a block when the leftover space can hold `HDR + 16` bytes, and calls `sbrk` when no suitable free block is found. `my_free` checks the block's magic number, marks it free, and merges adjacent free blocks on the heap. `my_calloc` checks for multiplication overflow before zeroing memory, and `my_realloc` handles `NULL` and zero-size cases, shrinks in place, grows into an adjacent free block if possible, or copies data into a newly allocated block.

## Written Questions

### 1. Why 16-byte alignment?
On 64-bit platforms (like x86-64 and ARM64) and for C11 `max_align_t` types, 16-byte alignment avoids alignment faults on vector/SIMD instructions and keeps naturally aligned types properly positioned. Rounding payload sizes to multiples of 16, keeping `sizeof(block_t)` a multiple of 16, and aligning the starting break ensure every returned pointer is 16-byte aligned.

### 2. Metadata overhead and minimum split size
Each block uses `HDR` bytes for its header. Splitting only happens when `blk->size >= need + HDR + 16` so the split remainder has room for a `block_t` header and at least 16 bytes of payload. Without that check, tiny or zero-byte fragments would pile up in the list without being useful.

### 3. Coalescing and time complexity
If adjacent free blocks are not merged, small freed chunks stay separated and large allocations can fail even when total free memory looks big enough. Walking the list in address order and merging contiguous free blocks takes (n)$ time per free pass; adding boundary tags (footers) and a doubly linked free list would allow (1)$ coalescing.

### 4. First-Fit vs. Best-Fit
First-Fit picks the first free block that fits, so it usually scans less of the list per allocation but can leave small fragments near the start of the heap. Best-Fit checks the whole list for the tightest fit, which helps preserve larger holes at the cost of scanning more blocks on each allocation.

### 5. `realloc` cases and `calloc` overflow checks
`my_realloc(NULL, n)` behaves like `my_malloc(n)`, and `my_realloc(p, 0)` frees `p` and returns `NULL`. When shrinking, extra trailing space can be split off if large enough; when growing, a free next-door block can absorb the request if combined size is enough, otherwise a new block is allocated, the smaller of the old and new sizes is copied, and the old block is freed. In `my_calloc(nmemb, size)`, checking `nmemb != 0 && size > SIZE_MAX / nmemb` catches overflow before `nmemb * size` wraps around.

### 6. Corruption checks and heap invariants
Using distinct magic numbers for active and freed blocks catches bad pointers, double frees, and buffer overflows that overwrite the next header. `heap_check()` checks 16-byte alignment, valid magic values, ascending block addresses, and that adjacent free blocks are not left uncoalesced.

## Limitations
- Not thread-safe (uses global list pointers with no locks).
- Never shrinks the heap back down with negative `sbrk`.
- Linear list scans can slow down as the number of blocks grows.

## Sources and AI Use
- Course notes, *Computer Systems: A Programmer's Perspective* (Ch. 9), and *Operating Systems: Three Easy Pieces* (Ch. 17).
- Used an AI coding assistant for debugging, testing, and drafting/editing documentation.
