#include <iostream>
#include <fstream>

#include <atomic>
#include <thread>
#include <numeric>

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
