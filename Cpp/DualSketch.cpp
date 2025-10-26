#include "header/DualSketch.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>

DualSketch::DualSketch(float memory_kb) {

    k = 32; // 4, 8, 16, 32, 64
    m_ht_frac = 0.55; // fraction of memory for HT

    method = 2; // estimate method = {0: lower bound, 1: upper bound, 2: arithmetic mean, 3: harmonic mean}


    rand_seed = 171273612;

    // the total bits of each bucket in HT.
    // 5 fields: F, U, C, V, D; each is 32 bits
    uint32_t ht_bucket_bits = 32 + 32 + 32 + 32 + 32;

    // the total bits of each Cell in QT.
    // 3 fields: E, R, P; each is 32 bits
    const uint32_t qt_cell_bits = 32 + 32 + 32;

    float memo_kb_ht = memory_kb * m_ht_frac;
    float memo_kb_qt = memory_kb - memo_kb_ht;

    m1 = static_cast<uint32_t>(std::round(memo_kb_ht * 1024 * 8 / ht_bucket_bits));
    m2 = static_cast<uint32_t>(std::round(memo_kb_qt * 1024 * 8 / qt_cell_bits));

    heavy_table.resize(m1);
    quad_table.resize(m2);
}





DualSketch::~DualSketch() {
}


