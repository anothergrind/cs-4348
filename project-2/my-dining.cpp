#include <iostream>
#include <fstream>

#include <atomic>
#include <thread>
#include <numeric>

using namespace std;

// ===== Peterson lock (A) =====
// Two-thread lock. id must be 0 or 1, distinct for the two participants.
class PetersonLock
{
public:
    void acquire(int id)
    {
        // TODO
    }

    void release(int id)
    {
        // TODO
    }
};

// ===== Tournament-tree lock (B) =====
// n-thread lock built from PetersonLocks. tid is the thread's number, 0 to n-1.
class TTLock
{
public:
    TTLock(int n)
    {
        // TODO
    }

    void acquire(int tid)
    {
        // TODO
    }

    void release(int tid)
    {
        // TODO
    }
};

// ===== Philosopher loop / output / RNG (B) =====
// Prints one event line, e.g. "COARSE 2 0 THINK_START", in a single output call.
void print_event(const char *solution, int id, int round, const char *event)
{
    // TODO
}

// Sleeps for a random 1-500 ms; must not race with other threads.
void random_delay()
{
    // TODO
}

// ===== Coarse solution (B) =====
void coarse_grained(int n)
{
    // TODO
}

// ===== Fine solution (A) =====
void fine_grained(int n)
{
    // TODO
}

// ===== main + validation (A) =====
// usage: ./my-dining n   (n >= 2)
int main(int argc, char *argv[])
{
    // TODO
    return 0;
}
