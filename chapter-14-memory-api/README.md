# OSTEP Chapter 14: Memory API

Homework for OSTEP, chapter 14 ("Interlude: Memory API"). Small C programs with deliberate memory bugs, inspected with gdb and valgrind.

**Main takeaway:** a program that runs is not necessarily a correct program.

## Build and run

```sh
gcc -g -Wall ex4.c -o ex4.out          # -g gives line numbers in gdb/valgrind
./ex4.out
gdb ./ex4.out
valgrind --leak-check=yes ./ex4.out
```

## Overview: who notices which bug?

| Exercise | Bug                            | Normal run                | valgrind         |
|----------|--------------------------------|---------------------------|------------------|
| 1 to 3   | NULL dereference               | segfault (kernel)         | Invalid read     |
| 4        | memory leak                    | nothing                   | definitely lost  |
| 5        | heap buffer overflow           | nothing                   | Invalid write    |
| 6        | use-after-free                 | nothing, prints garbage   | Invalid read     |
| 7        | invalid pointer passed to free | abort (glibc)             | Invalid free     |

Three of the five bugs go unnoticed without a tool.

## Exercises 1 to 3: dereferencing a NULL pointer

```c
#include <stddef.h>

int main() {
    int *test = NULL;
    int test_dereferenced = *test;
}
```

The pointer holds address `0x0`, then the program tries to read the value stored there. Result: segmentation fault.

Why: `NULL` is not a random address, it is always 0. Nothing is mapped at that address in the process's address space. What happens step by step:

1. The CPU tries to read 4 bytes starting at address 0.
2. The MMU finds no valid page table entry and raises a page fault.
3. The kernel checks whether the address belongs to a valid region of the process. It does not.
4. The kernel sends `SIGSEGV` to the process.

gdb (exercise 2) shows the exact location:

```
Program received signal SIGSEGV, Segmentation fault.
0x000055555555513d in main () at null.c:5
5        int test_dereferenced = *test;
```

valgrind (exercise 3) reports the same thing from its own bookkeeping:

```
Invalid read of size 4
 Address 0x0 is not stack'd, malloc'd or (recently) free'd
Process terminating with default action of signal 11 (SIGSEGV)
 Access not within mapped region at address 0x0
```

> **Side note:** Linux deliberately leaves the lowest pages unmapped (`/proc/sys/vm/mmap_min_addr`, typically 65536). This costs no physical RAM because the addresses are virtual, and it makes NULL accesses fail immediately. It also blocks an old class of exploits. The kernel is mapped into the upper half of every process's address space. If a kernel bug dereferences a NULL pointer, the access lands in the user part. If a process could place its own data there, the kernel would use it with full privileges.

## Exercise 4: memory leak

```c
#include <stdlib.h>

int main() {
    int *p = malloc(4 * sizeof(int));
    p[0] = 1;
    p[1] = 2;
    p[2] = 3;
    p[3] = 4;
}
```

`malloc` reserves heap space for 4 integers, so 16 bytes (an `int` is 4 bytes). The memory is never returned with `free`.

A normal run and gdb show nothing, because no single access is invalid. Only valgrind sees it:

```
HEAP SUMMARY:
    in use at exit: 16 bytes in 1 blocks
  total heap usage: 1 allocs, 0 frees, 16 bytes allocated

16 bytes in 1 blocks are definitely lost in loss record 1 of 1
   at 0x4846828: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
   by 0x10915E: main (ex4.c:4)
```

The stack trace points to where the block was allocated. A leak itself has no location.

In context: for a short program this has no consequences, because the kernel tears down the whole address space when the process exits. There are two layers: `malloc`/`free` inside the process, and the kernel's page management underneath. Leaks hurt in long-running processes such as servers or a shell.

## Exercise 5: writing past the end of an array

```c
#include <stdlib.h>

int main() {
    int *p = malloc(100 * sizeof(int));
    p[100] = 0;
}
```

400 bytes are reserved for indices 0 to 99. `p[100]` writes 4 bytes directly behind the block.

Normal run: no crash. valgrind:

```
Invalid write of size 4
   at 0x10916D: main (ex5.c:5)
 Address 0x4a851d0 is 0 bytes after a block of size 400 alloc'd
   at 0x4846828: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
   by 0x10915E: main (ex5.c:4)
```

On top of that comes a 400 byte leak, since `free` is missing here too.

Why no segfault: the MMU only knows pages (4096 bytes), not malloc blocks. glibc requests one large piece of heap from the kernel (about 132 KiB) and splits it up itself. The memory behind the 400 byte block is still mapped, so from the hardware's point of view the access is legal. valgrind replaces `malloc` and tracks the boundaries of every block down to the byte.

The program is still wrong (undefined behavior). A few bytes further sits the metadata of the next block, which is the starting point for heap exploitation.

## Exercise 6: use-after-free

```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = malloc(sizeof(int) * 2);
    free(p);

    printf("%d\n", p[1]);
    return 0;
}
```

The program accesses memory after freeing it. After the `free`, `p` is a *dangling pointer* (it points to memory the program no longer owns). Accessing memory through it is called *use-after-free*.

The program runs to completion and prints some value. valgrind:

```
Invalid read of size 4
   at 0x1091B7: main (ex6.c:8)
 Address 0x4a85044 is 4 bytes inside a block of size 8 free'd
   at 0x484988F: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
   by 0x1091AE: main (ex6.c:6)
 Block was alloc'd at
   at 0x4846828: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
   by 0x10919E: main (ex6.c:5)
```

Three stack traces: the invalid access, the `free`, and the allocation.

Why no segfault: `free` does not hand the memory back to the kernel. It only puts the block on glibc's internal free list. The page stays mapped. The contents now belong to the allocator, which stores its own metadata there and may hand the block out again on the next `malloc`.

## Exercise 7: passing the wrong pointer to free

```c
#include <stdlib.h>

int main() {
    int *p = malloc(3 * sizeof(int));
    free(p + 1);
    return 0;
}
```

`free` does not receive the pointer that `malloc` returned, but an address shifted by 4 bytes into the middle of the block.

```
free(): invalid pointer
[1]    131523 IOT instruction (core dumped)  ./ex7.out
```

Why: `free` only gets an address, no size. The size is stored in a header that `malloc` places right before the block (in glibc, 8 bytes at `p - 8`). With the shifted pointer, `free` looks in the wrong place and would read garbage as the block size. glibc runs sanity checks first (blocks always sit at multiples of 16, `p + 4` does not) and aborts on its own via `abort()`.

This is not a segfault from the kernel but `SIGABRT` from the library. "IOT instruction" is the old name for the same signal, which is how zsh prints it.

Answer to the book's question: no tool is needed for this bug, the runtime library catches it. The checks are only hardening though. Metadata and user data sit next to each other on the heap without protection, and a well forged header would be accepted.
