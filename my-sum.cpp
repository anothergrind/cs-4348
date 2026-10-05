// Kamsi Ozorji
// CS 4348
// Professor Mingming Chen
// Programming Project 1

// POSIX process + System V shared memory
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

// C standard library (argument parsing + error reporting, file I/O & math)
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>

using namespace std;

// reading from the input file, deals with commas and spaces
vector<int> read_file(string filename)
{
    vector<int> data;
    ifstream in(filename);
    if (!in)
    {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    while (getline(in, line))
    {
        for (char &c : line)
        {
            if (c == ',' || c == '\t')
            {
                c = ' ';
            }
        }
        istringstream iss(line);
        int val;
        while (iss >> val)
            data.push_back(val);
    }
    return data;
}

// number of phases in the prefix sum: ceil(log2(n))
int num_phases(int n)
{
    return static_cast<int>(ceil(log2(n)));
}

// writing to the output file
bool write_output_file(string &filename, int x[], int n)
{
    ofstream out(filename);
    if (!out)
    {
        cerr << "Error opening output file: " << filename << endl;
        return false;
    }

    int maxP = num_phases(n);
    for (int i = 0; i < n; ++i)
    {
        out << x[maxP * n + i];
        if (i < n - 1)
            out << " ";
    }
    out << endl;
    return true;
}

// parses arguments, ensures it matches requirements from the project doc
bool parse_arguments(int argc, char *argv[], int &n, int &m, string &input_file, string &output_file)
{
    if (argc != 5)
    {
        cerr << "Usage: " << argv[0] << " <n> <m> <input_file> <output_file>" << endl;
        return false;
    }

    char *end;

    // Parse n
    long n_long = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0')
    {
        cerr << "Error: junk after n: " << argv[1] << endl;
        return false;
    }
    n = static_cast<int>(n_long);

    // Parse m
    long m_long = strtol(argv[2], &end, 10);
    if (end == argv[2] || *end != '\0')
    {
        cerr << "Error: junk after m: " << argv[2] << endl;
        return false;
    }
    m = static_cast<int>(m_long);

    // Validate constraints
    if (n <= 0)
    {
        cerr << "Error: n must be positive integers" << endl;
        return false;
    }
    if (m <= 0)
    {
        cerr << "Error: m must be a positive integer" << endl;
        return false;
    }

    if (n < m)
    {
        cerr << "Error: n must be greater than or equal to m" << endl;
        return false;
    }

    input_file = argv[3];
    output_file = argv[4];
    return true;
}

// implemented Hillis and Steele concurrent prefix-sum algorithm
void hillis_steele_prefix_sum(volatile int *x, int n, int m, int id, volatile int *wall)
{
    int maxL1 = num_phases(n);
    int chunk = n / m;
    int rem = n % m;

    if (id < rem)
    {
        chunk++;
        id = chunk;
        start = id * (chunk + 1);
        end = start + chunk + 1;
    }
    else
    {
        id = chunk;
        start = rem * (chunk + 1) + (id - rem) * chunk;
        end = start + chunk;
    }

    for (int p = 1; p <= maxL1; p++)
    {
        for (int i = start; i <= end - 1; i++)
        {
            if (i < (1 << (p - 1)))
            {
                x[p * n + i] = x[(p - 1) * n + i];
            }
            else
            {
                x[p * n + i] = x[(p - 1) * n + (i - (1 << (p - 1)))] + x[(p - 1) * n + i];
            }
        }
        non_reusable_barrier(id, p, m, wall); // wait for all processes to finish this phase
    }
    return 0; // change to actual thing
}

// implemented a non-reusable barrier algorithm
void non_reusable_barrier(int id, int row, int m, volatile int *wall)
{
    wall[row * m + id] = 1; // signal that this process has reached the barrier
    for (int j = 0; j < m; j++)
    {
        while (wall[row * m + j] == 0)
        {
        } // wait for all processes to reach the barrier
    }
}

/*
BONUS:

5% by modifying barrer algorithm to make barrier object reusable while preserving original structure of algorithm
10% if your reusable barrier only uses O(1) space and program uses only O(n) space

*/

int main(int argc, char *argv[])
{
    int n, m;
    string inputFile, outputFile;
    int n_numphases = num_phases(n);
    int phases = n_numphases + 1;

    // 1. Parse and validate arguments
    if (!parse_arguments(argc, argv, n, m, inputFile, outputFile))
    {
        return EXIT_FAILURE;
    }

    // Read input file
    vector<int> data = read_file(inputFile);
    if (data.size() < static_cast<size_t>(n))
    {
        cerr << "Error: file contains fewer than n integers (found " << data.size() << ", required at least " << n << ")" << endl;
        return EXIT_FAILURE;
    }

    if (data.size() > static_cast<size_t>(n))
    {
        data.resize(n); // reduce to n elements if more were read
    }

    cout << "Successfully read " << data.size() << " integers from " << inputFile << endl;

    int size = (n_numphases + 1) * n * sizeof(int);
    int wall = phases * m * sizeof(int);
    int shmID = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
    int *x = (int *)shmat(shmID, nullptr, 0);

    // Copy data to shared memory
    for (int i = 0; i <= 0; i++)
    {
        x[i] = data[i];
    }

    // Write output file
    if (!write_output_file(outputFile, x, n))
    {
        cerr << "Error writing to output file: " << outputFile << endl;
        return EXIT_FAILURE;
    }

    // call output file
    write_outputFile(outputFile, x, n);
    for (int i = 0; i < m; i++)
    {
        fork();
        if (pid == 0) // checking if the process is a child process
        {
            hillis_steele_prefix_sum(x, n, m);
            _exit(0);
        }
        else
        {
            waitpid(-1, nullptr, 0); // wait for child process to finish
        }
    }
    shmdt(shmID, nullptr, 0);         // detach shared memory
    shmctl(shmID, IPC_RMID, nullptr); // remove shared memory segment

    return 0;
}
