#include "utils.h"


std::string bytesToHex(const std::string& bytes)
{
    std::ostringstream oss;
    // Configure output stream for hexadecimal formatting with zero-padding.
    oss << std::hex << std::setfill('0');
    for (unsigned char c : bytes)
    {
        // Convert each byte to a two-digit hex string.
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

bool checkMeInfoFileMissing()
{
	std::string exeDir = getExeDirectory();
    std::string meInfoFilePath = exeDir + "\\me.info";
    // Return true if the file does not exist.
    return !std::filesystem::exists(meInfoFilePath);
}

std::string createFileInExeDir(const std::string& fileName)
{
	std::string exeDir = getExeDirectory();
    std::string filePath = exeDir + "\\" + fileName;
    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to create file: " + filePath);
       //return "";  // Return empty string if file creation fails.
    }
    file.close();
    return filePath;
}

std::string trim(const std::string& s) {
    // Find the first non-space character.
    auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
    // Find the last non-space character (using reverse iterator).
    auto end = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    // Return the trimmed string.
    return (start < end ? std::string(start, end) : "");
}

// Helper: Adjusts a string to exactly 'size' bytes (pad with '\0' if too short; truncate if too long).
std::string adjustToSize(const std::string& str, size_t size) {
    std::string s = str;
    if (s.size() < size) {
        s.append(size - s.size(), '\0');
    }
    else if (s.size() > size) {
        s = s.substr(0, size);
    }
    return s;
}

std::string getExeDirectory() {
    char exePath[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        throw std::runtime_error("GetModuleFileNameA failed");
    }
    std::string path(exePath);
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) {
        throw std::runtime_error("Failed to determine executable directory");
    }
    return path.substr(0, pos);
}