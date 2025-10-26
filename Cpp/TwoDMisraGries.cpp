
#include <cmath>
#include <random>
#include <chrono>
#include "header/TwoDMisraGries.h"



TwoDMisraGries::TwoDMisraGries(float memory_kb) {

    s2 = 8; // length for each inner list

    // 2 fields: heavy hitter key and frequency, each is 32 bits
    uint32_t outer_bits = 32 + 32;

    // 2 fields: Element key and frequency, each is 32 bits
    const uint32_t inner_bits = 32 + 32;

    // heavy hitter key + frequency + a list for elements
    uint32_t bit_sum_4_outer_cell = outer_bits + (s2 * inner_bits);

    // Calculate 's1', the length of outer list
    if (bit_sum_4_outer_cell > 0) {
        s1 = static_cast<uint32_t>(std::round(memory_kb * 1024 * 8 / bit_sum_4_outer_cell));
    } else {
        s1 = 0;
    }

    if (s1 == 0) {
        std::cerr << "Error: The calculated length of outer list is 0. Please increase memory_kb." << std::endl;
        return;
    }

}




void TwoDMisraGries::update(uint32_t x, uint32_t y) {

    auto it = outer_map.find(x);

    if (it != outer_map.end()) { // case 1, x exists.
        it->second.freq_outer++;
        update_inner_list(it->second.inner_list, y);

    } else { // case 2: x does not exist

        if (outer_map.size() < s1) {

            OuterStruct new_outer;
            new_outer.freq_outer = 1;

            InnerStruct new_inner;
            new_inner.key_inner = y;
            new_inner.freq_inner = 1;

            new_outer.inner_list.push_back(new_inner);
            outer_map.emplace(x, new_outer);

        } else {

            std::vector<uint32_t> keys_to_remove;
            for (auto& pair : outer_map) {
                pair.second.freq_outer--;

                if (pair.second.freq_outer == 0) {
                    keys_to_remove.push_back(pair.first);
                } else{
                    // random choose an item in inner list, decrease its freq
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::vector<InnerStruct>& inner_list = pair.second.inner_list;
                    if (!inner_list.empty()) {
                        std::uniform_int_distribution<> distrib(0, inner_list.size() - 1);
                        size_t index_to_decrease = distrib(gen);

                        inner_list[index_to_decrease].freq_inner--;

                        if (inner_list[index_to_decrease].freq_inner == 0) {
                            inner_list.erase(inner_list.begin() + index_to_decrease);
                        }
                    }
                }

            }

            for (uint32_t key : keys_to_remove) {
                outer_map.erase(key);
            }
        }
    }
}


void TwoDMisraGries::update_inner_list(std::vector<InnerStruct>& list, uint32_t y) {

    for (auto& inner_ : list) {
        if (inner_.key_inner == y) {
            inner_.freq_inner++;
            return;
        }
    }

    if (list.size() < s2) {
        InnerStruct new_inner;
        new_inner.key_inner = y;
        new_inner.freq_inner = 1;
        list.push_back(new_inner);
        return;
    }

    std::vector<size_t> indices_to_remove;
    for (size_t i = 0; i < list.size(); ++i) {
        list[i].freq_inner--;
        if (list[i].freq_inner == 0) {
            indices_to_remove.push_back(i);
        }
    }

    std::sort(indices_to_remove.rbegin(), indices_to_remove.rend());
    for (size_t index : indices_to_remove) {
        list.erase(list.begin() + index);
    }
}





/**
 * @brief to retrieve heavy hitters and their quadratic elements.
 * @return A pair of maps.
 * 1st map stores heavy hitters: flow ID (uint32_t) -> estimated frequency (uint32_t).
 * 2nd map stores hot quadratic elements for each heavy hitter:
 * The key is the heavy hitter's flow ID (uint32_t), and the value is another map.
 * The inner map stores element ID (uint32_t) -> frequency (uint32_t).
 */
std::pair<std::map<uint32_t, uint32_t>,
        std::map<uint32_t, std::map<uint32_t, uint32_t>>> TwoDMisraGries::query(uint32_t heavy_hitter_th, float phi) {
    std::map<uint32_t, uint32_t> heavy_hitters;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> quad_elements;

    for (auto& pair : outer_map) {
        auto x = pair.first;
        auto x_freq = pair.second.freq_outer;

        if ( x_freq >= heavy_hitter_th){
            heavy_hitters[x] = x_freq;

            std::vector<InnerStruct>& inner_list = pair.second.inner_list;
            for (uint32_t i = 0; i < inner_list.size(); ++i) {
                auto y = inner_list[i].key_inner;
                auto y_freq = inner_list[i].freq_inner;
                if ( y_freq >= x_freq * phi ) {
                    quad_elements[x][y] = y_freq;
                }
            }
        }
    }

    return {heavy_hitters, quad_elements};
}




void TwoDMisraGries::evaluation(const std::vector<std::pair<uint32_t, uint32_t>> &dataset,
                          const std::map<uint32_t, uint32_t> &flows,
                          std::map<uint32_t, std::map<uint32_t, uint32_t>> quadratic_eles,
                          uint32_t heavy_hitter_th, float ele_th_phi) {

    std::cout << "\n" << "2D-MG:" << std::endl;

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