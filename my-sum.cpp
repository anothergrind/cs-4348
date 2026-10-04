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

// C standard library (argument parsing + error reporting)
#include <cstdlib>
#include <cerrno>
#include <cstring>

// C++ standard library (file I/O & math)
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>

using namespace std;

// reading from the input file
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
        // process each line of input
        istringstream iss(line);
        // parse the line into an array of integers
        int val;
        // store the integers into the vector
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
int hillis_steele_prefix_sum(int x[], int n, int m)
{
    int maxL1 = num_phases(n);
    for (int p = 1; p <= maxL1; p++)
    {
        for (int i = 0; i <= n - 1; i++)
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
    }
    return 0; // change to actual thing
}

// implemented a non-reusable barrier algorithm
void non_reusable_barrier(int wall[], int m)
{
    while (true)
    {
        bool all_done = true;
        for (int j = 1; j <= m; j++)
        {
            if (wall[j] == 0)
            {
                all_done = false;
                break;
            }
        }
        if (all_done)
        {
            break;
        }
    }
}
// TODO: Ensure your program uses O(n log_2 n) space and run in O( ([n log_2 n] / m) + m log_2 n) time.

/*
BONUS:

5% by modifying barrer algorithm to make barrier object reusable while preserving original structure of algorithm
10% if your reusable barrier only uses O(1) space and program uses only O(n) space

*/

int main(int argc, char *argv[])
{
    int n, m;
    string inputFile, outputFile;

    // 1. Parse and validate arguments
    if (!parse_arguments(argc, argv, n, m, inputFile, outputFile))
    {
        return EXIT_FAILURE;
    }

    // Read input file
    vector<int> data = read_file(inputFile);
    if (data.size() != static_cast<size_t>(n))
    {
        cerr << "Error: number of integers read from file does not match n (" << n << ")" << endl;
        return EXIT_FAILURE;
    }

    cout << "Successfully read " << data.size() << " integers from " << inputFile << endl;

    // TODO: Set up shared memory, copy input data into shared array,
    // fork worker processes, run hillis_steele_prefix_sum and barrier.

    // Write output file
    /*
        if (!write_output_file(outputFile, x, n))
        {
            return EXIT_FAILURE;
        }
    */

    return 0;
}
