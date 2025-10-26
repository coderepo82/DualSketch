

#ifndef CSSCHH_H
#define CSSCHH_H


#include <cstdint>
#include <vector>
#include <unordered_map>
#include <map>
#include "utils.h"

// An approximate implementation of the following paper's method:
// “Fast and accurate mining of correlated heavy hitters”
// https://doi.org/10.1007/s10618-017-0526-x

// Cascading Space Saving -> CSS


struct SPEntry {
    uint32_t label;
    uint32_t freq;
};

struct STEntry {
    uint64_t key;
    uint32_t counter;
};

class CSSCHH {
private:

    float ss1_hh_ratio;

    uint32_t N;

    // an approximate implementation of space-saving for heavy hitter
    std::vector<SPEntry> ss1_heavy_hitter;

    // an approximate implementation of space-saving for quadratic elements
    std::vector<STEntry> ss2_quad_ele;

    // fast index for entry in ss1_heavy_hitter
    std::unordered_map<uint32_t, uint32_t> key_to_index_ss1;

    // fast index for entry in ss2_quad_ele
    std::unordered_map<uint64_t, uint32_t> key_to_index_ss2;

    uint32_t max_num_ss1; // parameter k1

    uint32_t max_num_ss2; // parameter k2

    void insert_ss1(uint32_t x);
    void replace_min_flow(uint32_t new_flow);

    void replace_min_element(uint64_t new_ele);


public:

    CSSCHH(float memory_kb);

    void update(uint32_t x, uint32_t y);

    std::pair<std::map<uint32_t, uint32_t>,
            std::map<uint32_t, std::map<uint32_t, uint32_t>>> query(uint32_t heavy_hitter_th, float phi);

    void evaluation(const std::vector<std::pair<uint32_t, uint32_t>>& dataset,
                    const std::map<uint32_t, uint32_t>& flows,
                    std::map<uint32_t, std::map<uint32_t, uint32_t>> quadratic_eles,
                    uint32_t heavy_hitter_th, float phi);


};




#endif
