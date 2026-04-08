#pragma once
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <set>

namespace risc
{

    struct FieldMeta
    {
        int min_size;
        bool is_var;
        int safe_size;
    };

    class Encoder
    {
    public:
        void encode(const nlohmann::json& input, nlohmann::ordered_json& output);

    private:
        int parse_length(const nlohmann::json& input) const;
        const nlohmann::json& get_json_array(const nlohmann::json& object, const std::string& key) const;
        void validate_group(const nlohmann::json& group) const;
        size_t get_max_insns(const nlohmann::json& groups) const;
        std::map<std::string, FieldMeta> parse_field_specs(const nlohmann::json& field_defs_json) const;
        void update_safe_sizes(const nlohmann::json& groups,
                               std::map<std::string, FieldMeta>& specs,
                               int total_len,
                               int sys_bits) const;
        std::map<std::string, std::pair<int, int>> build_global_layout(
            const nlohmann::json& groups,
            const std::map<std::string, FieldMeta>& specs,
            int total_len,
            int sys_bits) const;
        std::vector<std::pair<std::string, std::pair<int, int>>> get_active_operands(
            const nlohmann::json& group,
            const std::map<std::string, std::pair<int, int>>& global_layout) const;
        void append_instruction_entry(nlohmann::ordered_json& result,
                                      const nlohmann::json& group,
                                      size_t g_idx,
                                      size_t i_idx,
                                      const std::map<std::string, std::pair<int, int>>& global_layout,
                                      int total_len,
                                      int sys_bits,
                                      int f_size,
                                      int op_size) const;
    };

    int get_required_bits(size_t n);
    std::ofstream create_file(nlohmann::ordered_json& output_json, const char* file_name);
    std::string to_binary(int value, int width);
    bool is_variadic_field(const std::string& raw_value);
    int parse_field_size(const std::string& raw_value);

}
