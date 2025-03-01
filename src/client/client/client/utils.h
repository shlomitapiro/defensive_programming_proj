#pragma once

#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>

// פונקציית עזר שממירה buffer בינארי (16 בתים) למחרוזת Hex באורך 32 תווים.
std::string bytesToHex(const std::string& bytes);
