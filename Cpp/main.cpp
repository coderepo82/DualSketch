
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <utility>
#include <iomanip>
#include <map>
#include <set>
#include <cstdint>
#include "header/GlobalHH.h"
#include "header/TwoDMisraGries.h"


#ifdef _WIN32
//#include <winsock2.h>
#include <optional>
#include "header/DualSketch.h"
#include "header/DUET.h"
#include "header/CSSCHH.h"

#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif


/**
 * @brief Loads the CAIDA 2019 dataset from specified files.
 * @return A tuple containing three collections:
 * 1. std::vector<std::pair<uint32_t, uint32_t>>: The raw dataset of (source_ip, dest_ip) pairs.
 * 2. std::map<uint32_t, uint32_t>: The true count for each flow (source IP).
 * 3. std::map<uint32_t, std::map<uint32_t, uint32_t>>: The true count for each element (dest IP)
 * within each flow (source IP).
 */
std::tuple<std::vector<std::pair<uint32_t, uint32_t>>,
        std::map<uint32_t, uint32_t>,
        std::map<uint32_t, std::map<uint32_t, uint32_t>>> loadDataSetCAIDA() {
    std::vector<std::string> file_paths = {
            "./dataset/CAIDA2019/file1.txt",
            "./dataset/CAIDA2019/file2.txt",
            // your dataset file list
    };

    std::vector<std::pair<uint32_t, uint32_t>> data;
    std::map<uint32_t, uint32_t> true_flow_size;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> true_element_size;
    uint64_t total_data_num = 0;

    for (const auto &file_path: file_paths) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << file_path << std::endl;
            continue;
        }

        std::string line;
        while (getline(file, line)) {
            std::istringstream iss(line);
            std::string source_ip, dest_ip;
            iss >> source_ip >> dest_ip;
            if (source_ip.empty() || dest_ip.empty()) continue;

            uint32_t src_ip_int = 0, dst_ip_int = 0;
            int part = 0;
            char dot;
            std::istringstream ss1(source_ip), ss2(dest_ip);
            bool valid = true;

            for (int i = 0; i < 4; i++) {
                if (!(ss1 >> part)) {
                    valid = false;
                    break;
                }
                if (part < 0 || part > 255) {
                    valid = false;
                    break;
                }
                src_ip_int = (src_ip_int << 8) | part;
                if (i < 3 && !(ss1 >> dot && dot == '.')) {
                    valid = false;
                    break;
                }
            }
            if (ss1 >> dot) valid = false;

            for (int i = 0; i < 4 && valid; i++) {
                if (!(ss2 >> part)) {
                    valid = false;
                    break;
                }
                if (part < 0 || part > 255) {
                    valid = false;
                    break;
                }
                dst_ip_int = (dst_ip_int << 8) | part;
                if (i < 3 && !(ss2 >> dot && dot == '.')) {
                    valid = false;
                    break;
                }
            }
            if (ss2 >> dot) valid = false;

            if (!valid) continue;

            if (src_ip_int == 0 || dst_ip_int == 0) continue;

            data.emplace_back(src_ip_int, dst_ip_int);
            true_flow_size[src_ip_int]++;
            true_element_size[src_ip_int][dst_ip_int]++;
            total_data_num++;
        }

        std::cout << file_path << " is loaded." << std::endl;
    }

    std::cout << file_paths.size() << " data loaded.\n"
              << "Totaling packets: " << total_data_num
              << ", Unique flows (src_ip as flow label): " << true_flow_size.size() << std::endl;

    return std::make_tuple(data, true_flow_size, true_element_size);
}


/**
 * @brief Loads the MAWI dataset from specified files.
 * @return A tuple containing three collections:
 * 1. std::vector<std::pair<uint32_t, uint32_t>>: The raw dataset of (source_ip, dest_ip) pairs.
 * 2. std::map<uint32_t, uint32_t>: The true count for each flow (source IP).
 * 3. std::map<uint32_t, std::map<uint32_t, uint32_t>>: The true count for each element (dest IP)
 * within each flow (source IP).
 */
std::tuple<
        std::vector<std::pair<uint32_t, uint32_t>>,
        std::map<uint32_t, uint32_t>,
        std::map<uint32_t, std::map<uint32_t, uint32_t>>
