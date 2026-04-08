#include "encoder.hpp"

namespace risc
{

    int Encoder::parse_length(const nlohmann::json& input) const
    {
        if (!input.contains("length")) throw std::runtime_error("Missing 'length' key in input JSON");
        int total_len = 0;
        try 
        {
            total_len = std::stoi(input["length"].get<std::string>());
        }
         catch (const std::exception&) 
        {
            throw std::runtime_error("Invalid 'length' value: must be a valid integer");
        }
        if (total_len <= 0) throw std::runtime_error("'length' must be positive");
        return total_len;
    }

    const nlohmann::json& Encoder::get_json_array(const nlohmann::json& object, const std::string& key) const
    {
        if (!object.contains(key)) throw std::runtime_error("Missing '" + key + "' key in input JSON");
        const auto& value = object[key];
        if (!value.is_array()) throw std::runtime_error("'" + key + "' must be an array");
        return value;
    }

    void Encoder::validate_group(const nlohmann::json& group) const
    {
        if (!group.contains("insns")) throw std::runtime_error("Group missing 'insns' key");
        if (!group.contains("operands")) throw std::runtime_error("Group missing 'operands' key");

        const auto& insns = group["insns"];
        if (!insns.is_array() || insns.empty()) throw std::runtime_error("Group 'insns' must be non-empty array");

        const auto& operands = group["operands"];
        if (!operands.is_array()) throw std::runtime_error("Group 'operands' must be an array");
    }

    size_t Encoder::get_max_insns(const nlohmann::json& groups) const
    {
        size_t max_insns = 0;
        for (const auto& group : groups) 
        {
            validate_group(group);
            max_insns = std::max(max_insns, group["insns"].size());
        }
        return max_insns;
    }

    std::map<std::string, FieldMeta> Encoder::parse_field_specs(const nlohmann::json& field_defs_json) const
    {
        std::map<std::string, FieldMeta> specs;
        for (const auto& f : field_defs_json) 
        {
            if (!f.is_object() || f.empty())
                throw std::runtime_error("Field definition must be non-empty object");

            std::string name = f.begin().key();
            if (name.empty()) throw std::runtime_error("Field name cannot be empty");

            std::string raw_value = f.begin().value().get<std::string>();
            bool is_var = is_variadic_field(raw_value);
            int size = parse_field_size(raw_value);
            if (size <= 0) throw std::runtime_error("Field size must be positive for '" + name + "'");
            specs[name] = {size, is_var, size};
        }
        return specs;
    }

    void Encoder::update_safe_sizes(const nlohmann::json& groups,
                                   std::map<std::string, FieldMeta>& specs,
                                   int total_len,
                                   int sys_bits) const
    {
        std::map<std::string, int> max_surplus;
        for (auto& pair : specs) 
        {
            if (pair.second.is_var) max_surplus[pair.first] = total_len;
        }

        for (const auto& group : groups) 
        {
            validate_group(group);
            int needed = sys_bits;
            std::vector<std::string> group_vars;
            for (const std::string& op : group["operands"]) 
            {
                if (op == "code" || op == "opcode") continue;
                if (specs.find(op) == specs.end())
                    throw std::runtime_error("Operand '" + op + "' not defined in 'fields'");
                needed += specs[op].min_size;
                if (specs[op].is_var) group_vars.push_back(op);
            }
            if (needed > total_len)
                throw std::runtime_error("Group requires " + std::to_string(needed) + " bits but total length is " + std::to_string(total_len));
            int surplus = std::max(0, total_len - needed);
            for (auto& var_name : group_vars) 
            {
                max_surplus[var_name] = std::min(max_surplus[var_name], surplus);
            }
        }

        for (auto& pair : specs) 
        {
            if (pair.second.is_var) pair.second.safe_size += max_surplus[pair.first];
        }
    }

