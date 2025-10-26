#include "header/DUET.h"

#include <algorithm>
#include <chrono>

#include "header/CountMin.h"


DUET::DUET(float memory_kb) {

    Nth = 1000;

    // memory allocation ratio
    float cm_ratio = 0.35;
    float filter_ratio = 0.35;
    float stable_ratio = 0.3;

    float total_bits = memory_kb * 1024 * 8;

    // CountMin
    float cm_bits = total_bits * cm_ratio;
    count_min = new CountMin(cm_bits/1024/8);

    // Filter
    size_t filter_bits = static_cast<size_t>(total_bits * filter_ratio);
    // Each bucket = 64 bits (x,y) + 32 bits count = 96 bits
    d_filter = 4;
    w_filter = static_cast<int>(filter_bits / (d_filter * 96));
    if (w_filter < 1) w_filter = 1;
    filter.resize(d_filter, std::vector<Bucket>(w_filter, {0, 0}));

    // STable
    size_t stable_bits = static_cast<size_t>(total_bits * stable_ratio);
    l_stable = 200;
    r_stable = static_cast<int>(stable_bits / (l_stable * 96));
    if (r_stable < 1) r_stable = 1;
    stable.resize(l_stable, std::vector<Bucket>(r_stable, {0, 0}));

    rand_seeds = generateSeeds32(d_filter);

}

DUET::~DUET() {
    delete count_min;
}



void DUET::Insert2Filter(uint32_t x, uint32_t y) {

    uint32_t hash_val = 0;
    MurmurHash3_x86_32(&y, sizeof(y), 799957137, &hash_val);
    uint32_t row = hash_val % d_filter;

    uint32_t hash_val_x = 0;
    MurmurHash3_x86_32(&x, sizeof(x), rand_seeds[row], &hash_val_x);
    uint32_t col = hash_val_x % w_filter;

    uint64_t combined_xy = combine_xy(x,y);

    if (filter[row][col].element == 0) {
        filter[row][col].element = combined_xy;
        filter[row][col].count = 1;
    }
    else if (filter[row][col].element == combined_xy) {
        filter[row][col].count++;
    }

    else {
        filter[row][col].count--;
        if (filter[row][col].count == 0) {
            filter[row][col].element = combined_xy;
            filter[row][col].count = 1;
        }
    }

}



void DUET::Insert2Table(uint32_t x, uint32_t y, uint32_t cnt) {

    uint64_t combined_xy = combine_xy(x, y);

    uint32_t hash_val = 0;
    MurmurHash3_x86_32(&x, sizeof(x), 17157137, &hash_val);
    uint32_t i = hash_val % l_stable;

    // Search for combined_xy in the determined row
    int empty_cell_index = -1;
    uint32_t min_frequency = UINT32_MAX; // Max value for uint32_t
    int min_cell_index = -1;

    for (int j = 0; j < r_stable; ++j) {
        Bucket& current_cell = stable[i][j];

        if (current_cell.element == combined_xy) {
            // Case 1: The element already exists. Add to its frequency.
            current_cell.count += cnt;
            return;
        }

        // Find an empty cell
        if (current_cell.element == 0 && empty_cell_index == -1) {
            empty_cell_index = j;
        }

        if (current_cell.element != 0 && current_cell.count < min_frequency) {
            min_frequency = current_cell.count;
            min_cell_index = j;
        }
    }

    // Case 2: Element does not exist, but there is an empty cell.
    if (empty_cell_index != -1) {
        Bucket& empty_cell = stable[i][empty_cell_index];
        empty_cell.element = combined_xy;
        empty_cell.count = cnt;
        return;
    }

    // Case 3: Element does not exist, and the row is full.
    // Decrease the frequency of the least frequent cell.
    Bucket& min_cell = stable[i][min_cell_index];
    if (min_cell.count > cnt) {
        min_cell.count -= cnt;
    } else {
        min_cell.element = combined_xy;
        min_cell.count = cnt - min_cell.count;
    }
}