> loadDataSetMAWI() {
    std::vector<std::string> file_paths = {
            // your dataset file list
            "./dataset/MAWI2024/parsed_mawi_2024.csv",
    };

    std::vector<std::pair<uint32_t, uint32_t>> data;
    std::map<uint32_t, uint32_t> true_flow_size;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> true_element_size;
    uint64_t total_data_num = 0;

    auto parse_ip = [](const std::string &ip_str) -> std::optional<uint32_t> {
        std::stringstream ss(ip_str);
        uint32_t result = 0;
        int part;

        for (int i = 0; i < 4; ++i) {
            if (!(ss >> part)) return std::nullopt;
            if (part < 0 || part > 255) return std::nullopt;
            result = (result << 8) | part;
            if (i < 3) {
                if (ss.peek() != '.') return std::nullopt;
                ss.ignore();
            }
        }

        if (ss.rdbuf()->in_avail() != 0) return std::nullopt;
        return result;
    };

    for (const auto &file_path: file_paths) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << file_path << std::endl;
            continue;
        }

        std::string line;
        if (!std::getline(file, line)) {
            std::cerr << "Empty file: " << file_path << std::endl;
            continue;
        }

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string src_ip_str, dst_ip_str;

            // Assumes field order is: src_ip,dst_ip,...
            if (!std::getline(ss, src_ip_str, ',')) continue;
            if (!std::getline(ss, dst_ip_str, ',')) continue;

            auto src_ip_opt = parse_ip(src_ip_str);
            auto dst_ip_opt = parse_ip(dst_ip_str);
            if (!src_ip_opt || !dst_ip_opt) continue;

            uint32_t src_ip = src_ip_opt.value();
            uint32_t dst_ip = dst_ip_opt.value();

            if (src_ip == 0 || dst_ip == 0) continue;

            data.emplace_back(src_ip, dst_ip);
            true_flow_size[src_ip]++;
            true_element_size[src_ip][dst_ip]++;
            total_data_num++;
        }

        std::cout << "Loaded file: " << file_path << std::endl;
    }

    std::cout << "Total data: " << total_data_num;
    std::cout << ", Unique flows (src_ip as flow ID): " << true_flow_size.size() << std::endl;

    return std::make_tuple(data, true_flow_size, true_element_size);
}


std::tuple<std::vector<std::pair<uint32_t, uint32_t>>,
        std::map<uint32_t, uint32_t>,
        std::map<uint32_t, std::map<uint32_t, uint32_t>>> loadDatasetFreqItemMining() {

    std::vector<std::string> file_paths = {
            "./dataset/Frequent Itemset Mining Dataset Repository/kosarak.dat",
            "./dataset/Frequent Itemset Mining Dataset Repository/pumsb.dat",
            "./dataset/Frequent Itemset Mining Dataset Repository/pumsb_star.dat",
            "./dataset/Frequent Itemset Mining Dataset Repository/T10I4D100K.dat",
            "./dataset/Frequent Itemset Mining Dataset Repository/T40I10D100K.dat",
            "./dataset/Frequent Itemset Mining Dataset Repository/webdocs.dat",
    };

    std::vector<std::pair<uint32_t, uint32_t>> data;
    std::map<uint32_t, uint32_t> flow_true_count;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> flow_element_true_count;

    uint64_t total_data_num = 0;

    for (const auto &file_path: file_paths) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << file_path << std::endl;
            continue;
        }

        std::string line;
        uint32_t line_count = 0;

        while (getline(file, line)) {
            line_count++;
            if (line.empty() || line.find_first_not_of(" \t\n\r") == std::string::npos) {
                continue;
            }

            std::istringstream iss(line);
            std::vector<uint32_t> numbers;
            uint32_t number;

            while (iss >> number) {
                numbers.push_back(number);
            }

            if (numbers.size() < 1) {
                continue;
            }

            if (numbers.size() < 2) {
                numbers[0] = (rand() % 32767) + 1;
            }

            try {
                uint32_t flow_id = numbers[0] + 1;
                uint32_t ele_id = numbers[1] + 1;

                if (flow_id == 0 || ele_id == 0) continue;

                data.emplace_back(flow_id, ele_id);
                flow_true_count[flow_id]++;
                flow_element_true_count[flow_id][ele_id]++;

                total_data_num++;
            } catch (const std::exception &e) {
                std::cerr << "Skipping invalid line " << line_count << " in " << file_path << ": " << line << " ("
                          << e.what() << ")" << std::endl;
                continue;
            }
        }
    }

    std::cout << "All data is loaded, totaling data: " << total_data_num << std::endl;
    std::cout << "Unique flows: " << flow_true_count.size() << std::endl;

    return std::make_tuple(data, flow_true_count, flow_element_true_count);
}


