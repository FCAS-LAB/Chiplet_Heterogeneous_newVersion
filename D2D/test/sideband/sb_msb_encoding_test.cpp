#include "sideband/sb_msg_encoding.hpp"
#include <gtest/gtest.h>
#include <iostream>

using namespace CCPS;

TEST(SbMsbEncodingTests, SbMsbEncodingTest) {
    const UInt res{
        SBMessage_factory(
            SBM().LINK_MGMT_ADAPTER0_REQ_DISABLE,
            "Protocol_0",
            false,
            "D2D"
        )
    };
    EXPECT_EQ(72057645577584658ULL, res.toBigUInt());
}
