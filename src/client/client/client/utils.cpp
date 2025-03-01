#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>

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
