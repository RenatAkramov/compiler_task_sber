#include <gtest/gtest.h>
#include "encoder.hpp"

using namespace risc;

TEST(EncoderMathTest, RequiredBitsCalculation)
{
    EXPECT_EQ(get_required_bits(1), 1);
    EXPECT_EQ(get_required_bits(2), 1);
    EXPECT_EQ(get_required_bits(4), 2);
    EXPECT_EQ(get_required_bits(5), 3);
    EXPECT_EQ(get_required_bits(18), 5);
}

TEST(EncoderLogicTest, FieldUniformity) 
{
    Encoder encoder;
    nlohmann::json input = {
        {"length", "32"},
        {"fields", {{{"rd", "5"}}, {{"rs1", "5"}}}},
        {"instructions", {
            {
                {"format", "R"},
                {"insns", {"add"}},
                {"operands", {"rd", "rs1"}}
            },
            {
                {"format", "I"},
                {"insns", {"addi"}},
                {"operands", {"rd"}}
            }
        }}
    };

    nlohmann::ordered_json output;
    encoder.encode(input, output);


    auto rd_add = output[0]["fields"][2]["rd"];

    auto rd_addi = output[1]["fields"][2]["rd"];

    EXPECT_EQ(rd_add["msb"], rd_addi["msb"]);
    EXPECT_EQ(rd_add["lsb"], rd_addi["lsb"]);
}


TEST(EncoderLogicTest, SafeSurplusCalculation) 
{
    Encoder encoder;
    nlohmann::json input = {
        {"length", "16"},
        {"fields", {{{"imm", ">=4"}}}},
        {"instructions", {
            {
                {"format", "A"},
                {"insns", {"op1"}},
                {"operands", {"imm"}}

            }
        }}
    };

    nlohmann::ordered_json output;
    encoder.encode(input, output);

    auto find_field = [&](const std::string& name) -> nlohmann::ordered_json 
    {
        for (auto& f : output[0]["fields"])
        {
            if (f.contains(name)) return f[name];
        }
        throw std::runtime_error("Field not found: " + name);
    };

    auto imm = find_field("imm");
    EXPECT_EQ(imm["msb"], 13);
    EXPECT_EQ(imm["lsb"], 0);
    EXPECT_EQ(imm["msb"].get<int>() - imm["lsb"].get<int>() + 1, 14);
}


TEST(EncoderLogicTest, CollisionDetection) 
{
    Encoder encoder;

    nlohmann::json input = {
        {"length", "8"},
        {"fields", {{{"big_field", "10"}}}},
        {"instructions", {
            {
                {"format", "ERR"},
                {"insns", {"fail"}},
                {"operands", {"big_field"}}
            }
        }}
    };

    nlohmann::ordered_json output;

    EXPECT_THROW(encoder.encode(input, output), std::runtime_error);
}


TEST(EncoderLogicTest, ReservoirInsertion) 
{
    Encoder encoder;
    nlohmann::json input = {
        {"length", "32"},
        {"fields", {{{"rd", "5"}}, {{"rs2", "5"}}}},
        {"instructions", {
            {
                {"format", "GAP"},
                {"insns", {"jump"}},
                {"operands", {"rs2"}}
            }
        }}
    };

    nlohmann::ordered_json output;
    encoder.encode(input, output);

    bool res_found = false;
    for (auto& f : output[0]["fields"]) 
    {

        if (f.begin().key().find("RES") != std::string::npos) 
        {
            res_found = true;
        }
    }
    EXPECT_TRUE(res_found);
}



int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
