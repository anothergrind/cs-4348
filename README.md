## Programming Project 1

### CS 4348: Operating Systems

Professor Mingming Chen
Team Members: Kamsi Ozorji and Khoa Bui

This program implements the Hillis and Steele concurrent prefix-sum algorithm using POSIX processes (`fork`, `waitpid`) and shared memory (`shmget`, `shmat`). It is designed to run on Linux/Unix-based machines (e.g., Ubuntu, WSL, or UTD CS servers).

This program computes prefix sums with m forked workers, via the Hillis-Steele algorithm, using shared memory and a reusable barrier
Runs in O(n log n / m + m log n) time complexity, with O(n) space complexity

#### Compiling the program

From an Ubuntu/WSL terminal

```
cd your_library/cs-4348
make
```

To clean up previous compilations
`make clean`

The input file must contain at least `n` spaced integers. Currently the spacing can be done with spaces, tabs, commas, or a combination of those mentioned above.

### Running the program

The program acccepts four command line arguments in the order below:
`./my-sum <n> <m> <input_file> <output_file>`

- n, the number of values
- m, the number of worker processes
- A, the input file
- B, the output file

For example

```
printf "1 2 3 4 5\n" > input.txt
./my-sum 5 2 input.txt output.txt
cat output.txt
```

The output is:
`1 3 6 10 15`
