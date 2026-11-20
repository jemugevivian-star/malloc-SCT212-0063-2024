# Benchmark and Fragmentation Report

## Setup
I ran 50,000 random `malloc` and `free` operations across 1,024 slots with payload sizes from 1 to 4,096 bytes and a fixed seed (`srand(42)`)[cite: 13]. Execution time was measured with `clock_gettime(CLOCK_MONOTONIC)`, and heap metrics were taken from `heap_stats()` right after the loop finished.

## Results

| Policy / Allocator | Time (ms) | `heap_bytes` | `free_bytes` | `free_blocks` | `largest_free` | External Fragmentation |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| `FIT_FIRST` | 159.61 | 1,376,256 | 204,624 | 223 | 50,944 | 0.751 |
| `FIT_BEST` | 207.97 | 1,376,256 | 205,424 | 162 | 116,320 | 0.434 |
| `libc malloc` | 3.41 | - | - | - | - | - |

External fragmentation is calculated as  - \text{largest\_free} / \text{free\_bytes}$.

### Raw Output
```text
FIT_FIRST  |  140.38 ms | heap=1376256 free=204624 blocks=223 largest=50944 frag=0.751
FIT_BEST   |  181.23 ms | heap=1376256 free=205424 blocks=162 largest=116320 frag=0.434
LIBC       |    3.57 ms | libc baseline
```

## Observations

**First-Fit vs. Best-Fit:** `FIT_FIRST` took 159.61 ms while `FIT_BEST` took 207.97 ms[cite: 13]. Stopping at the first block that fits saves scan time compared to checking every block for the closest size match. On the other hand, `FIT_BEST` had lower external fragmentation (0.434 vs. 0.751) and left a larger contiguous free block (116,320 bytes vs. 50,944 bytes across fewer free blocks: 162 vs. 223) because it avoids carving up big holes when a tighter match exists[cite: 13].

**Comparison with glibc `malloc`:** System `malloc` ran the same 50,000 operations in 3.41 ms[cite: 13]. My allocator walks a single linked list and grows via `sbrk`, whereas `ptmalloc2` uses size-segregated bins (such as `tcache` and small/large bins), doubly linked free structures with boundary tags for fast coalescing, and `mmap` for larger requests.
