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

    struct FieldSpec 
    {
        int  min_size;
        bool is_var;
        int  msb;
        int  lsb;
    };

    class Encoder 
    {
        public:
            void encode(const nlohmann::json& input, nlohmann::ordered_json& output);

        private:
            std::string to_binary(int value, int width);
    };

} 