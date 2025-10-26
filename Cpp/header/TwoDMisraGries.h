

#ifndef TWODMISRAGRIES_H
#define TWODMISRAGRIES_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <algorithm>
#include <map>


struct InnerStruct {
    uint32_t key_inner;
    uint32_t freq_inner;

    InnerStruct() : key_inner(0), freq_inner(0){}
};


struct OuterStruct {
    uint32_t freq_outer;
    std::vector<InnerStruct> inner_list;
};


class TwoDMisraGries {

private:

    size_t s1; // length of outer_map
    size_t s2; // length of inner_list

    // Use a map (rather than a list) to store flow info,
    // to avoid traversal when looking up a flow in update process
    std::unordered_map<uint32_t, OuterStruct> outer_map;

    void update_inner_list(std::vector<InnerStruct>& list, uint32_t key_inner);

public:
    TwoDMisraGries(float memory_kb);

    void update(uint32_t x, uint32_t y);

    std::pair<std::map<uint32_t, uint32_t>,
            std::map<uint32_t, std::map<uint32_t, uint32_t>>> query(uint32_t heavy_hitter_th, float phi);

    void evaluation(const std::vector<std::pair<uint32_t, uint32_t>>& dataset,
                    const std::map<uint32_t, uint32_t>& flows,
                    std::map<uint32_t, std::map<uint32_t, uint32_t>> quadratic_eles,
                    uint32_t heavy_hitter_th, float phi);

};

#endif
