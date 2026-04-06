#include "encoder.hpp"

int main(int argc, char* argv[]) 
{
    if (argc < 3) 
    {
        std::cerr << "Usage: " << argv[0] << " <input.json> <output.json>" << std::endl;
        return 1;
    }

    try 
    {
        std::ifstream input_file(argv[1]);
        if (!input_file.is_open()) throw std::runtime_error("Cannot open input file.");
        
        nlohmann::json input_json;
        input_file >> input_json;

        risc::Encoder encoder;
        nlohmann::ordered_json output_json;
        encoder.encode(input_json, output_json);

        std::ofstream output_file(argv[2]);
        output_file << "[\n";
        for (size_t i = 0; i < output_json.size(); ++i) 
        {
            output_file << "  {\n";
            output_file << "    \"insn\": " << output_json[i]["insn"].dump() << ",\n";
            output_file << "    \"fields\": [\n";
            auto& fields = output_json[i]["fields"];
            for (size_t j = 0; j < fields.size(); ++j) 
            {
                output_file << "      " << fields[j].dump() << (j < fields.size() - 1 ? ",\n" : "\n");
            }
            output_file << "    ]\n  }" << (i < output_json.size() - 1 ? ",\n" : "\n");
        }
        output_file << "]\n";

        std::cout << "Success! Encoded instructions saved to " << argv[2] << std::endl;

    } 
    catch (const std::exception& e) 
    {
        std::cerr << "\033[1;31mError:\033[0m " << e.what() << std::endl;
        return 1;
    }

    return 0;
}