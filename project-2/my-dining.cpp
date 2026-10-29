#include <iostream>
#include <fstream>

#include <atomic>
#include <thread>
#include <numeric>
#include <cmath>
#include <vector>

using namespace std;

// ===== Peterson lock (A) =====
// Two-thread lock. id must be 0 or 1, distinct for the two participants.

/*
    Pseudocode from class
        Boolean flag[2] := {false, false};
        Integer turn;

        Acquire:
            flag[myId] := true;
            turn := 1 - myId;
            while(flag[1 - myId] and  (turn != myId)) do
                begin
                    ;   // do nothing (spin-wait)
                end while


        Critical Section

        Release:
            flag[myId] := false;
*/
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
            // do nothing (spin-wait)
        }
    }

    void release(int id)
    {
        flag[id] = false;
    }
};

// ===== Tournament-tree lock (B) =====
// n-thread lock built from PetersonLocks. tid is the thread's number, 0 to n-1.

/*
    TTLock(n):
    size := 1
    while size < n: size := size * 2
    levels := log2(size)
    locks[1 .. size-1] := array of PetersonLock

    acquire(tid):
        node := size + tid
        while node > 1:
            locks[node / 2].acquire(node % 2)    // my id = which child I am
            node := node / 2                     // move up
        // holding the root: critical section

    release(tid):
        // release in REVERSE order: root first, leaf level last
        for level := levels-1 down to 0:
            node := (size + tid) >> level        // my ancestor at that level
            locks[node / 2].release(node % 2)
*/
class TTLock
{
    int size = 1;
    double levels;
    vector<PetersonLock> locks;

public:
    TTLock(int n)
    {
        while (size <= n)
        {
            size = size * 2;
        }
        levels = log2(size);
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
        int node = 0;
        for (int level = levels - 1; level >= 0; level--)
        {
            node = (size + tid) >> level;
            locks[node / 2].release(node % 2);
        }
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
