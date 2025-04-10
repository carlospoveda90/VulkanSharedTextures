#pragma once
#include <iostream>

#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
#define LOG_WARN(msg) std::cout << "[WARN] " << msg << std::endl
#define LOG_ERR(msg) std::cerr << "[ERROR] " << msg << std::endl
