## The project is modularized to separate different concerns. Here is the role of each file:

Makefile: The build script. Use make to compile the project.

structures.h: Contains all the C++ structs that define the problem (e.g., Helicopter, Village) and the solution (e.g., Trip, Solution).

io_handler.cpp / .h: Handles parsing input files and formatting output files. You should not need to modify this.

solver.cpp / .h: This is your primary workspace. The core problem-solving logic goes here.

main.cpp: The main driver. It connects the I/O to the solver and manages the timeout. You should not need to modify this.

format_checker.cpp: A separate program to validate your output and calculate its score.

sample_input.txt: An example input file for testing.
sample_output.txt: A valid output corresponding to the sample input, for your reference.