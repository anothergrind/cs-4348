# Programming Project 2

## CS 4348: Operating Systems

Professor Mingming Chen

Project 2 Team 5

Kamsi Ozorji and Khoa Bui

## Overview

`my-dining` solves the Dining Philosophers problem twice, using only locks built from Peterson's two-thread algorithm:

1. **Coarse-grained:** one n-thread Tournament-Tree lock protects all chopsticks.
2. **Fine-grained:** each chopstick has its own two-thread Peterson lock.

Each philosopher is a `std::thread` and completes five think/eat cycles. The coarse run finishes before the fine run starts.

## Compile and run

```bash
make clean
make
./my-dining n      # n >= 2, e.g. ./my-dining 5
```

## Tournament-Tree lock design

The tree is stored in a vector, heap style:

- `size` is the smallest power of two >= n. Internal nodes are `1 .. size-1`, and each one is a `PetersonLock`.
- Thread `tid` starts at leaf `size + tid`. Node `k`'s parent is `k / 2`, and `k % 2` (left child 0, right child 1) is its Peterson id there, so the two threads meeting at a node always have different ids.
- **acquire:** go from the leaf up to the root, acquiring each parent lock. Holding the root means holding the lock.
- **release:** release the same locks in reverse, from the root down.
- **n not a power of two:** leaves `n .. size-1` have no thread. Their Peterson flag stays `false`, so real threads never wait on them.

## Fine-grained deadlock prevention

Chopstick `i` is between philosophers `i` and `(i + 1) % n`. Philosopher `i` uses chopsticks `(i - 1 + n) % n` (left) and `i` (right). Each chopstick lock has exactly two users, so it is a valid Peterson lock.

- Philosophers `0 .. n-2` pick up the **left** chopstick first, then the right.
- Philosopher `n-1` picks up the **right** chopstick first, then the left.

Deadlock needs every philosopher holding one chopstick and waiting for the next around the circle. Here philosopher 0 and philosopher `n-1` both want chopstick `n-1` first. Only one can hold it, so the other holds nothing and the circle is broken. Locks are released in reverse order. Neighbours share a chopstick lock, so they never eat together.

## Assumptions and limitations

- `n` must be an integer >= 2. Anything else, or a wrong number of arguments, prints an error to stderr and exits with 1.
- One thread per philosopher, so very large `n` is limited by the OS. If a thread cannot be created, the program prints an error, joins the threads that started, and exits with 1.
- The locks spin (calling `yield()`), which uses CPU while threads wait.
- Each thread has its own `thread_local` random generator for the 1-500 ms delays. Each event line is written with a single `printf` call.
- Coarse allows one eater at a time. Fine allows non-neighbouring philosophers to eat together.
