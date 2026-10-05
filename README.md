# cs-4348
Project 1, CS 4348

Team members: Kamsi Ozorji, Khoa Bui

## Running `main.cpp`

This program uses POSIX/Linux APIs (`fork`, `waitpid`, and System V shared
memory). Run it in Linux or WSL; it will not compile as a native Windows
program with the regular Windows C++ libraries.

From an Ubuntu/WSL terminal:

```bash
cd /mnt/c/Users/mkhoa/OneDrive/Documents/cs-4348
make
```

The input file must contain at least `n` whitespace-separated integers. For
example:

```bash
printf "1 2 3 4 5\n" > input.txt
./my-sum 5 2 input.txt output.txt
cat output.txt
```

The output is:

```text
1 3 6 10 15
```

The four arguments are `n`, the number of values; `m`, the number of worker
processes; the input file; and the output file.

The program uses a reusable generation-based barrier and two alternating
working arrays. This avoids storing all intermediate Hillis-Steele rows and
uses `O(n)` space for the working arrays. The barrier stores one arrival
generation per worker, so its barrier state is `O(m)`; because `m <= n`, the
total shared memory remains `O(n)`. 
O(1) barrier bonus: we did not implement an `O(1)`-space
barrier. With read/write-only synchronization, each worker keeps its own
arrival slot.

To remove the compiled executable:

```bash
make clean
```
