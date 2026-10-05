// Kamsi Ozorji & Khoa Bui
// CS 4348
// Professor Mingming Chen
// Programming Project 1

/*
    This program computes prefix sums with m forked workers, via the Hillis-Steele algorithm, using shared memory and a reusable barrier
    Runs in O(n log n / m + m log n) time complexity, with O(n) space complexity
*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

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

// Reads integers from an input file, handles spaces, tabs, and commas
//  @param filename: the name of the input file to read from.
//  @return A vector with integers from the file

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

// Calculates number of prefix-sum phases needed, log2(n) rounded up
//  @param n: number of elements in the array
//  @return The number of phases needed

int num_phases(int n)
{
    return static_cast<int>(ceil(log2(n)));
}

// Writes the results to an output file
//  @param filename: the name of the output file to write to.
//  @param x: pointer to shared memory array that contains the results
//  @param n: number of elements in the array
//  @return true if successful, false otherwise

bool write_output_file(string &filename, volatile int *x, int n)
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
        out << x[(maxP % 2) * n + i];
        if (i < n - 1)
            out << " ";
    }
    out << endl;
    return true;
}

// Parses and validates arguments to ensure its correctly formatted and meets constraints listed in the document
//  @param argc: number of command line arguments
//  @param argv: array of command line arguments
//  @param n: reference to store the number of elements
//  @param m: reference to store the number of processes (workers)
//  @param input_file: reference to store the input file name
//  @param output_file: reference to store the output file name
//  @return true if arguments are valid, false otherwise

bool parse_arguments(int argc, char *argv[], int &n, int &m, string &input_file, string &output_file)
{
    if (argc != 5)
    {
        cerr << "Usage: " << argv[0] << " <n> <m> <input_file> <output_file>" << endl;
        return false;
    }

    char *end;

    // parse n
    long n_long = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0')
    {
        cerr << "Error: junk after n: " << argv[1] << endl;
        return false;
    }
    n = static_cast<int>(n_long);

    // parse m
    long m_long = strtol(argv[2], &end, 10);
    if (end == argv[2] || *end != '\0')
    {
        cerr << "Error: junk after m: " << argv[2] << endl;
        return false;
    }
    m = static_cast<int>(m_long);

    // validate constraints
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

// Implements a reusable barrier to sychronize processes using shared memory
//  @param id: the process ID (0 to m-1)
//  @param phase: the current phase of the prefix-sum algorithm
//  @param m: the number of processes (workers)
//  @param wall: pointer to the shared memory integer used for synchronization

void reusable_barrier(int id, int phase, int m, volatile int *wall)
{
    while (wall[0] != (phase - 1) * m + id)
    {
        // wait until its process's turn
    }
    wall[0] = (phase - 1) * m + id + 1; // signal that this process has arrived
    while (wall[0] < phase * m)
    {
        // wait until every process has arrived
    }
}

// Run's Hillis and Steele concurrent prefix-sum algorithm, maintains the time complexity O(n log n / m + m log n)
//  @param x: pointer to shared memory array that contains the input data and will hold the results
//  @param n: number of elements in the array
//  @param m: number of processes (workers)
//  @param id: the process ID (0 to m-1)
//  @param wall: pointer to the shared memory integer used for synchronization

void hillis_steele_prefix_sum(volatile int *x, int n, int m, int id, volatile int *wall)
{
    int start, end;
    int totalPhases = num_phases(n);
    int chunk = n / m;
    int rem = n % m;

    if (id < rem)
    {
        start = id * (chunk + 1);
        end = start + chunk + 1;
    }
    else
    {
        start = rem * (chunk + 1) + (id - rem) * chunk;
        end = start + chunk;
    }

    for (int p = 1; p <= totalPhases; p++)
    {
        for (int i = start; i <= end - 1; i++)
        {
            if (i < (1 << (p - 1)))
            {
                x[(p % 2) * n + i] = x[((p - 1) % 2) * n + i];
            }
            else
            {
                x[(p % 2) * n + i] = x[((p - 1) % 2) * n + (i - (1 << (p - 1)))] + x[((p - 1) % 2) * n + i];
            }
        }
        reusable_barrier(id, p, m, wall);
    }
}

// Runs the main program
//  @param argc: number of command line arguments
//  @param argv: array of command line arguments
//  @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise

int main(int argc, char *argv[])
{
    int n, m;
    string inputFile, outputFile;

    // parse and validate arguments
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

    int size = 2 * n * sizeof(int);
    int wallSize = sizeof(int);

    int shmID = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
    if (shmID == -1)
    {
        cerr << "Error creating shared memory for x: " << strerror(errno) << endl;
        return EXIT_FAILURE;
    }
    volatile int *x = (volatile int *)shmat(shmID, nullptr, 0);
    if (x == (void *)-1)
    {
        cerr << "Error attaching shared memory for x: " << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    int wallID = shmget(IPC_PRIVATE, wallSize, IPC_CREAT | 0600);
    if (wallID == -1)
    {
        cerr << "Error creating shared memory for wall: " << strerror(errno) << endl;
        shmdt((void *)x);
        shmctl(shmID, IPC_RMID, nullptr);
        return EXIT_FAILURE;
    }
    volatile int *wall = (volatile int *)shmat(wallID, nullptr, 0);
    if (wall == (void *)-1)
    {
        cerr << "Error attaching shared memory for wall: " << strerror(errno) << endl;
        shmdt((void *)x);
        shmctl(shmID, IPC_RMID, nullptr);
        shmctl(wallID, IPC_RMID, nullptr);
        return EXIT_FAILURE;
    }

    // copy data to shared memory
    for (int i = 0; i < n; i++)
    {
        x[i] = data[i];
    }

    // initialize barrier memory
    wall[0] = 0;

    vector<pid_t> children;
    children.reserve(m);

    for (int i = 0; i < m; i++)
    {
        pid_t pid = fork();
        if (pid == -1)
        {
            cerr << "Error forking process: " << strerror(errno) << endl;

            // kill created children
            for (pid_t child : children)
            {
                kill(child, SIGKILL);
            }

            for (pid_t child : children)
            {
                waitpid(child, nullptr, 0);
            }

            // detach and remove shared memory
            shmdt((void *)x);
            shmdt((void *)wall);

            shmctl(shmID, IPC_RMID, nullptr);
            shmctl(wallID, IPC_RMID, nullptr);

            return EXIT_FAILURE;
        }
        if (pid == 0) // checking if the process is a child process
        {
            hillis_steele_prefix_sum(x, n, m, i, wall);
            _exit(0);
        }

        children.push_back(pid);
    }

    bool waitFailed = false;
    for (pid_t child : children)
    {
        if (waitpid(child, nullptr, 0) == -1)
        {
            cerr << "Error waiting for child process: " << strerror(errno) << endl;
            waitFailed = true;
        }
    }

    if (waitFailed)
    {
        // detach and remove shared memory
        shmdt((void *)x);
        shmdt((void *)wall);

        shmctl(shmID, IPC_RMID, nullptr);
        shmctl(wallID, IPC_RMID, nullptr);

        return EXIT_FAILURE;
    }

    if (!write_output_file(outputFile, x, n))
    {
        cerr << "Error writing to output file: " << outputFile << endl;
        // detach and remove shared memory
        shmdt((void *)x);
        shmdt((void *)wall);

        shmctl(shmID, IPC_RMID, nullptr);
        shmctl(wallID, IPC_RMID, nullptr);

        return EXIT_FAILURE;
    }

    // detach shared memory
    shmdt((void *)x);
    shmdt((void *)wall);

    // remove shared memory segment
    shmctl(shmID, IPC_RMID, nullptr);
    shmctl(wallID, IPC_RMID, nullptr);

    return EXIT_SUCCESS;
}
