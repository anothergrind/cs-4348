// Kamsi Ozorji
// CS 4348
// Professor Mingming Chen
// Programming Project 1

// POSIX process + System V shared memory
#include <sys/types.h> // pid_t, key_t
#include <sys/ipc.h>   // IPC_PRIVATE, IPC_CREAT
#include <sys/shm.h>   // shmget, shmat, shmdt, shmctl
#include <sys/wait.h>  // waitpid
#include <unistd.h>    // fork, _exit

// C standard library (argument parsing + error reporting)
#include <cstdlib> // strtol, EXIT_FAILURE
#include <cerrno>  // errno
#include <cstring> // strerror

// C++ standard library (file I/O in the main process only)
#include <iostream> // cerr
#include <fstream>  // input/output files
#include <string>
#include <vector> // parent-local only; shared arrays live in shared memory

using namespace std;

/*
IMPLEMENTATION CONSTRAINTS

Use of Locks, semaphores, atomic instructions, or any other syncrhonization mechanism isn't allowed
Proces sycnchronization can be achieved only through read and write oeprations on shared memory
*/

// Submission must have Makefile to compile program, and README file with names of all team members and instructions for running compiled program
// TODO: Implement the Hillis and Steele concurrent prefix-sum algorithm. This algorithm takes in an input array
int hillis_steele_prefix_sum(int[] x)
{
    for (int p = 1; p <= log2(n); p++)
    {
        for (i = 0; i <= n - 1; i++)
        {
            if (i < 2 ^ (p - 1))
            {
                x[p][i] = x[p - 1][i];
            }
            else
            {
                x[p][i] = x[p - 1][i - 2 ^ (p - 1)] + x[p - 1][i];
            }
        }
    }
    return 0; // change to actual thing
}

/*
Algorithm 1 Hillis and Steele concurrent prefix-sum algorithm
1: x[0] ← the input array
2: for p ←1 to ⌈log2 n⌉ do
3:      for i ←0 to n −1 in parallel do
4:          if i < 2^(p−1) then
5:              x[p][i] ←x[p −1][i]
6:          else
7:              x[p][i] ←x[p −1][i −2^(p−1)] + x[p −1][i]
8:          end if
9:      end for
10: end for
*/

// TODO: Implement Algorithm 2, A non-reusable barrier

/*
Algorithm 2 A non-reusable barrier
1: wall[1..m] ←{0, 0, . . . , 0}                ▷ shared by all processes

2: Code for process pi:
3: wall[i] ←1
4: while ∃j : wall[j] = 0 do
5:      skip                                    ▷ spin
6: end while
*/
void non - reusable - barrier(int wall[])
{
    wall[i] = 1;
    while (true)
    {
        bool all_done = true;
        for (int j = 1; j <= m; j++)
        {
            if (wall[j] == 0)
            {
                all_done = false;
                break;
            }
        }
        if (all_done)
        {
            break;
        }
    }
}
// TODO: Ensure your program uses O(n log_2 n) space and run in O( ([n log_2 n] / m) + m log_2 n) time.
// You can assume n >= m

/*
BONUS:

5% by modifying barrer algorithm to make barrier object reusable while preserving original structure of algorithm
10% if your reusable barrier only uses O(1) space and program uses only O(n) space

*/