
#ifndef COUNTMIN_H
#define COUNTMIN_H


#include <cstdlib>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include "MurmurHash3.h"
#include <random>
#include <vector>


class CountMin {
    private:
        int depth;
        int width;
        uint32_t counter_bits;
        std::vector<std::vector<uint32_t>> counters;

    public:
        CountMin(float memory_kb);

        void update(const uint32_t flow_label, uint32_t weight= 1);

        uint32_t query(const uint32_t flow_label);

};


#endif
