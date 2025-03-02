#include "Utils.h"



std::string bytesToHex(const std::string& bytes)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : bytes)
    {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

bool checkMeInfoFileExists()
{
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = std::string(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
    std::string meInfoFilePath = exeDir + "\\me.info";
    if (std::filesystem::exists(meInfoFilePath)) {
        return false;
    }
    return true;
}

std::string createFileInExeDir(const std::string& fileName)
{
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = std::string(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
    std::string filePath = exeDir + "\\" + fileName;
    std::ofstream file(filePath);
    if (!file.is_open()) {
		return "";   
    }
	file.close();
	return filePath;
}
