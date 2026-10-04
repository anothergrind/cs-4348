

// Use shmdt(), exit(), fork(), waitpid(), shmget(), shmat(), shmctl()
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// Standard C++ headers
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <vector>


using namespace std;

bool parse_positive_int(const char* text, int& value)
{
    char* end = nullptr;
    errno = 0;
    long parsed = strtol(text, &end, 10);

    if (errno == ERANGE || end == text || *end != '\0' ||
        parsed <= 0 || parsed > numeric_limits<int>::max()) {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

void stop_workers(const vector<pid_t>& workers)
{
    for (pid_t pid : workers) {
        if (kill(pid, SIGTERM) == -1 && errno != ESRCH) {
            cerr << "Error: Could not terminate worker " << pid << ": "
                 << strerror(errno) << endl;
        }
    }

    for (pid_t pid : workers) {
        while (waitpid(pid, nullptr, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno != ECHILD) {
                cerr << "Error: waitpid failed while cleaning up worker "
                     << pid << ": " << strerror(errno) << endl;
            }
            break;
        }
    }
}


// Compute one worker's portion of the next prefix-sum array.
void compute_range(const long long* source, long long* destination,
                   int offset, int lo, int hi)
{
    for (int i = lo; i < hi; i++) {
        if (i < offset)
            // Case 1: No element for offset to steps back
            destination[i] = source[i];
        else
            // Case 2: Combine current value with offset value
            destination[i] = source[i - offset] + source[i];
    }
}

/*
Original non-reusable barrier:

void arriveAndWait(int id, int row, int m, volatile int* wall) {
    wall[row * m + id] = 1;
    for (int j = 0; j < m; j++) {
        while (wall[row * m + j] == 0) {
        }
    }
}

It required a separate m-entry barrier row for every iteration. The
implementation below reuses one barrier using a generation number.
*/

// Reusable barrier: each worker records the iteration it has reached in its
// own slot, then waits until every worker has recorded at least that value.
void arriveAndWait(int id, int generation, int m, volatile int* wall) {
    wall[id] = generation;

    for (int j = 0; j < m; j++) {
        while (wall[j] < generation) {
            // Busy wait until every worker reaches this generation.
        }
    }
}

// Each worker computes its assigned range, swaps the two working arrays after
// every barrier, then detaches from shared memory and exits.
void run_worker(int id, int n, int m, long long* first,
                long long* second, volatile int* wall) {
    // Calculate workload [lo, hi)
    int base_chunk = n / m;
    int remainder = n% m;
    
    int lo, hi;
    if (id < remainder) {
        lo = id * (base_chunk + 1);
        hi = lo + base_chunk + 1;
    } else {
        lo = id * base_chunk + remainder;
        hi = lo + base_chunk;
    }

    long long* source = first;
    long long* destination = second;
    for (int offset = 1; offset < n; offset *= 2) {
        compute_range(source, destination, offset, lo, hi);

        // Wait for all worker processes before moving to the next row.
        arriveAndWait(id, offset, m, wall);

        long long* temporary = source;
        source = destination;
        destination = temporary;
    }

    // Detach from shared memory when finished
    shmdt((void*)first);
    exit(0); // Exit child process
}

int main(int argc, char* argv[])
{
    // Check for correct number of 4 arguments + 1 program name
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <n> <m> <input_file> <output_file>" << endl;
        return 1;
    }

    int n;
    int m;
    if (!parse_positive_int(argv[1], n) || !parse_positive_int(argv[2], m)) {
        cerr << "Error: n and m must be positive integers." << endl;
        return 1;
    }
    string input_file = argv[3]; // Input file name
    string output_file = argv[4]; // Output file name

    if (m > n) {
        cerr << "Error: n and m must be positive integers, and m must be <= n." << endl;
        return 1;
    }

    // Read input from file
    ifstream infile(input_file);
    if (!infile) {
        cerr << "Error: Could not open input file " << input_file << endl;
        return 1;
    }
    vector<long long> input(n);
    for (int i = 0; i < n; i++) {
        if (!(infile >> input[i])) {
            cerr << "Error: Could not read element " << i << " from input file." << endl;
            return 1;
        }
    }
    infile.close();

    // Calculate the number of Hillis-Steele iterations.
    int rows = 1;
    for (int offset = 1; offset < n; offset *= 2) {
        rows++;
    }

    /*
     Without shared memory (Standard fork):
        Porcess 1 x[0] = 42 -> Write to process 1 private ram
        process 2 x[0]      -> still read 0 and wait for process 1 to finish
     With shared memory (shgmet/shmat):
        Process 1 x[0] = 42 +  Process 2 x[0] -> Shared ram blocked -> value become for everyone
    */

    /*
    Original matrix-based storage:

        size_t size_x = (size_t)rows * n;
        long long* x = (long long*)base;
        x[row * n + i] = ...

    That approach stores every intermediate row and uses O(n log n) space.
    
    The bonus implementation below alternates between two arrays, reducing
    the prefix-sum storage to O(n).
    */
    size_t size_x = (size_t)n * 2; // Two working arrays of n long longs.
    // One reusable arrival value per worker.
    size_t size_wall = (size_t)m; // ints
    size_t size_x_bytes = size_x * sizeof(long long) + size_wall * sizeof(int);

    int shmid = shmget(IPC_PRIVATE, size_x_bytes, IPC_CREAT | 0666);
    if (shmid == -1) {
        cerr << "Error: Could not create shared memory segment for x. " << strerror(errno) << endl;
        return 1;
    }

    void* base = shmat(shmid, nullptr, 0);
    if (base == (void*)-1) {
        cerr << "Error: Could not attach shared memory segment for x. " << strerror(errno) << endl;
        shmctl(shmid, IPC_RMID, nullptr); // Clean up
        return 1;
    }

    long long* first = (long long*)base;
    long long* second = first + n;
    volatile int* wall = (volatile int*)(first + size_x);

    for (size_t i = 0; i < size_wall; i++) {
        wall[i] = 0; // Initialize x to 0
    }
    for (int i = 0; i < n; i++) {
        first[i] = input[i]; // Initialize the first working array.
    }

    // Fork m worker processes
    vector<pid_t> workers;
    workers.reserve(m);
    for (int id = 0; id < m; id++) {
        pid_t pid = fork();
        if (pid == -1) {
            cerr << "Error: Could not fork process " << id << ". " << strerror(errno) << endl;
            stop_workers(workers);
            shmdt((void*)base);
            shmctl(shmid, IPC_RMID, nullptr);
            return 1;
        }
        if (pid == 0) {
            // Child process
            run_worker(id, n, m, first, second, wall);
        }
        workers.push_back(pid);
    }
    // Wait for all m workers
    bool worker_error = false;
    vector<bool> reaped(workers.size(), false);
    int remaining = static_cast<int>(workers.size());
    while (remaining > 0) {
        int status;
        pid_t pid;
        while (true) {
            pid = waitpid(-1, &status, 0);
            if (pid != -1) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            cerr << "Error: waitpid failed: " << strerror(errno) << endl;
            worker_error = true;
            break;
        }
        if (pid == -1) {
            break;
        }

        size_t worker_index = 0;
        while (worker_index < workers.size() && workers[worker_index] != pid) {
            worker_index++;
        }
        if (worker_index < workers.size()) {
            reaped[worker_index] = true;
        }
        remaining--;

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            if (WIFSIGNALED(status)) {
                cerr << "Error: Worker " << pid << " terminated by signal "
                     << WTERMSIG(status) << "." << endl;
            } else {
                cerr << "Error: Worker " << pid << " exited with status "
                     << WEXITSTATUS(status) << "." << endl;
            }
            worker_error = true;
            vector<pid_t> active_workers;
            for (size_t i = 0; i < workers.size(); i++) {
                if (!reaped[i]) {
                    active_workers.push_back(workers[i]);
                }
            }
            stop_workers(active_workers);
            remaining = 0;
            break;
        }
    }
    if (worker_error && remaining > 0) {
        vector<pid_t> active_workers;
        for (size_t i = 0; i < workers.size(); i++) {
            if (!reaped[i]) {
                active_workers.push_back(workers[i]);
            }
        }
        stop_workers(active_workers);
    }

    if (worker_error) {
        if (shmdt(base) == -1) {
            cerr << "Error: shmdt failed: " << strerror(errno) << endl;
        }
        if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
            cerr << "Error: shmctl failed: " << strerror(errno) << endl;
        }
        return 1;
    }

    // The final result is in the array selected after the last swap.
    ofstream outfile(output_file);
    if (!outfile) {
        cerr << "Error: Could not open output file " << output_file << endl;
        shmdt(base);
        shmctl(shmid, IPC_RMID, nullptr);
        return 1;
    }
    long long* result = ((rows - 1) % 2 == 0) ? first : second;
    for (int i = 0; i < n; i++) {
        outfile << result[i] << (i + 1 < n ? " " : "\n");
    }
    outfile.close();

    // Cleanup
    if (shmdt(base) == -1) cerr << "Error: shmdt failed: " << strerror(errno) << endl;
    if (shmctl(shmid, IPC_RMID, nullptr) == -1) cerr << "Error: shmctl failed: " << strerror(errno) << endl;
    return 0;
}   