    std::map<std::string, std::pair<int, int>> Encoder::build_global_layout(
        const nlohmann::json& groups,
        const std::map<std::string, FieldMeta>& specs,
        int total_len,
        int sys_bits) const
    {
        std::map<std::string, std::pair<int, int>> global_layout;
        int operand_base = total_len - 1 - sys_bits;

        for (const auto& group : groups) 
        {
            validate_group(group);
            std::vector<std::pair<int, int>> taken;
            for (const std::string& op : group["operands"]) 
            {
                if (global_layout.count(op)) taken.push_back(global_layout.at(op));
            }

            int current_ptr = operand_base;
            for (const std::string& op : group["operands"]) 
            {
                if (op == "code" || op == "opcode" || global_layout.count(op)) continue;
                int size = specs.at(op).safe_size;
                while (true) 
                {
                    bool collision = false;
                    int msb = current_ptr;
                    int lsb = msb - size + 1;
                    for (auto& taken_range : taken) 
                    {
                        if (!(msb < taken_range.second || lsb > taken_range.first)) 
                        {
                            collision = true;
                            break;
                        }
                    }
                    if (!collision) 
                    {
                        if (lsb < 0 || msb >= total_len || msb < lsb)
                            throw std::runtime_error("Instruction exceeds bit length: field '" + op + "' with range [" + std::to_string(msb) + ":" + std::to_string(lsb) + "]");
                        global_layout[op] = {msb, lsb};
                        taken.push_back({msb, lsb});
                        current_ptr = lsb - 1;
                        break;
                    }
                    current_ptr--;
                }
            }
        }
        return global_layout;
    }

    std::vector<std::pair<std::string, std::pair<int, int>>> Encoder::get_active_operands(
        const nlohmann::json& group,
        const std::map<std::string, std::pair<int, int>>& global_layout) const
    {
        std::vector<std::pair<std::string, std::pair<int, int>>> active;
        for (const std::string& op : group["operands"]) 
        {
            if (op == "code" || op == "opcode") continue;
            if (global_layout.find(op) == global_layout.end())
                throw std::runtime_error("Operand '" + op + "' not found in global layout");
            active.push_back({op, global_layout.at(op)});
        }
        std::sort(active.begin(), active.end(), [](auto& a, auto& b) {
            return a.second.first > b.second.first;
        });
        return active;
    }

    void Encoder::append_instruction_entry(nlohmann::ordered_json& result,
                                          const nlohmann::json& group,
                                          size_t g_idx,
                                          size_t i_idx,
                                          const std::map<std::string, std::pair<int, int>>& global_layout,
                                          int total_len,
                                          int sys_bits,
                                          int f_size,
                                          int op_size) const
    {
        nlohmann::ordered_json entry;
        entry["insn"] = group["insns"][i_idx];
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();

        int bit_ptr = total_len - 1;
        fields.push_back({{"F", {{"msb", bit_ptr}, {"lsb", bit_ptr - f_size + 1}, {"value", to_binary(g_idx, f_size)}}}});
        bit_ptr -= f_size;

        std::string op_v = (group["insns"].size() == 1) ? "+" : to_binary(i_idx, op_size);
        fields.push_back({{"OPCODE", {{"msb", bit_ptr}, {"lsb", bit_ptr - op_size + 1}, {"value", op_v}}}});
        bit_ptr -= op_size;

        auto active = get_active_operands(group, global_layout);
        int res_c = 0;
        bit_ptr = total_len - 1 - sys_bits;
        for (auto& op : active) {
            if (bit_ptr > op.second.first) {
                fields.push_back({{"RES" + std::to_string(res_c++), {{"msb", bit_ptr}, {"lsb", op.second.first + 1}, {"value", std::string(bit_ptr - op.second.first, '0')}}}});
            }
            fields.push_back({{op.first, {{"msb", op.second.first}, {"lsb", op.second.second}, {"value", "+"}}}});
            bit_ptr = op.second.second - 1;
        }
        if (bit_ptr >= 0) {
            fields.push_back({{"RES" + std::to_string(res_c), {{"msb", bit_ptr}, {"lsb", 0}, {"value", std::string(bit_ptr + 1, '0')}}}});
        }

        entry["fields"] = fields;
        result.push_back(entry);
    }

    void Encoder::encode(const nlohmann::json& input, nlohmann::ordered_json& output)
    {
        int total_len = parse_length(input);
        const auto& groups = get_json_array(input, "instructions");
        const auto& field_defs_json = get_json_array(input, "fields");

        int f_size = get_required_bits(groups.size());
        int op_size = get_required_bits(get_max_insns(groups));

        auto specs = parse_field_specs(field_defs_json);
        int sys_bits = f_size + op_size;
        update_safe_sizes(groups, specs, total_len, sys_bits);
        auto global_layout = build_global_layout(groups, specs, total_len, sys_bits);

        nlohmann::ordered_json result = nlohmann::ordered_json::array();
        for (size_t g_idx = 0; g_idx < groups.size(); g_idx++) 
        {
            const auto& group = groups[g_idx];
            for (size_t i_idx = 0; i_idx < group["insns"].size(); i_idx++) 
            {
                append_instruction_entry(result, group, g_idx, i_idx, global_layout, total_len, sys_bits, f_size, op_size);
            }
        }

        output = result;
    }

}
