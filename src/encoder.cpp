#include "encoder.hpp"

namespace risc 
{

    int get_required_bits(size_t n) 
    {
        if (n <= 1) return 1;
        return static_cast<int>(std::ceil(std::log2(n)));
    }

    void Encoder::encode(const nlohmann::json& input, nlohmann::ordered_json& output) 
    {
        int total_len = std::stoi(input["length"].get<std::string>());
        auto groups = input["instructions"];
        auto field_defs_json = input["fields"];

        int f_size = get_required_bits(groups.size());
        size_t max_insns = 0;
        for (auto& g : groups) max_insns = std::max(max_insns, g["insns"].size());
        int op_size = get_required_bits(max_insns);

        struct FieldMeta { int min_size; bool is_var; int safe_size; };
        std::map<std::string, FieldMeta> specs;
        for (auto& f : field_defs_json) 
        {
            std::string name = f.begin().key();
            std::string val = f.begin().value().get<std::string>();
            bool is_var = (val.find(">=") != std::string::npos);
            int size = std::stoi(is_var ? val.substr(2) : val);
            specs[name] = {size, is_var, size};
        }

        int sys_bits = f_size + op_size;
        std::map<std::string, int> max_surplus;
        for (auto& s : specs) if (s.second.is_var) max_surplus[s.first] = total_len;

        for (auto& group : groups) 
        {
            int needed = sys_bits;
            std::vector<std::string> group_vars;
            for (std::string op : group["operands"]) 
            {
                if (op == "code" || op == "opcode") continue;
                needed += specs[op].min_size;
                if (specs[op].is_var) group_vars.push_back(op);
            }
            int surplus = std::max(0, total_len - needed);
            for (auto& v : group_vars) max_surplus[v] = std::min(max_surplus[v], surplus);
        }
        for (auto& s : specs) if (s.second.is_var) s.second.safe_size += max_surplus[s.first];

        std::map<std::string, std::pair<int, int>> global_layout;
        int operand_base = total_len - 1 - sys_bits;

        for (auto& group : groups) 
        {
            std::vector<std::pair<int, int>> taken;
            for (std::string op : group["operands"]) 
            {
                if (global_layout.count(op)) taken.push_back(global_layout[op]);
            }

            int current_ptr = operand_base;
            for (std::string op : group["operands"]) 
            {
                if (op == "code" || op == "opcode" || global_layout.count(op)) continue;

                int size = specs[op].safe_size;
                while (true) 
                {
                    bool collision = false;
                    int msb = current_ptr;
                    int lsb = msb - size + 1;
                    for (auto& t : taken) 
                    {
                        if (!(msb < t.second || lsb > t.first)) { collision = true; break; }
                    }
                    if (!collision) 
                    {
                        global_layout[op] = {msb, lsb};
                        taken.push_back({msb, lsb});
                        current_ptr = lsb - 1;
                        break;
                    }
                    current_ptr--; 
                }
            }
        }

        nlohmann::ordered_json result = nlohmann::ordered_json::array();
        for (size_t g_idx = 0; g_idx < groups.size(); ++g_idx) 
        {
            auto& group = groups[g_idx];
            for (size_t i_idx = 0; i_idx < group["insns"].size(); ++i_idx) 
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

                std::vector<std::pair<std::string, std::pair<int, int>>> active;
                for (std::string o : group["operands"]) if (o != "code" && o != "opcode") active.push_back({o, global_layout[o]});
                std::sort(active.begin(), active.end(), [](auto& a, auto& b) { return a.second.first > b.second.first; });

                int res_c = 0;
                bit_ptr = total_len - 1 - sys_bits;
                for (auto& op : active) 
                {
                    if (bit_ptr > op.second.first) 
                    {
                        fields.push_back({{"RES" + std::to_string(res_c++), {
                            {"msb", bit_ptr}, {"lsb", op.second.first + 1}, {"value", std::string(bit_ptr - op.second.first, '0')}
                        }}});
                    }
                    fields.push_back({{op.first, {{"msb", op.second.first}, {"lsb", op.second.second}, {"value", "+"}} }});
                    bit_ptr = op.second.second - 1;
                }
                if (bit_ptr >= 0) fields.push_back({{"RES" + std::to_string(res_c), {{"msb", bit_ptr}, {"lsb", 0}, {"value", std::string(bit_ptr + 1, '0')}}}});

                entry["fields"] = fields;
                result.push_back(entry);
            }
        }
        output = result;
    }

    std::string Encoder::to_binary(int value, int width) 
    {
        std::string s = "";
        for (int i = width - 1; i >= 0; --i) s += ((value >> i) & 1) ? '1' : '0';
        return s;
    }

}