void DUET::update(uint32_t x, uint32_t y) {

    uint32_t cm_es = count_min->query(x);
    count_min->update(x);
    if (cm_es < Nth) {
        Insert2Filter(x, y);
        if (cm_es + 1 == Nth) {
            for (int i=0; i < d_filter; i++) {
                uint32_t hash_val = 0;
                MurmurHash3_x86_32(&x, sizeof(x), rand_seeds[i], &hash_val);
                uint32_t j = hash_val % w_filter;
                uint64_t combined_xy = filter[i][j].element;
                std::pair<uint32_t, uint32_t> labels = split_xy(combined_xy);
                if (labels.first == x) {
                    Insert2Table(x,labels.second, filter[i][j].count);
                    filter[i][j].element = 0;
                    filter[i][j].count = 0;
                }
            }
        }

    } else {
        Insert2Table(x, y,1);
    }

}




// 1st map stores heavy item/flow along with estimated frequency
// 2nd map stores hot quadratic elements (in the form of combine(x,y)) and frequencies
// you can invoke function 'split_xy(uint64_t)' to get 'x' and 'y'
std::pair<std::map<uint32_t, uint32_t>, std::map<uint64_t, uint32_t>> DUET::query(uint32_t heavy_hitter_th, float phi) {

    // an element is hot quadratic if its count >= (phi * item's frequency)

    std::map<uint32_t, uint32_t> heavy_hitters;
    std::map<uint64_t, uint32_t> hot_quadratic_elements;

    // Iterate through all cells in STable
    for (int i = 0; i < l_stable; ++i) {
        for (int j = 0; j < r_stable; ++j) {
            const Bucket& current_cell = stable[i][j];
            if (current_cell.element != 0) {
                uint64_t combined_xy = current_cell.element;
                uint32_t xy_count = current_cell.count;

                std::pair<uint32_t, uint32_t> split_pair = split_xy(combined_xy);
                uint32_t current_x = split_pair.first;
                uint32_t cm_es = count_min->query(current_x);

                // Condition : Check for heavy hitter
                if (cm_es >= heavy_hitter_th) {
                    heavy_hitters[current_x] = cm_es;

                    // Condition 2: Check for hot quadratic element
                    if (xy_count >= cm_es * phi) {
                        hot_quadratic_elements[combined_xy] = xy_count;
                    }
                }

            }
        }
    }

    return {heavy_hitters, hot_quadratic_elements};
}




/**
 * @brief retrieve heavy hitters and their hot quadratic elements.
 * @return A pair of maps.
 * 1st map stores heavy hitters: flow ID (uint32_t) -> estimated frequency (uint32_t).
 * 2nd map stores hot quadratic elements for each heavy hitter:
 * The key is the heavy hitter's flow ID (uint32_t), and the value is another map.
 * The inner map stores element ID (uint32_t) -> frequency (uint32_t).
 */
std::pair<std::map<uint32_t, uint32_t>, std::map<uint32_t, std::map<uint32_t, uint32_t>>> DUET::getHHAndHotQuadEle(uint32_t heavy_hitter_th, float phi) {

    auto[heavy_hitters, hot_quadratic_elements] = query(heavy_hitter_th, phi);

    // This map will store the final hot elements for each heavy hitter
    std::map<uint32_t, std::map<uint32_t, uint32_t>> hot_quad_elements;

    for (const auto& pair : hot_quadratic_elements) {
        uint64_t combined_xy = pair.first;
        uint32_t frequency = pair.second;
        auto [x, y] = split_xy(combined_xy);
        hot_quad_elements[x][y] = frequency;
    }

    return {heavy_hitters, hot_quad_elements};
}




