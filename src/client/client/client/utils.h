#pragma once
#define NOMINMAX

#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdint>
#include <tuple>
#include <cstring>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

/**
 * @brief Converts a binary buffer (16 bytes) into a hexadecimal string.
 *
 * Each two bytes in the output represent one byte of input in hexadecimal format.
 *
 * @param bytes A string containing the binary data.
 * @return A hexadecimal representation of the input data as a string.
 */
std::string bytesToHex(const std::string& bytes);

/**
 * @brief Checks whether the "me.info" file is missing in the executable directory.
 *
 * This function looks for a file named "me.info" in the same directory as the executable.
 *
 * @return true if the file is missing, false if it exists.
 */
bool checkMeInfoFileMissing();

/**
 * @brief Creates a file in the executable directory.
 *
 * The file is created in the same directory as the executable.
 *
 * @param fileName The name of the file to create.
 * @return The full path to the created file, or an empty string if the file could not be created.
 */
std::string createFileInExeDir(const std::string& fileName);

/**
 * @brief Trims leading and trailing whitespace from a string.
 *
 * Whitespace characters include spaces, tabs, and newline characters.
 *
 * @param s The string to be trimmed.
 * @return A new string with the leading and trailing whitespace removed.
 */
std::string trim(const std::string& s);

/**
 * @brief Adjusts a string to a specified size by either padding or truncating it.
 *
 * If the input string is shorter than \p size, the function appends null characters ('\0')
 * to make it exactly \p size bytes long. If the input string is longer, it is truncated
 * to the first \p size bytes.
 *
 * @param str The original string to adjust.
 * @param size The desired size (in bytes) of the returned string.
 * @return A new string of exactly \p size bytes.
 */
std::string adjustToSize(const std::string& str, size_t size);

/**
 * @brief Retrieves the directory path of the current executable.
 *
 * This function uses Windows API (GetModuleFileNameA) to get the full path of the running
 * executable, then extracts the directory portion from it.
 *
 * @return A string containing the path to the directory of the current executable.
 */
std::string getExeDirectory();