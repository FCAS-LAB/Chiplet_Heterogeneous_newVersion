#include "utils/base_types.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>


using namespace CCPS;



TEST(UIntTests, AppendTest) {
    unsigned v0{5};
    unsigned v1{3};

    UInt d0{8, v0};
    UInt d1{8, v1};

    d0.append(d1);

    unsigned res = (v0<<8) | v1;

    EXPECT_EQ_BigUInt(d0, res) << "uint append mismatch, in ";

}

TEST(UIntTests, ToBoolTest) {
    unsigned v0{5};
    unsigned v1{0};

    UInt d0{8, v0};
    UInt d1{8, v1};

    EXPECT_EQ_BOOL(d0, true);
    EXPECT_EQ_BOOL(d1, false);

    if (d0) {
        //std::cout << "d0 is true" << std::endl;
    } else {
        std::cout << "d0 is false" << std::endl;
        EXPECT_EQ(1, 0);
    }
    if (d1) {
        std::cout << "d1 is true" << std::endl;
        EXPECT_EQ(1, 0);
    } else {
        //std::cout << "d1 is false" << std::endl;
    }

}
