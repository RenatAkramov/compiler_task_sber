#include "encoder.hpp"

namespace risc
{
    std::ofstream create_file(nlohmann::ordered_json& output_json, const char* file_name)
    {
        std::ofstream output_file(file_name);
        output_file << "[\n";
        for (size_t i = 0; i < output_json.size(); i++) {
            output_file << "  {\n";
            output_file << "    \"insn\": " << output_json[i]["insn"].dump() << ",\n";
            output_file << "    \"fields\": [\n";
            auto& fields = output_json[i]["fields"];
            for (size_t j = 0; j < fields.size(); j++) {
                output_file << "      " << fields[j].dump() << (j < fields.size() - 1 ? ",\n" : "\n");
            }
            output_file << "    ]\n  }" << (i < output_json.size() - 1 ? ",\n" : "\n");
        }
        output_file << "]\n";
        return output_file;
    }

    int get_required_bits(size_t n)
    {
        if (n <= 1) return 1;
        return static_cast<int>(std::ceil(std::log2(n)));
    }

    bool is_variadic_field(const std::string& raw_value)
    {
        return raw_value.rfind(">=", 0) == 0;
    }

    int parse_field_size(const std::string& raw_value)
    {
        if (raw_value.empty()) throw std::runtime_error("Field size string is empty");
        std::string id = is_variadic_field(raw_value) ? raw_value.substr(2) : raw_value;
        try {
            return std::stoi(id);
        } catch (const std::exception& e) {
            throw std::runtime_error("Invalid field size: " + std::string(e.what()));
        }
    }

    std::string to_binary(int value, int width)
    {
        std::string s;
        s.reserve(width);
        for (int i = width - 1; i >= 0; --i) {
            s += ((value >> i) & 1) ? '1' : '0';
        }
        return s;
    }
}
