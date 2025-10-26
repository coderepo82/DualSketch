#include "header/GlobalHH.h"
#include <iostream>
#include <limits>
#include <map>
#include <chrono>


GlobalHH::GlobalHH(float memory_kb){

    // memory allocation ratio for count-min
    cm_ratio = 0.4;

    // CountMin
    float cm_memo_kb = memory_kb * cm_ratio;
    count_min = new CountMin(cm_memo_kb);

    float ss_memo_kb = memory_kb - cm_memo_kb;
    max_num = (ss_memo_kb * 1024 * 8) / 96; // 64 bits key + 32 bits counter

    space_saving.reserve(max_num);
}


void GlobalHH::update(uint32_t x, uint32_t y) {

    count_min->update(x);

    uint64_t combined_xy = combine_xy(x,y);

    auto it = key_to_index.find(combined_xy);
    if (it != key_to_index.end()) {
        uint32_t index = it->second;
        if (space_saving[index].key == combined_xy){
            space_saving[index].counter++;
            return;
        }
        else{
            std::cerr << "GlobalHH: SpaceSaving index map error, falling back to linear search." << std::endl;
            for (uint32_t i = 0; i < space_saving.size(); ++i) {
                if (space_saving[i].key == combined_xy) {
                    space_saving[i].counter++;
                    key_to_index[combined_xy] = i;
                    return;
                }
            }
        }
    } else {
        if (space_saving.size() < max_num) {
            Entry new_entry = {combined_xy, 1};
            space_saving.push_back(new_entry);
            key_to_index[combined_xy] = space_saving.size() - 1;

        } else {
            replace_min_with_new_key(combined_xy);
        }
    }
}


void GlobalHH::replace_min_with_new_key(uint64_t new_key) {
    if (space_saving.empty()) {
        Entry new_entry = {new_key, 1};
        space_saving.push_back(new_entry);
        key_to_index[new_key] = space_saving.size() - 1;
        return;
    }

    uint32_t min_counter = std::numeric_limits<uint32_t>::max();
    int64_t min_index = -1;

    for (uint32_t i = 0; i < space_saving.size(); ++i) {
        if (space_saving[i].counter < min_counter) {
            min_counter = space_saving[i].counter;
            min_index = i;
        }
    }

    key_to_index.erase(space_saving[min_index].key);

    space_saving[min_index].key = new_key;
    space_saving[min_index].counter += 1;

    key_to_index[new_key] = min_index;
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
        std::map<uint32_t, std::map<uint32_t, uint32_t>>> GlobalHH::query(uint32_t heavy_hitter_th, float phi) {
    std::map<uint32_t, uint32_t> heavy_hitters;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> quad_elements;

    // Iterate through all entries
    for (auto& entry : space_saving) {

        uint64_t combined_xy = entry.key;
        uint32_t xy_count = entry.counter;

        std::pair<uint32_t, uint32_t> split_pair = split_xy(combined_xy);
        uint32_t x = split_pair.first;
        uint32_t y = split_pair.second;
        uint32_t cm_es = count_min->query(x);

        // Condition : Check for heavy hitter
        if (cm_es >= heavy_hitter_th) {
            heavy_hitters[x] = cm_es;

            // Condition 2: Check for hot quadratic element
            if (xy_count >= cm_es * phi) {
                quad_elements[x][y] = xy_count;
            }
        }
    }

    return {heavy_hitters, quad_elements};
}






void GlobalHH::evaluation(const std::vector<std::pair<uint32_t, uint32_t>> &dataset,
                      const std::map<uint32_t, uint32_t> &flows,
                      std::map<uint32_t, std::map<uint32_t, uint32_t>> quadratic_eles,
                      uint32_t heavy_hitter_th, float ele_th_phi) {

    std::cout << "\n" << "GlobalHH:" << std::endl;

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