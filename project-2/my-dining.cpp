/*
    Kamsi Ozorji & Khoa Bui
    CS 4348
    11/1/2026
    Purpose: To solve Dining Philosophers with n threads, first with one Tournament-Tree lock, then with one Peterson lock per chopstick (Project 2)
*/

#include <atomic>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <exception>

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <iostream>

using namespace std;

// Peterson's lock for exactly two threads, each participant uses a distinct id of 0 or 1
class PetersonLock
{
    atomic<bool> flag[2] = {{false}, {false}};
    atomic<int> turn{0};

public:
    // Blocks until the caller owns the lock
    //  @param id: the caller's id, 0 or 1
    void acquire(int id)
    {
        // announce interest and give the other thread the turn
        flag[id] = true;
        turn = 1 - id;

        // wait while the other thread wants the lock and it is its turn
        while (flag[1 - id] && turn != id)
        {
            this_thread::yield();
        }
    }

    // Gives the lock up
    //  @param id: the caller's id, 0 or 1
    void release(int id)
    {
        flag[id] = false;
    }
};

// Tournament-tree lock for n threads, stored as an array with the root at 1 and the children of node k at 2k and 2k+1
// size is n rounded up to a power of two, thread tid owns leaf size + tid, unused leaves just mean no opponent
class TTLock
{
    int size = 1;
    int levels = 0;
    vector<PetersonLock> locks;

public:
    TTLock(int n)
    {
        // round n up to the next power of two and count the levels
        while (size < n)
        {
            size = size * 2;
            levels++;
        }
        locks = vector<PetersonLock>(size);
    }

    // Locks from the leaf up to the root
    //  @param tid: the thread's number, 0 to n-1
    void acquire(int tid)
    {
        int node = size + tid;

        // win one Peterson lock per level, the side is whether the node is a left or right child
        while (node > 1)
        {
            locks[node / 2].acquire(node % 2);
            node = node / 2;
        }
    }

    // Unlocks from the root down to the leaf
    //  @param tid: the thread's number, 0 to n-1
    void release(int tid)
    {
        // the first node is the child of the root, the last one is the leaf
        for (int i = levels - 1; i >= 0; i--)
        {
            int node = (size + tid) >> i;
            locks[node / 2].release(node % 2);
        }
    }
};

// Prints one event line with a single printf call
//  @param solution: "COARSE" or "FINE"
//  @param id: the philosopher's number
//  @param round: the current round, 0 to 4
//  @param event: THINK_START, THINK_END, EAT_START or EAT_END
void print_event(const char *solution, int id, int round, const char *event)
{
    printf("%s %d %d %s\n", solution, id, round, event);
    fflush(stdout);
}

// Sleeps for a random 1 to 500 ms, using a per-thread generator
void random_delay()
{
    thread_local mt19937 gen(random_device{}());
    uniform_int_distribution<int> dist(1, 500);
    this_thread::sleep_for(chrono::milliseconds(dist(gen)));
}

// Philosopher thread for the coarse solution
//  @param lock: the shared tournament-tree lock
//  @param id: the philosopher's number
void coarse_philosopher(TTLock &lock, int id)
{
    for (int i = 0; i < 5; i++)
    {
        // think
        print_event("COARSE", id, i, "THINK_START");
        random_delay();
        print_event("COARSE", id, i, "THINK_END");

        // eat while holding the lock
        lock.acquire(id);
        print_event("COARSE", id, i, "EAT_START");
        random_delay();
        print_event("COARSE", id, i, "EAT_END");
        lock.release(id);
    }
}

