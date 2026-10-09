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
#include <sstream>
#include <cstdint>
#include <climits>

using namespace std;

// Reads integers from an input file, handles spaces, tabs, and commas
//  @param filename: the name of the input file to read from.
//  @param count: the number of valid integers required.
//  @return A vector with integers from the file

vector<int64_t> read_file(string filename, int count)
{
    vector<int64_t> data;
    ifstream in(filename);
    if (!in)
    {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    while (data.size() < static_cast<size_t>(count) && getline(in, line))
    {
        for (char &c : line)
        {
            if (c == ',' || c == '\t')
            {
                c = ' ';
            }
        }
        istringstream iss(line);
        string token;
        while (data.size() < static_cast<size_t>(count) && iss >> token)
        {
            char *end = nullptr;
            errno = 0;
            long long value = strtoll(token.c_str(), &end, 10);
            if (errno == ERANGE || end == token.c_str() || *end != '\0')
            {
                cerr << "Error: invalid integer in input file: " << token << endl;
                exit(EXIT_FAILURE);
            }
            data.push_back(static_cast<int64_t>(value));
        }
    }
    if (in.bad())
    {
        cerr << "Error reading input file: " << filename << endl;
        exit(EXIT_FAILURE);
    }
    return data;
}

// Calculates number of prefix-sum phases needed, log2(n) rounded up
//  @param n: number of elements in the array
//  @return The number of phases needed

int num_phases(int n)
{
    int phases = 0;
    for (int64_t offset = 1; offset < n; offset <<= 1)
        ++phases;
    return phases;
}

// Writes the results to an output file
//  @param filename: the name of the output file to write to.
//  @param x: pointer to shared memory array that contains the results
//  @param n: number of elements in the array
//  @return true if successful, false otherwise

bool write_output_file(string &filename, volatile int64_t *x, int n)
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
    out.close();
    if (!out)
    {
        cerr << "Error writing output file: " << filename << endl;
        return false;
    }
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
    errno = 0;
    long long n_long = strtoll(argv[1], &end, 10);
    if (errno == ERANGE || n_long > INT_MAX || n_long < INT_MIN)
    {
        cerr << "Error: n is out of range: " << argv[1] << endl;
        return false;
    }
    if (end == argv[1] || *end != '\0')
    {
        cerr << "Error: junk after n: " << argv[1] << endl;
        return false;
    }
    n = static_cast<int>(n_long);

    // parse m
    errno = 0;
    long long m_long = strtoll(argv[2], &end, 10);
    if (errno == ERANGE || m_long > INT_MAX || m_long < INT_MIN)
    {
        cerr << "Error: m is out of range: " << argv[2] << endl;
        return false;
    }
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
//  @param wall: pointer to the shared memory used for synchronization, wall[0] is the arrival counter and wall[1] is the abort flag

void reusable_barrier(int id, int phase, int m, volatile int *wall)
{
    while (wall[0] != (phase - 1) * m + id && wall[1] == 0)
    {
        // wait until its process's turn
    }
    if (wall[1] != 0)
        return;
    wall[0] = (phase - 1) * m + id + 1;
    while (wall[0] < phase * m && wall[1] == 0)
    {
        // wait until every process has arrived
    }
}

// Run's Hillis and Steele concurrent prefix-sum algorithm, maintains the time complexity O(n log n / m + m log n)
//  @param x: pointer to shared memory array that contains the input data and will hold the results
//  @param n: number of elements in the array
//  @param m: number of processes (workers)
//  @param id: the process ID (0 to m-1)
//  @param wall: pointer to the shared memory used for synchronization, wall[0] is the arrival counter and wall[1] is the abort flag

void hillis_steele_prefix_sum(volatile int64_t *x, int n, int m, int id, volatile int *wall)
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
            int offset = 1 << (p - 1);
            if (i < offset)
            {
                x[(p % 2) * n + i] = x[((p - 1) % 2) * n + i];
            }
            else
            {
                x[(p % 2) * n + i] = x[((p - 1) % 2) * n + (i - offset)] + x[((p - 1) % 2) * n + i];
            }
        }
        reusable_barrier(id, p, m, wall);
    }
}

// Detaches and removes both shared memory segments, reporting any cleanup failures
//  @param x: pointer to shared memory array that contains the input data and results
//  @param wall: pointer to the shared memory used for synchronization
//  @param shmID: the shared memory ID for x
//  @param wallID: the shared memory ID for wall
//  @return true if successful, false otherwise

