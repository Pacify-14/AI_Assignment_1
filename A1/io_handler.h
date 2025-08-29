#ifndef IO_HANDLER_H
#define IO_HANDLER_H

#include <string>
#include "structures.h"

/**
 * @brief Reads and parses the input file.
 * * @param filename The path to the input file.
 * @return A ProblemData struct containing all the parsed information.
 */
ProblemData readInputData(const std::string& filename);

/**
 * @brief Writes the generated solution to an output file in the specified format.
 * * @param filename The path to the output file.
 * @param solution The solution object to write.
 */
void writeOutputData(const std::string& filename, const Solution& solution);

#endif // IO_HANDLER_H