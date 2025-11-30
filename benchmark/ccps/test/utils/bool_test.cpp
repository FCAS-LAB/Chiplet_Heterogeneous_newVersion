#include "utils/base_types.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

TEST(UIntTests, BoolToboolTest) {
    bool v0{true};
    bool v1{false};

    Bool d0{v0};
    Bool d1{v1};

    //EXPECT_EQ(d0, true) << "UInt bool operator test failed";
    //EXPECT_EQ(d1, false) << "UInt bool operator test failed";

    if (d0) {
        //std::cout << "d0 is true" << std::endl;
    } else {
        std::cout << "d0 is false" << std::endl;
        EXPECT_EQ(1, 0) << "UInt bool operator test failed";
    }
    if (d1) {
        std::cout << "d1 is true" << std::endl;
        EXPECT_EQ(1, 0) << "UInt bool operator test failed";
    } else {
        //std::cout << "d1 is false" << std::endl;
    }

}
