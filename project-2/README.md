# Programming Project 2

## CS 4348: Operating Systems

Professor Mingming Chen

Project 2 Team 5

Kamsi Ozorji and Khoa Bui

# Dining Philosophers

This project implements the Dining Philosophers problem in C++17 using
Peterson locks and `std::thread`. It runs two solutions:

- **Coarse-grained:** one tournament-tree lock protects all chopsticks.
- **Fine-grained:** each chopstick has its own two-thread Peterson lock.

## Building and running

```bash
make
./my-dining 5
```

The argument must be an integer `n >= 2`. The program runs five
thinking/eating cycles for each philosopher and prints the coarse-grained
solution before the fine-grained solution.

To remove the executable:

```bash
make clean
```

## Synchronization design

The Peterson lock supports exactly two participants, identified by local IDs
`0` and `1`. Its flags and victim variable are `std::atomic`.

The tournament-tree lock uses one Peterson lock at every internal tree node.
The number of leaves is the smallest power of two greater than or equal to
`n`. Philosopher `i` is assigned to leaf `leaves + i`; unused leaves are never
assigned to a thread. A philosopher acquires Peterson locks from its leaf to
the root and releases them in reverse order.

In the fine-grained solution, chopstick `c` is shared only by philosophers
`c` and `(c + 1) mod n`. Each philosopher acquires the lower-numbered of its
two chopsticks first and releases the locks in reverse order. This prevents a
circular wait, so the fine-grained solution is deadlock-free. The
coarse-grained solution is deadlock-free because every philosopher uses the
single tournament-tree lock before accessing either chopstick.

## Output

The program prints `BEGIN COARSE` and `END COARSE`, followed by
`BEGIN FINE` and `END FINE`. Activity records have this format:

```text
SOLUTION philosopher_id round event
```

where `SOLUTION` is `COARSE` or `FINE`, `round` is `0` through `4`, and
`event` is `THINK_START`, `THINK_END`, `EAT_START`, or `EAT_END`.
