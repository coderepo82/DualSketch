

#ifndef GLOBALHH_H
#define GLOBALHH_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <map>
#include "CountMin.h"
#include "utils.h"


struct Entry {
    uint64_t key;
    uint32_t counter;

};

class GlobalHH {
private:

    float cm_ratio;

    CountMin* count_min; // use a count-min for flow size estimation, as did in DUET

    // an approximate implementation of space-saving
    std::vector<Entry> space_saving;

    std::unordered_map<uint64_t, uint32_t> key_to_index;

    uint32_t max_num; // max number for stored (x, y)

    void replace_min_with_new_key(uint64_t new_key);


public:

    GlobalHH(float memory_kb);

    void update(uint32_t x, uint32_t y);

    std::pair<std::map<uint32_t, uint32_t>,
            std::map<uint32_t, std::map<uint32_t, uint32_t>>> query(uint32_t heavy_hitter_th, float phi);

    void evaluation(const std::vector<std::pair<uint32_t, uint32_t>>& dataset,
                    const std::map<uint32_t, uint32_t>& flows,
                    std::map<uint32_t, std::map<uint32_t, uint32_t>> quadratic_eles,
                    uint32_t heavy_hitter_th, float phi);


};

#endif
