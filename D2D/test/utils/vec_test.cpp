//#include "utils/base_types.hpp"
//#include "test_utils.hpp"
//#include <gtest/gtest.h>
//#include <iostream>
//
//using namespace CCPS;
//
//TEST(VecTests, IndexTest) {
//    int size = 8;
//    Vec<UInt> v(size);
//    EXPECT_EQ(v.size(), size);
//
//    for (int i = 0; i < size; i++) {
//        v[i] = UInt(8, i);
//    }
//
//    for (int i = 0; i < size; i++) {
//        EXPECT_EQ_BigUInt(v[i], i);
//    }
//}