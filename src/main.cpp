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
        std::ofstream output_file = risc::create_file(output_json, argv[2]);
        if (!output_file.is_open()) throw std::runtime_error("Cannot open output file.");

        std::cout << "Success! Encoded instructions saved to " << argv[2] << std::endl;

    } 
    catch (const std::exception& e) 
    {
        std::cerr << "\033[1;31mError:\033[0m " << e.what() << std::endl;
        return 1;
    }

    return 0;
}