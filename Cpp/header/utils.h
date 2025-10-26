#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <utility>
#include <iostream>
#include <vector>
#include <cmath>

// x, y --> combined_xy
uint64_t combine_xy(uint32_t x, uint32_t y);

// combined_xy --> x, y
std::pair<uint32_t, uint32_t> split_xy(uint64_t combined);



// To generate a specified number (param 'count') of random seeds
std::vector<uint32_t> generateSeeds32(size_t count);



#endif // UTILS_H