// x is flow label, y is element label, (x, y) equals (f, e)
void DualSketch::update(uint32_t x, uint32_t y) {

    uint32_t hash_val = 0;
    MurmurHash3_x86_32(&x, sizeof(x), rand_seed, &hash_val);

    uint32_t i = hash_val % m1; // bkt index in HT

    // Case 1: HT[i] is empty
    if (heavy_table[i].F == 0) {

        uint32_t min_R_value = UINT32_MAX;
        int64_t min_cell_index = -1;

        uint32_t j_start = hash_val % (m2 - k + 1); // start cell index in QT
        for (uint32_t j = j_start; j < (j_start + k) ; ++j) {
            // last accessed cell index is j_start + k - 1
            if (quad_table[j].E == 0){
                quad_table[j].E = y;
                quad_table[j].R = 1;
                quad_table[j].P = x;

                heavy_table[i].F = x;
                heavy_table[i].U = heavy_table[i].D;
                heavy_table[i].C = 1;
                heavy_table[i].V = 0;
                return;

            } else{
                // Find cell with minimum R value
                if (quad_table[j].R < min_R_value) {
                    min_R_value = quad_table[j].R;
                    min_cell_index = j;
                }
            }
        }

        // No empty cell found, perform decay on the min cell
        quad_table[min_cell_index].R--;
        if (quad_table[min_cell_index].R > 0) {
            heavy_table[i].D++;
        } else {
            uint32_t x_clear = quad_table[min_cell_index].P;

            quad_table[min_cell_index].E = y;
            quad_table[min_cell_index].R = 1;
            quad_table[min_cell_index].P = x;

            heavy_table[i].F = x;
            heavy_table[i].U = heavy_table[i].D;
            heavy_table[i].C = 1;
            heavy_table[i].V = 0;

            if (x_clear == x) return; // In theory, this should never happen, just for code robustness.

            uint32_t hash_val_tmp = 0;
            MurmurHash3_x86_32(&x_clear, sizeof(x_clear), rand_seed, &hash_val_tmp);
            uint32_t j_tmp = hash_val_tmp % (m2 - k + 1);

            for (uint32_t j = j_tmp; j < (j_tmp + k); ++j) {
                if (quad_table[j].P == x_clear)
                    return;
            }

            uint32_t idx_clear = hash_val_tmp % m1;

            heavy_table[idx_clear].F = 0;
            heavy_table[idx_clear].U = 0;
            heavy_table[idx_clear].D += heavy_table[idx_clear].C + heavy_table[idx_clear].V;
            heavy_table[idx_clear].C = 0;
            heavy_table[idx_clear].V = 0;

        }
        return;
    }


    // Case 2: HT[i] is not empty, and HT[i].F == x
    if (heavy_table[i].F == x) {
        heavy_table[i].C++;

        int64_t empty_cell_index = -1;
        int64_t min_cell_index = -1;
        uint32_t min_R_value = UINT32_MAX;

        uint32_t j_start = hash_val % (m2 - k + 1); // start cell index in QT
        for (uint32_t j = j_start; j < (j_start + k); ++j) {

            // Element y already exists in a cell
            if (quad_table[j].E == y && quad_table[j].P == x) {
                quad_table[j].R++;
                return;
            }

            // Find 1st empty cell
            if (quad_table[j].E == 0 && empty_cell_index == -1 ) {
                empty_cell_index = j;
            }

            // Find cell with minimum R value
            if (quad_table[j].E != 0 && quad_table[j].R < min_R_value) {
                min_R_value = quad_table[j].R;
                min_cell_index = j;
            }
        }

        // If the loop completes, it means element y was not found.
        if (empty_cell_index > -1) {
            // Insert the new element y into the empty cell
            quad_table[empty_cell_index].E = y;
            quad_table[empty_cell_index].R = 1;
            quad_table[empty_cell_index].P = x;
        } else {
            // No empty cell found, perform decay on the min cell
            quad_table[min_cell_index].R--;

            // If R > 0 after decay, end process
            if (quad_table[min_cell_index].R > 0) {
                return;
            }

            // If R == 0, replace the cell with the new element y
            uint32_t x_clear = quad_table[min_cell_index].P;

            // Replace with new element
            quad_table[min_cell_index].E = y;
            quad_table[min_cell_index].R = 1;
            quad_table[min_cell_index].P = x;

            // If the cleared cell belonged to the current flow (x), just replace it
            if (x_clear == x) {
                return;
            }

            // If the cleared cell belonged to another flow (x'), check for consistency
            uint32_t hash_val_tmp = 0;
            MurmurHash3_x86_32(&x_clear, sizeof(x_clear), rand_seed, &hash_val_tmp);
            uint32_t j_tmp = hash_val_tmp % (m2 - k + 1);

            for (uint32_t j = j_tmp; j < (j_tmp + k); ++j) {
                if (quad_table[j].P == x_clear) {
                    return;
                }
            }

            uint32_t idx_clear = hash_val_tmp % m1;

            // Kick out the old flow
            heavy_table[idx_clear].F = 0;
            heavy_table[idx_clear].U = 0;
            heavy_table[idx_clear].D += heavy_table[idx_clear].C + heavy_table[idx_clear].V;
            heavy_table[idx_clear].C = 0;
            heavy_table[idx_clear].V = 0;
        }

    }

    // Case 3: HT[i] is not empty, but HT[i].F != x
    else {
        heavy_table[i].C--;
        heavy_table[i].V++;

        // If C > 0 after decay, drop the packet
        if (heavy_table[i].C > 0) {
            heavy_table[i].D++;
            return;
        }

        // If C == 0, kick out the old flow

        uint32_t x_clear = heavy_table[i].F;

        heavy_table[i].F = 0;
        heavy_table[i].U = 0;
        heavy_table[i].C = 0;
        heavy_table[i].D += heavy_table[i].V;
        heavy_table[i].V = 0;

        // Clear all elements of the old x from QT
        uint32_t hash_val_clear = 0;
        MurmurHash3_x86_32(&x_clear, sizeof(x_clear), rand_seed, &hash_val_clear);
        uint32_t j_clear = hash_val_clear % (m2 - k + 1);
        for (uint32_t j = j_clear; j < (j_clear + k); ++j) {
            if (quad_table[j].P == x_clear) {
                quad_table[j].E = 0;
                quad_table[j].R = 0;
                quad_table[j].P = 0;
            }
        }

        // drop the arriving (x,y)
        heavy_table[i].D++;
    }

}





/**
 * @brief Queries the DualSketch to retrieve heavy hitters and their heavy quadratic elements.
 * @return A pair of maps.
 * 1st map stores heavy hitters: ID (uint32_t) -> estimated frequency (uint32_t).
 * 2nd map stores hot quadratic elements for each heavy hitter:
 * The key is the heavy hitter's ID (uint32_t), and the value is another map.
 * The inner map stores element (uint32_t) -> frequency (uint32_t).
 */
