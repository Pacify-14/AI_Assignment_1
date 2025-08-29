#include <iostream>
#include <string>
#include "structures.h"
#include "io_handler.h"
#include "solver.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_filename> <output_filename>" << endl;
        return 1;
    }

    string input_filename = argv[1];
    string output_filename = argv[2];

    try {
        // 1. Read problem data from input file
        ProblemData problem = readInputData(input_filename);
        cout << "Successfully read input file: " << input_filename << endl;

        auto allowed_duration = chrono::milliseconds(long(problem.time_limit_minutes * 60 * 1000));
        auto start_time = chrono::steady_clock::now();
        auto deadline = start_time + allowed_duration;

        // 2. Solve the problem
        Solution solution = solve(problem);
        
        auto end_time = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << "Solver completed in " << elapsed.count() / 1000.0 << " seconds." << endl;

        if (end_time > deadline) {
            cerr << "TimeLimitExceeded: Solver exceeded the time limit of " << problem.time_limit_minutes << " minutes." << endl;
            cout << "This instance will receive a score of 0." << endl;
            return 1;
        }

        // 3. Write the solution to the output file
        writeOutputData(output_filename, solution);
        cout << "Successfully wrote solution to output file: " << output_filename << endl;

    } catch (const runtime_error& e) {
        cerr << "An error occurred: " << e.what() << endl;
        return 1;
    }

    return 0;
}