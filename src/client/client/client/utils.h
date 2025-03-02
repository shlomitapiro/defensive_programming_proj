#pragma once

#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


// פונקציית עזר שממירה buffer בינארי (16 בתים) למחרוזת Hex באורך 32 תווים.
std::string bytesToHex(const std::string& bytes);
bool checkMeInfoFileExists();
std::string createFileInExeDir(const std::string& fileName);