void DUET::evaluation(const std::vector<std::pair<uint32_t, uint32_t>> &dataset,
                            const std::map<uint32_t, uint32_t> &flows,
                            std::map<uint32_t, std::map<uint32_t, uint32_t>> quadratic_eles,
                            uint32_t heavy_hitter_th, float ele_th_phi) {

    std::cout << "\n" << "DUET:" << std::endl;

    // Process the entire dataset
    auto start_update = std::chrono::high_resolution_clock::now();
    for (const auto &[x, y]: dataset) {
        update(x, y);
    }
    auto end_update = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> update_duration = end_update - start_update;
    double update_throughput_Mdps = (dataset.size() / 1e6) / update_duration.count();
    std::cout << " - Update Throughput: " << update_throughput_Mdps << " Mdps" << std::endl;


    // Query the sketch for results
    auto start_query = std::chrono::high_resolution_clock::now();
    auto [queried_heavy_hitters, queried_quad_elements] = getHHAndHotQuadEle(heavy_hitter_th, ele_th_phi);
    auto end_query = std::chrono::high_resolution_clock::now();
    auto query_duration = end_query - start_query;
    std::cout << " - Query Time: " << std::chrono::duration<double, std::milli>(query_duration).count() << " ms" << std::endl;


    // Find true heavy hitters and their hot quadratic elements
    std::map<uint32_t, uint32_t> true_heavy_hitters;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> true_hot_quad_elements;

    for (const auto &[flow_id, flow_size]: flows) {
        if (flow_size >= heavy_hitter_th) {
            true_heavy_hitters[flow_id] = flow_size;

            // Find true hot quadratic elements for this heavy hitter
            if (quadratic_eles.count(flow_id)) {
                for (const auto &[ele_id, ele_size]: quadratic_eles.at(flow_id)) {
                    if (ele_size >= ele_th_phi * flow_size)
                        true_hot_quad_elements[flow_id][ele_id] = ele_size;
                }
            }
        }
    }

    // --- Heavy Hitter Evaluation ---

    float hh_are_sum = 0.0f;
    uint32_t hh_true_positives = 0;
    for (const auto &[id, true_size]: true_heavy_hitters) {
        if (queried_heavy_hitters.count(id)) {
            uint32_t queried_size = queried_heavy_hitters.at(id);
            hh_are_sum += std::abs(static_cast<float>(true_size) - queried_size) / true_size;
            hh_true_positives++;
        }
    }

    float hh_are = (hh_true_positives > 0) ? hh_are_sum / hh_true_positives : 0.0f;
    float hh_precision = (queried_heavy_hitters.size() > 0) ? static_cast<float>(hh_true_positives) /
                                                              queried_heavy_hitters.size() : 0.0f;
    float hh_recall = (true_heavy_hitters.size() > 0) ? static_cast<float>(hh_true_positives) /
                                                        true_heavy_hitters.size() : 0.0f;
    float hh_f1 = (hh_precision + hh_recall > 0) ? 2 * (hh_precision * hh_recall) / (hh_precision + hh_recall) : 0.0f;

    std::cout << " - Heavy Hitter Metrics | ";
    std::cout << "ARE: " << hh_are << ", ";
    std::cout << "F1: " << hh_f1 << "\n";


    // --- Quadratic Element Evaluation ---

    float ele_are_sum = 0.0f;
    uint32_t ele_true_positives = 0;
    uint32_t total_queried_hot_ele_count = 0; // queried hot quadratic element count
    uint32_t total_true_hot_ele_count = 0; // true hot quadratic element count


    for (const auto &[flow_id, queried_elements]: queried_quad_elements) {
        total_queried_hot_ele_count += queried_elements.size();
    }


    // Calculate ARE and True Positives
    for (const auto &[flow_id, true_hot_elements]: true_hot_quad_elements) {
        if (queried_quad_elements.count(flow_id)) {
            const auto &queried_elements = queried_quad_elements.at(flow_id);
            for (const auto &[ele_id, true_size]: true_hot_elements) {
                if (queried_elements.count(ele_id)) {
                    uint32_t queried_size = queried_elements.at(ele_id);
                    ele_are_sum += std::abs(static_cast<float>(true_size) - queried_size) / true_size;
                    ele_true_positives++;
                }
            }
        }
        total_true_hot_ele_count += true_hot_elements.size();
    }



    float ele_are = (ele_true_positives > 0) ? ele_are_sum / ele_true_positives : 0.0f;
    float ele_precision = (total_queried_hot_ele_count > 0) ? static_cast<float>(ele_true_positives) /
                                                              total_queried_hot_ele_count : 0.0f;
    float ele_recall = (total_true_hot_ele_count > 0) ? static_cast<float>(ele_true_positives) / total_true_hot_ele_count
                                                      : 0.0f;
    float ele_f1 = (ele_precision + ele_recall > 0) ? 2 * (ele_precision * ele_recall) / (ele_precision + ele_recall)
                                                    : 0.0f;

    std::cout << " - Heavy Quadratic Ele Metrics | ";
    std::cout << "ARE: " << ele_are << ", ";
    std::cout << "F1: " << ele_f1 << "\n";

}