bool cleanup_shared_memory(volatile int64_t *x, volatile int *wall, int shmID, int wallID)
{
    bool ok = true;
    if (shmdt((void *)x) == -1)
    {
        cerr << "Error detaching x shared memory: " << strerror(errno) << endl;
        ok = false;
    }
    if (shmdt((void *)wall) == -1)
    {
        cerr << "Error detaching barrier shared memory: " << strerror(errno) << endl;
        ok = false;
    }
    if (shmctl(shmID, IPC_RMID, nullptr) == -1)
    {
        cerr << "Error removing x shared memory: " << strerror(errno) << endl;
        ok = false;
    }
    if (shmctl(wallID, IPC_RMID, nullptr) == -1)
    {
        cerr << "Error removing barrier shared memory: " << strerror(errno) << endl;
        ok = false;
    }
    return ok;
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
    vector<int64_t> data = read_file(inputFile, n);
    if (data.size() < static_cast<size_t>(n))
    {
        cerr << "Error: file contains fewer than n integers (found " << data.size() << ", required at least " << n << ")" << endl;
        return EXIT_FAILURE;
    }

    cout << "Successfully read " << data.size() << " integers from " << inputFile << endl;

    size_t size = 2ULL * static_cast<size_t>(n) * sizeof(int64_t);
    size_t wallSize = 2 * sizeof(int);

    int shmID = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
    if (shmID == -1)
    {
        cerr << "Error creating shared memory for x: " << strerror(errno) << endl;
        return EXIT_FAILURE;
    }
    volatile int64_t *x = (volatile int64_t *)shmat(shmID, nullptr, 0);
    if (x == (void *)-1)
    {
        cerr << "Error attaching shared memory for x: " << strerror(errno) << endl;
        shmctl(shmID, IPC_RMID, nullptr);
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
    wall[1] = 0;

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
            cleanup_shared_memory(x, wall, shmID, wallID);

            return EXIT_FAILURE;
        }
        if (pid == 0) // checking if the process is a child process
        {
            hillis_steele_prefix_sum(x, n, m, i, wall);

            // Detach shared memory segments before exit
            bool detach_ok = true;
            if (shmdt((void *)x) == -1)
            {
                cerr << "Error detaching x shared memory in child: " << strerror(errno) << endl;
                detach_ok = false;
            }
            if (shmdt((void *)wall) == -1)
            {
                cerr << "Error detaching barrier shared memory in child: " << strerror(errno) << endl;
                detach_ok = false;
            }
            _exit(detach_ok ? 0 : EXIT_FAILURE);
        }

        children.push_back(pid);
    }

    bool waitFailed = false;
    bool abnormal_msg_printed = false;
    vector<bool> reaped(children.size(), false);
    size_t remaining = children.size();
    while (remaining > 0)
    {
        bool progress = false;
        for (size_t i = 0; i < children.size(); ++i)
        {
            if (reaped[i])
                continue;

            int status = 0;
            pid_t result = waitpid(children[i], &status, WNOHANG);
            if (result == -1)
            {
                if (errno == EINTR)
                    continue;
                cerr << "Error waiting for child process: " << strerror(errno) << endl;
                waitFailed = true;
                wall[1] = 1;
                kill(children[i], SIGKILL); // Ensure the failed-wait child is killed
                reaped[i] = true;
                --remaining;
                progress = true;
                for (size_t j = 0; j < children.size(); ++j)
                {
                    if (!reaped[j])
                        kill(children[j], SIGKILL);
                }
                continue;
            }
            if (result == 0)
                continue;

            reaped[i] = true;
            --remaining;
            progress = true;
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            {
                if (!abnormal_msg_printed)
                {
                    cerr << "Error: worker process terminated abnormally" << endl;
                    abnormal_msg_printed = true;
                }
                waitFailed = true;
                wall[1] = 1;
                for (size_t j = 0; j < children.size(); ++j)
                {
                    if (!reaped[j])
                        kill(children[j], SIGKILL);
                }
            }
        }
        if (!progress && remaining > 0)
            usleep(1000);
    }

    if (waitFailed)
    {
        cleanup_shared_memory(x, wall, shmID, wallID);
        return EXIT_FAILURE;
    }

    if (!write_output_file(outputFile, x, n))
    {
        cerr << "Error writing to output file: " << outputFile << endl;
        cleanup_shared_memory(x, wall, shmID, wallID);
        return EXIT_FAILURE;
    }

    return cleanup_shared_memory(x, wall, shmID, wallID) ? EXIT_SUCCESS : EXIT_FAILURE;
}
