// Use shmdt(), exit(), fork(), waitpid(), shmget(), shmat(), shmctl()
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

using namespace std;

// Parse a strictly positive, fully numeric argument within int range.
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

// Detach from and remove the shared-memory segment, reporting any failure.
bool cleanup_shared_memory(int shmid, void* base)
{
    bool success = true;
    if (shmdt(base) == -1)
    {
        cerr << "Error: shmdt failed: " << strerror(errno) << endl;
        success = false;
    }
    if (shmctl(shmid, IPC_RMID, nullptr) == -1)
    {
        cerr << "Error: shmctl failed: " << strerror(errno) << endl;
        success = false;
    }
    return success;
}

// Terminate the given workers and wait for each of them.
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
            if (errno == EINTR) continue;
            if (errno != ECHILD) {
                cerr << "Error: waitpid failed while cleaning up worker "
                     << pid << ": " << strerror(errno) << endl;
            }
            break;
        }
    }
}

// Compute one worker's portion [lo, hi) of the next prefix-sum array.
void compute_range(const long long* source, long long* destination,
                   long long offset, int lo, int hi)
{
    for (int i = lo; i < hi; i++) {
        // If i is before the offset, no earlier element exists to add.
        if (i < offset) {
            destination[i] = source[i];
        } else {
            // Otherwise combine this value with the value offset positions back.
            destination[i] = source[i - offset] + source[i];
        }
    }
}

// Reusable barrier: each worker records the iteration it has reached in its
// own slot, then waits until every worker has recorded at least that value.
// (Original non-reusable version needed a separate m-entry row per iteration.)
void arriveAndWait(int id, long long generation, int m,
                   volatile long long* wall)
{
    wall[id] = generation;
    for (int j = 0; j < m; j++) {
        while (wall[j] < generation) {
            // Busy wait until every worker reaches this generation.
        }
    }
}

// Each worker computes its range, waits at the barrier, swaps the two working
// arrays, then detaches from shared memory and exits.
void run_worker(int id, int n, int m, long long* first,
                long long* second, volatile long long* wall)
{
    int base_chunk = n / m;
    int remainder = n % m;

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
    for (long long offset = 1; offset < n; offset *= 2) {
        compute_range(source, destination, offset, lo, hi);

        // Wait for all workers before starting the next iteration.
        arriveAndWait(id, offset, m, wall);

        long long* temporary = source;
        source = destination;
        destination = temporary;
    }

    if (shmdt((void*)first) == -1) {
        cerr << "Error: Worker " << id
             << " failed to detach shared memory: "
             << strerror(errno) << endl;
        exit(1);
    }
    exit(0);
}

int main(int argc, char* argv[])
{
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
    string input_file = argv[3];
    string output_file = argv[4];

    if (m > n) {
        cerr << "Error: m must be <= n." << endl;
        return 1;
    }

    // Read and validate input before allocating shared memory or forking.
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

    // Number of arrays produced: x[0] plus one per Hillis-Steele iteration.
    int rows = 1;
    for (long long offset = 1; offset < n; offset *= 2) {
        rows++;
    }

    /*
     Without shared memory, each forked process gets its own private copy of
     memory, so writes in one process are invisible to the others.
     With shmget/shmat, all processes map the same physical memory, so a write
     by one worker is seen by all.

     Storing every intermediate row would take O(n log n) space. Alternating
     between two arrays reduces the prefix-sum storage to O(n).
    */
    size_t size_x = (size_t)n * 2;   // element count: two arrays of n long longs
    size_t size_wall = (size_t)m;    // one long long arrival slot per worker
    size_t size_bytes = size_x * sizeof(long long)
                      + size_wall * sizeof(long long);

    int shmid = shmget(IPC_PRIVATE, size_bytes, IPC_CREAT | 0666);
    if (shmid == -1) {
        cerr << "Error: Could not create shared memory segment. " << strerror(errno) << endl;
        return 1;
    }

    void* base = shmat(shmid, nullptr, 0);
    if (base == (void*)-1) {
        cerr << "Error: Could not attach shared memory segment. " << strerror(errno) << endl;
        if (shmctl(shmid, IPC_RMID, nullptr) == -1)
            cerr << "Error: shmctl failed: " << strerror(errno) << endl;
        return 1;
    }

    long long* first = (long long*)base;
    long long* second = first + n;
    // size_x counts long longs (2n), so first + size_x points just past both
    // arrays, which is where the barrier slots begin.
    volatile long long* wall =
        (volatile long long*)(first + size_x);

    for (size_t i = 0; i < size_wall; i++) {
        wall[i] = 0;                 // initialize barrier slots to 0
    }
    for (int i = 0; i < n; i++) {
        first[i] = input[i];         // initialize the first working array
    }

    vector<pid_t> workers;
    workers.reserve(m);
    for (int id = 0; id < m; id++) {
        pid_t pid = fork();
        if (pid == -1) {
            cerr << "Error: Could not fork process " << id << ". " << strerror(errno) << endl;
            stop_workers(workers);
            cleanup_shared_memory(shmid, base);
            return 1;
        }
        if (pid == 0) {
            run_worker(id, n, m, first, second, wall);
        }
        workers.push_back(pid);
    }

    // Wait for all m workers.
    bool worker_error = false;
    vector<bool> reaped(workers.size(), false);
    int remaining = static_cast<int>(workers.size());
    while (remaining > 0) {
        int status;
        pid_t pid;
        while (true) {
            pid = waitpid(-1, &status, 0);
            if (pid != -1) break;
            if (errno == EINTR) continue;
            cerr << "Error: waitpid failed: " << strerror(errno) << endl;
            worker_error = true;
            break;
        }
        if (pid == -1) break;

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
            break;
        }
    }

    if (worker_error) {
        vector<pid_t> active_workers;
        for (size_t i = 0; i < workers.size(); i++) {
            if (!reaped[i]) active_workers.push_back(workers[i]);
        }
        stop_workers(active_workers);
        cleanup_shared_memory(shmid, base);
        return 1;
    }

    // The final result is in the array selected after the last swap.
    ofstream outfile(output_file);
    if (!outfile) {
        cerr << "Error: Could not open output file " << output_file << endl;
        cleanup_shared_memory(shmid, base);
        return 1;
    }
    long long* result = ((rows - 1) % 2 == 0) ? first : second;
    for (int i = 0; i < n; i++) {
        outfile << result[i] << (i + 1 < n ? " " : "\n");
    }
    outfile.close();
    if (outfile.fail()) {
        cerr << "Error: Failed while writing output file " << output_file << endl;
        cleanup_shared_memory(shmid, base);
        return 1;
    }

    return cleanup_shared_memory(shmid, base) ? 0 : 1;
}