
#include <algorithm>
#include "header/CountMin.h"

CountMin::CountMin(float memory_kb) {

    this->counter_bits = 32;
    this->depth = 4;

    uint32_t total_counters = static_cast<uint32_t>(std::round(memory_kb * 1024 * 8 / this->counter_bits));

    width = static_cast<int>(std::round(total_counters / depth));

    counters = std::vector<std::vector<uint32_t>>(depth, std::vector<uint32_t>(width, 0));
}



void CountMin::update(const uint32_t flow_label, uint32_t weight) {
    uint32_t hash_value = 0;
    for (int i = 0; i < depth; i++) {
        MurmurHash3_x86_32(&flow_label, 4, i, &hash_value);
        uint32_t j = hash_value % width;
        counters[i][j] = std::clamp(counters[i][j] + weight, 0u, UINT32_MAX);
    }
}


uint32_t CountMin::query(const uint32_t flow_label) {

    int min_value = 0x7FFFFFFF;
    uint32_t hash_value = 0;
    for (int i = 0; i < depth; i++) {
        MurmurHash3_x86_32(&flow_label, 4, i, &hash_value);
        uint32_t j = hash_value % width;
        int val = counters[i][j];
        if (val < min_value) {
            min_value = val;
        }
    }
    return min_value;
}