// Philosopher thread for the fine solution, chopstick c is shared by philosophers c (side 0) and c+1 (side 1)
// The last philosopher picks up its chopsticks in reverse order to prevent deadlock
//  @param chopsticks: one Peterson lock per chopstick
//  @param n: number of philosophers
//  @param id: the philosopher's number
void fine_philosopher(vector<PetersonLock> &chopsticks, int n, int id)
{
    // left chopstick first and right second, the last philosopher reverses them
    int first = (id + n - 1) % n, second = id;
    if (id == n - 1)
    {
        first = id;
        second = (id + n - 1) % n;
    }

    // the side is 0 when the chopstick's number equals the philosopher's, 1 otherwise
    int firstSide = (first == id) ? 0 : 1;
    int secondSide = (second == id) ? 0 : 1;

    for (int i = 0; i < 5; i++)
    {
        // think
        print_event("FINE", id, i, "THINK_START");
        random_delay();
        print_event("FINE", id, i, "THINK_END");

        // pick up both chopsticks and eat
        chopsticks[first].acquire(firstSide);
        chopsticks[second].acquire(secondSide);
        print_event("FINE", id, i, "EAT_START");
        random_delay();
        print_event("FINE", id, i, "EAT_END");

        // put them down in reverse order
        chopsticks[second].release(secondSide);
        chopsticks[first].release(firstSide);
    }
}

// Runs the coarse-grained solution
//  @param n: number of philosophers
//  @return true if all threads were created, false otherwise
bool coarse_grained(int n)
{
    TTLock lock(n);
    vector<thread> threads;
    bool ok = true;

    // create the philosopher threads
    try
    {
        for (int i = 0; i < n; i++)
        {
            threads.emplace_back(coarse_philosopher, ref(lock), i);
        }
    }
    catch (const exception &e)
    {
        cerr << "Error: could not create philosopher thread: " << e.what() << endl;
        ok = false;
    }

    // join every thread that was started
    for (int i = 0; i < static_cast<int>(threads.size()); i++)
    {
        threads[i].join();
    }
    return ok;
}

// Runs the fine-grained solution
//  @param n: number of philosophers
//  @return true if all threads were created, false otherwise
bool fine_grained(int n)
{
    vector<PetersonLock> chopsticks(n);
    vector<thread> threads;
    bool ok = true;

    // create the philosopher threads
    try
    {
        for (int i = 0; i < n; i++)
        {
            threads.emplace_back(fine_philosopher, ref(chopsticks), n, i);
        }
    }
    catch (const exception &e)
    {
        cerr << "Error: could not create philosopher thread: " << e.what() << endl;
        ok = false;
    }

    // join every thread that was started
    for (int i = 0; i < static_cast<int>(threads.size()); i++)
    {
        threads[i].join();
    }
    return ok;
}

// Parses and validates the command line
//  @param argc: number of command line arguments
//  @param argv: array of command line arguments
//  @param n: reference to store the number of philosophers
//  @return true if the arguments are valid, false otherwise
bool parse_arguments(int argc, char *argv[], int &n)
{
    // check the number of arguments
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <n>" << endl;
        return false;
    }

    // n must be an integer
    char *end;
    errno = 0;
    long n_long = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0')
    {
        cerr << "Error: n must be an integer: " << argv[1] << endl;
        return false;
    }

    // n must fit, tree size doubling must not overflow an int
    if (errno == ERANGE || n_long > INT_MAX / 2)
    {
        cerr << "Error: n is out of range: " << argv[1] << endl;
        return false;
    }
    if (n_long < 2)
    {
        cerr << "Error: n must be at least 2" << endl;
        return false;
    }
    n = static_cast<int>(n_long);
    return true;
}

// Runs the main program
//  @param argc: number of command line arguments
//  @param argv: array of command line arguments
//  @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise
int main(int argc, char *argv[])
{
    int n;
    if (!parse_arguments(argc, argv, n))
    {
        return EXIT_FAILURE;
    }

    try
    {
        // coarse solution first
        puts("BEGIN COARSE");
        bool coarseOk = coarse_grained(n);
        puts("END COARSE");
        if (!coarseOk)
        {
            return EXIT_FAILURE;
        }

        // then the fine solution
        puts("BEGIN FINE");
        bool fineOk = fine_grained(n);
        puts("END FINE");
        if (!fineOk)
        {
            return EXIT_FAILURE;
        }
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}