std::tuple<std::vector<std::pair<uint32_t, uint32_t>>,
        std::map<uint32_t, uint32_t>,
        std::map<uint32_t, std::map<uint32_t, uint32_t>>> loadSyntheticDataset() {

    std::vector<std::string> file_paths = {
            "./dataset/SyntheticDataset/skewed_dataset_zipf01.txt"
    };

    std::vector<std::pair<uint32_t, uint32_t>> data;
    std::map<uint32_t, uint32_t> flow_true_count;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> flow_element_true_count;

    uint64_t total_data_num = 0;

    for (const auto &file_path: file_paths) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << file_path << '\n';
            continue;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key_str, ele_str;
            if (!(iss >> key_str >> ele_str)) continue;

            try {
                uint32_t key_int = static_cast<uint32_t>(std::stoul(key_str)) + 1;
                uint32_t ele_int = static_cast<uint32_t>(std::stoul(ele_str)) + 1;

                if (key_int == 0 || ele_int == 0) continue;

                data.emplace_back(key_int, ele_int);

                flow_true_count[key_int]++;
                flow_element_true_count[key_int][ele_int]++;

                ++total_data_num;
            }
            catch (const std::exception &e) {
                std::cerr << "Parse error in file " << file_path
                          << " line: \"" << line << "\" — " << e.what() << '\n';
            }
        }
        std::cout << file_path << " is loaded.\n";
    }

    std::cout << file_paths.size() << " files data loaded.\n"
              << "Totaling: " << total_data_num
              << ", Unique flows: " << flow_true_count.size() << std::endl;

    return std::make_tuple(data, flow_true_count, flow_element_true_count);
}


int main() {

    std::cout << "Experiment starts ..." << std::endl;
    auto start_time = std::chrono::steady_clock::now();

    auto [dataset, flows, quadratic_eles] = loadDataSetCAIDA();
//    auto [dataset, flows, quadratic_eles] = loadDataSetMAWI();
//    auto [dataset, flows, quadratic_eles] = loadDatasetFreqItemMining();
//    auto [dataset, flows, quadratic_eles] = loadSyntheticDataset();

    std::vector<float> heavy_hitter_th_values = {0.0001}; // phi_1
    std::vector<float> quad_ele_th_values = {0.1}; // phi_2
    std::vector<uint32_t> memo_kb_values = {100, 200, 300, 400}; // memory in KB

    for (float hh_th_ratio: heavy_hitter_th_values) {
        for (float ele_th_phi: quad_ele_th_values) {
            for (uint32_t memo_kb: memo_kb_values) {

                uint32_t heavy_hitter_th = hh_th_ratio * dataset.size();

                std::cout << "\nHeavy hitter th = " << heavy_hitter_th
                          << " (N*phi), phi_1 = " << hh_th_ratio
                          << ", Quad element th (phi_2) = " << ele_th_phi
                          << ", memo_kb = " << memo_kb
                          << std::endl;

                auto *dualSketch = new DualSketch(memo_kb);
                dualSketch->evaluation(dataset, flows, quadratic_eles, heavy_hitter_th, ele_th_phi);
                delete dualSketch;

                auto *duet = new DUET(memo_kb);
                duet->evaluation(dataset, flows, quadratic_eles, heavy_hitter_th, ele_th_phi);
                delete duet;

                auto *global_hh = new GlobalHH(memo_kb);
                global_hh->evaluation(dataset, flows, quadratic_eles, heavy_hitter_th, ele_th_phi);
                delete global_hh;

                auto *twod_mg = new TwoDMisraGries(memo_kb);
                twod_mg->evaluation(dataset, flows, quadratic_eles, heavy_hitter_th, ele_th_phi);
                delete twod_mg;

                auto *css_chh = new CSSCHH(memo_kb);
                css_chh->evaluation(dataset, flows, quadratic_eles, heavy_hitter_th, ele_th_phi);
                delete css_chh;

            }
        }
    }


    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    std::cout << "\nExperiment ends. Elapsed time: "
              << elapsed_time / 3600 << "h " << (elapsed_time % 3600) / 60 << "m " << elapsed_time % 60 << "s"
              << std::endl;

    return 0;
}