std::pair<std::map<uint32_t, uint32_t>,
        std::map<uint32_t, std::map<uint32_t, uint32_t>>> DualSketch::query(uint32_t heavy_hitter_th) {
    std::map<uint32_t, uint32_t> heavy_hitters;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> quad_elements;

    for (uint32_t i = 0; i < m1; ++i) {
        if (heavy_table[i].F != 0) {
            uint32_t x = heavy_table[i].F;

            // the lower bound
            uint32_t lower_bound_hh = heavy_table[i].C + heavy_table[i].V;

            // the upper bound
            uint32_t upper_bound_hh = heavy_table[i].U + heavy_table[i].C + heavy_table[i].V;

            // the size for heavy hitter
            uint32_t heavy_hitter_size = 0;
            // estimate method = {0: lower bound, 1: upper bound, 2: arithmetic mean, 3: harmonic mean}
            switch (method) {
                case 0: // lower bound
                    heavy_hitter_size = lower_bound_hh;
                    break;

                case 1: // upper bound
                    heavy_hitter_size = upper_bound_hh;
                    break;

                case 2: // arithmetic mean
                    heavy_hitter_size = (lower_bound_hh + upper_bound_hh) / 2;
                    break;

                default: // harmonic mean
                    if (lower_bound_hh + upper_bound_hh != 0) {
                        heavy_hitter_size =
                                static_cast<uint32_t>((2LL * lower_bound_hh * upper_bound_hh) /
                                                      (lower_bound_hh + upper_bound_hh));
                    }
                    break;
            }

            if (heavy_hitter_size >= heavy_hitter_th) {
                heavy_hitters[x] = heavy_hitter_size;

                // Iterate through the k cells to find the quadratic elements
                uint32_t hash_val = 0;
                MurmurHash3_x86_32(&x, sizeof(x), rand_seed, &hash_val);
                uint32_t j_start = hash_val % (m2 - k + 1);
                std::map<uint32_t, uint32_t> current_quad_elements;
                for (uint32_t j = j_start; j < (j_start + k); ++j) {
                    // Check if the cell belongs to the current heavy hitter
                    if (quad_table[j].E != 0 && quad_table[j].P == x) {
                        current_quad_elements[quad_table[j].E] = quad_table[j].R;
                    }
                }
                quad_elements[x] = current_quad_elements;
            }
        }
    }

    return {heavy_hitters, quad_elements};
}


void DualSketch::evaluation(const std::vector<std::pair<uint32_t, uint32_t>> &dataset,
                            const std::map<uint32_t, uint32_t> &flows,
                            std::map<uint32_t, std::map<uint32_t, uint32_t>> quadratic_eles,
                            uint32_t heavy_hitter_th, float ele_th_phi) {

    std::cout << "\n" << "DualSketch:" << std::endl;

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
    auto [queried_heavy_hitters, queried_quad_elements] = query(heavy_hitter_th);
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

    // get queried hot quadratic elements
    std::map<uint32_t, std::map<uint32_t, uint32_t>> queried_hot_quad_elements;
    for (const auto &[flow_id, queried_elements]: queried_quad_elements) {
        uint32_t queried_flow_size = queried_heavy_hitters[flow_id];
        for (const auto &[ele_id, queried_ele_size]: queried_elements){
            if (queried_ele_size >= ele_th_phi * queried_flow_size)
            {
                queried_hot_quad_elements[flow_id][ele_id] = queried_ele_size;
                total_queried_hot_ele_count += 1;
            }
        }
    }


    // Calculate ARE and True Positives
    for (const auto &[flow_id, true_hot_elements]: true_hot_quad_elements) {
        if (queried_hot_quad_elements.count(flow_id)) {
            const auto &queried_hot_elements = queried_hot_quad_elements.at(flow_id);
            for (const auto &[ele_id, true_size]: true_hot_elements) {
                if (queried_hot_elements.count(ele_id)) {
                    uint32_t queried_size = queried_hot_elements.at(ele_id);
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
