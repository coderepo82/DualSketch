#include "header/CSSCHH.h"
#include <iostream>
#include <limits>
#include <map>
#include <chrono>


CSSCHH::CSSCHH(float memory_kb){

    // memory allocation ratio for ss1
    ss1_hh_ratio = 0.4;

    N = 0; // N init by 0

    // ss for heavy hitter
    float ss1_memo_kb = memory_kb * ss1_hh_ratio;
    float ss2_memo_kb = memory_kb - ss1_memo_kb;

    max_num_ss1 = (ss1_memo_kb * 1024 * 8) / 64; // 32 bits key + 32 bits counter
    max_num_ss2 = (ss2_memo_kb * 1024 * 8) / 96; // 64 bits key + 32 bits counter

    ss1_heavy_hitter.reserve(max_num_ss1);
    ss2_quad_ele.reserve(max_num_ss2);
}


void CSSCHH::update(uint32_t x, uint32_t y) {

    insert_ss1(x); // insert flow

    uint64_t combined_xy = combine_xy(x,y);

    auto it = key_to_index_ss2.find(combined_xy);
    if (it != key_to_index_ss2.end()) {
        uint32_t index = it->second;
        if (ss2_quad_ele[index].key == combined_xy){
            ss2_quad_ele[index].counter++;
            return;
        }
        else{
            std::cerr << "CSSCHH: SS2 index map error, falling back to linear search." << std::endl;
            for (uint32_t i = 0; i < ss2_quad_ele.size(); ++i) {
                if (ss2_quad_ele[i].key == combined_xy) {
                    ss2_quad_ele[i].counter++;
                    key_to_index_ss2[combined_xy] = i;
                    return;
                }
            }
        }
    } else {
        if (ss2_quad_ele.size() < max_num_ss2) {
            STEntry new_entry = {combined_xy, 1};
            ss2_quad_ele.push_back(new_entry);
            key_to_index_ss2[combined_xy] = ss2_quad_ele.size() - 1;

        } else {
            replace_min_element(combined_xy);
        }
    }
}



void CSSCHH::insert_ss1(uint32_t x) {

    auto it = key_to_index_ss1.find(x);
    if (it != key_to_index_ss1.end()) {
        uint32_t index = it->second;
        if (ss1_heavy_hitter[index].label == x){
            ss1_heavy_hitter[index].freq++;
            return;
        }
        else{
            std::cerr << "CSSCHH: SS1 index map error, falling back to linear search." << std::endl;
            for (uint32_t i = 0; i < ss1_heavy_hitter.size(); ++i) {
                if (ss1_heavy_hitter[i].label == x) {
                    ss1_heavy_hitter[i].freq++;
                    key_to_index_ss1[x] = i;
                    return;
                }
            }
        }
    } else {
        if (ss1_heavy_hitter.size() < max_num_ss1) {
            SPEntry new_entry = {x, 1};
            ss1_heavy_hitter.push_back(new_entry);
            key_to_index_ss1[x] = ss1_heavy_hitter.size() - 1;

        } else {
            replace_min_flow(x);
        }
    }
}



void CSSCHH::replace_min_flow(uint32_t new_flow) {
    if (ss1_heavy_hitter.empty()) {
        SPEntry new_entry = {new_flow, 1};
        ss1_heavy_hitter.push_back(new_entry);
        key_to_index_ss1[new_flow] = ss1_heavy_hitter.size() - 1;
        return;
    }

    uint32_t min_freq = std::numeric_limits<uint32_t>::max();
    int64_t min_index = -1;

    for (uint32_t i = 0; i < ss1_heavy_hitter.size(); ++i) {
        if (ss1_heavy_hitter[i].freq < min_freq) {
            min_freq = ss1_heavy_hitter[i].freq;
            min_index = i;
        }
    }

    key_to_index_ss1.erase(ss1_heavy_hitter[min_index].label);

    ss1_heavy_hitter[min_index].label = new_flow;
    ss1_heavy_hitter[min_index].freq += 1;

    key_to_index_ss1[new_flow] = min_index;
}




void CSSCHH::replace_min_element(uint64_t new_key) {
    if (ss2_quad_ele.empty()) {
        STEntry new_entry = {new_key, 1};
        ss2_quad_ele.push_back(new_entry);
        key_to_index_ss2[new_key] = ss2_quad_ele.size() - 1;
        return;
    }

    uint32_t min_counter = std::numeric_limits<uint32_t>::max();
    int64_t min_index = -1;

    for (uint32_t i = 0; i < ss2_quad_ele.size(); ++i) {
        if (ss2_quad_ele[i].counter < min_counter) {
            min_counter = ss2_quad_ele[i].counter;
            min_index = i;
        }
    }

    key_to_index_ss2.erase(ss2_quad_ele[min_index].key);

    ss2_quad_ele[min_index].key = new_key;
    ss2_quad_ele[min_index].counter += 1;

    key_to_index_ss2[new_key] = min_index;
}



/**
 * @brief retrieve heavy hitters and their quadratic elements.
 * @return A pair of maps.
 * 1st map stores heavy hitters: ID (uint32_t) -> estimated frequency (uint32_t).
 * 2nd map stores hot quadratic elements for each heavy hitter:
 * The key is the heavy hitter's ID (uint32_t), and the value is another map.
 * The inner map stores element (uint32_t) -> frequency (uint32_t).
 */
std::pair<std::map<uint32_t, uint32_t>,
        std::map<uint32_t, std::map<uint32_t, uint32_t>>> CSSCHH::query(uint32_t heavy_hitter_th, float phi) {
    std::map<uint32_t, uint32_t> heavy_hitters;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> quad_elements;

    for (auto& entry : ss1_heavy_hitter) {
        if (entry.freq >= heavy_hitter_th) {
            heavy_hitters[entry.label] = entry.freq;
        }
    }

    // Iterate through all entries
    for (auto& entry : ss2_quad_ele) {

        uint64_t combined_xy = entry.key;
        uint32_t xy_count = entry.counter;

        std::pair<uint32_t, uint32_t> split_pair = split_xy(combined_xy);
        uint32_t x = split_pair.first;
        uint32_t y = split_pair.second;

        auto it = heavy_hitters.find(x);
        if (it != heavy_hitters.end()) {
            uint32_t freq = it->second;

            // Check for hot quadratic element
            if (xy_count >= phi * (freq - (N/max_num_ss1))) {
                quad_elements[x][y] = xy_count;
            }
        }
    }

    return {heavy_hitters, quad_elements};
}






void CSSCHH::evaluation(const std::vector<std::pair<uint32_t, uint32_t>> &dataset,
                          const std::map<uint32_t, uint32_t> &flows,
                          std::map<uint32_t, std::map<uint32_t, uint32_t>> quadratic_eles,
                          uint32_t heavy_hitter_th, float ele_th_phi) {

    std::cout << "\n" << "CSSCHH: " << std::endl;

    N = dataset.size();

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
    auto [queried_heavy_hitters, queried_quad_elements] = query(heavy_hitter_th, ele_th_phi);
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
