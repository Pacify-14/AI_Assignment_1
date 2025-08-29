# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

# Executable names
EXEC = main
CHECKER_EXEC = format_checker

# Source files for the main solver
SRCS = main.cpp io_handler.cpp solver.cpp

# Source file for the checker
CHECKER_SRCS = format_checker.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)
CHECKER_OBJS = $(CHECKER_SRCS:.cpp=.o)

# Default target
all: $(EXEC)

# Rule to link the main solver program
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(OBJS)

# Rule to build the checker executable
checker: $(CHECKER_EXEC)

# NEW LINKING RULE: The checker executable depends on its own object file
# AND the io_handler.o file from the main project.
$(CHECKER_EXEC): $(CHECKER_OBJS) io_handler.o
	$(CXX) $(CXXFLAGS) -o $(CHECKER_EXEC) $(CHECKER_OBJS) io_handler.o

# Generic rule to compile .cpp to .o
# The headers are dependencies for all object files.
%.o: %.cpp structures.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(EXEC) $(CHECKER_EXEC) $(OBJS) $(CHECKER_OBJS)

.PHONY: all clean checker