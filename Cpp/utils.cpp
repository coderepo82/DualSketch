#include "header/utils.h"
#include <chrono>
#include <random>
#include <fstream>

uint64_t combine_xy(uint32_t x, uint32_t y) {
    return (static_cast<uint64_t>(x) << 32) | y;
}


std::pair<uint32_t, uint32_t> split_xy(uint64_t combined) {
    uint32_t x = static_cast<uint32_t>(combined >> 32);
    uint32_t y = static_cast<uint32_t>(combined);
    return {x, y};
}


std::vector<uint32_t> generateSeeds32(size_t count) {
    std::vector<uint32_t> seeds;
    seeds.reserve(count);
    std::random_device rd;
    std::mt19937 gen(rd() ^ static_cast<unsigned>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<uint32_t> dist((1U << 24), std::numeric_limits<uint32_t>::max());
    for (size_t i = 0; i < count; ++i) {
        seeds.push_back(dist(gen));
    }
    return seeds;
}
