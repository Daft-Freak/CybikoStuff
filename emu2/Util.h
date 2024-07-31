#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <string>

uint32_t cyIDFromString(const std::string &str);
std::string cyIDToString(uint32_t id);

uint32_t cybikoTime();

#endif