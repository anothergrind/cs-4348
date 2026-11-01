#include <iostream>
#include <fstream>

#include <atomic>
#include <thread>
#include <numeric>
#include <cmath>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <chrono>
#include <cerrno>
#include <climits>
#include <exception>

using namespace std;

// Peterson lock
// Two-thread lock. id must be 0 or 1, different for the two participants.
class PetersonLock
{
    atomic<bool> flag[2] = {false, false};
    atomic<int> turn{0};

public:
    void acquire(int id)
    {
        flag[id] = true;
        turn = 1 - id;
        while (flag[1 - id] && (turn != id))
        {
            this_thread::yield();
        }
    }

    void release(int id)
    {
        flag[id] = false;
    }
};

//  Tournament-tree lock
// n-thread lock built from PetersonLocks. tid is the thread's number, 0 to n-1.
class TTLock
{
    int size = 1;
    int levels = 0;
    vector<PetersonLock> locks;

public:
    TTLock(int n)
    {
        while (size < n)
        {
            size = size * 2;
            levels++;
        }
        locks = vector<PetersonLock>(size);
    }

    void acquire(int tid)
    {
        int node = size + tid;
        while (node > 1)
        {
            locks[node / 2].acquire(node % 2);
            node = node / 2;
        }
    }

    void release(int tid)
    {
        for (int level = levels - 1; level >= 0; level--)
        {
            int node = (size + tid) >> level;
            locks[node / 2].release(node % 2);
        }
    }
};

//  Philosopher loop / output
// Prints one event line, e.g. "COARSE 2 0 THINK_START", in a single output call.
void print_event(const char *solution, int id, int round, const char *event)
{
    printf("%s %d %d %s\n", solution, id, round, event);
    fflush(stdout);
}

// sleeps for a random 1-500 ms; each thread has its own generator.
void random_delay()
{
    thread_local mt19937 gen(random_device{}());
    this_thread::sleep_for(chrono::milliseconds(uniform_int_distribution<int>(1, 500)(gen)));
}

// Coarse solution
// One tournament-tree lock protects eating (both chopsticks) as a unit.
// Returns false if a thread could not be created; threads already started are still joined.
bool coarse_grained(int n)
{
    TTLock lock(n);
    vector<thread> ts;
    bool ok = true;
    try
    {
        for (int id = 0; id < n; id++)
        {
            ts.emplace_back([&lock, id]()
                            {
            for (int round = 0; round < 5; round++)
            {
                print_event("COARSE", id, round, "THINK_START");
                random_delay();
                print_event("COARSE", id, round, "THINK_END");
                lock.acquire(id);
                print_event("COARSE", id, round, "EAT_START");
                random_delay();
                print_event("COARSE", id, round, "EAT_END");
                lock.release(id);
            } });
        }
    }
    catch (const exception &e)
    {
        fprintf(stderr, "COARSE: could not create philosopher thread: %s\n", e.what());
        ok = false;
    }
    for (thread &t : ts)
        t.join();
    return ok;
}

// Fine solution
// One PetersonLock per chopstick. Chopstick c is shared by philosophers c (side 0) and c+1 (side 1).
// The last philosopher takes its right chopstick first, which breaks the circular wait.
bool fine_grained(int n)
{
    vector<PetersonLock> chopsticks(n);
    vector<thread> ts;
    bool ok = true;
    try
    {
        for (int id = 0; id < n; id++)
        {
            ts.emplace_back([&chopsticks, n, id]()
                            {
            int left = (id + n - 1) % n, right = id;
            int first = left, second = right;
            if (id == n - 1)
            {
                first = right;
                second = left;
            }
            // side is 0 if this philosopher's id equals the chopstick's index, else 1
            int firstSide = 1, secondSide = 1;
            if (first == id)
            {
                firstSide = 0;
            }
            if (second == id)
            {
                secondSide = 0;
            }
            for (int round = 0; round < 5; round++)
            {
                print_event("FINE", id, round, "THINK_START");
                random_delay();
                print_event("FINE", id, round, "THINK_END");
                chopsticks[first].acquire(firstSide);
                chopsticks[second].acquire(secondSide);
                print_event("FINE", id, round, "EAT_START");
                random_delay();
                print_event("FINE", id, round, "EAT_END");
                chopsticks[second].release(secondSide);
                chopsticks[first].release(firstSide);
            } });
        }
    }
    catch (const exception &e)
    {
        fprintf(stderr, "FINE: could not create philosopher thread: %s\n", e.what());
        ok = false;
    }
    for (thread &t : ts)
        t.join();
    return ok;
}

// main + validation
// usage: ./my-dining n   (n >= 2)
int main(int argc, char *argv[])
{
    char *end = nullptr;
    long n = 0;
    errno = 0;
    if (argc == 2)
    {
        n = strtol(argv[1], &end, 10);
    }
    if (argc != 2 || end == argv[1] || *end != '\0' || errno == ERANGE || n < 2)
    {
        fprintf(stderr, "Usage: %s n   (n >= 2)\n", argv[0]);
        return 1;
    }
    try
    {
        puts("BEGIN COARSE");
        bool coarseOk = coarse_grained(n);
        puts("END COARSE");
        if (!coarseOk)
        {
            return 1;
        }
        puts("BEGIN FINE");
        bool fineOk = fine_grained(n);
        puts("END FINE");
        if (!fineOk)
        {
            return 1;
        }
    }
    catch (const exception &e)
    {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    return 0;
}
