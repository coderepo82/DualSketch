
#ifndef DUET_H
#define DUET_H
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <map>
#include "utils.h"


class CountMin;

class DUET {
private:

    uint32_t Nth; // threshold for hot item/flow (i.e., heavy hitter)

    struct Bucket {
        uint64_t element; // identifier for (x, y)
        uint32_t count;
    };

    CountMin* count_min;

    // Filter
    std::vector<std::vector<Bucket>> filter;
    int d_filter; // d rows
    int w_filter; // w columns

    // STable
    std::vector<std::vector<Bucket>> stable;
    int l_stable; // l rows
    int r_stable; // r columns

    std::vector<uint32_t> rand_seeds; // random seeds

public:
    explicit DUET(float memory_kb);
    ~DUET();

    void update(uint32_t x, uint32_t y);

    std::pair<std::map<uint32_t, uint32_t>, std::map<uint64_t, uint32_t>> query(uint32_t heavy_hitter_th, float phi);

    std::pair<std::map<uint32_t, uint32_t>, std::map<uint32_t, std::map<uint32_t, uint32_t>>> getHHAndHotQuadEle(uint32_t heavy_hitter_th, float phi);

    void Insert2Filter(uint32_t x, uint32_t y);
    void Insert2Table(uint32_t x, uint32_t y, uint32_t count);

    void evaluation(const std::vector<std::pair<uint32_t, uint32_t>>& dataset,
                    const std::map<uint32_t, uint32_t>& flows,
                    std::map<uint32_t, std::map<uint32_t, uint32_t>> quadratic_eles,
                    uint32_t heavy_hitter_th, float phi);

};

#endif