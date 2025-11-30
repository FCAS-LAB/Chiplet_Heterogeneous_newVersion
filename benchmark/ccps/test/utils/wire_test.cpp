#include "utils/wire.hpp"
#include <gtest/gtest.h>


using namespace CCPS;

std::function<UInt()> get_uint_func(const unsigned &val) {
    return [&val]() -> UInt {
        return UInt(8, val);
    };
}

class WireTestHelper {
public:
    Wire<UInt> in;
    Wire<UInt> out;
    unsigned val;

    WireTestHelper() {
        in = [this]() {
            return UInt(8, val);
        };
        out = in;
    }

};

TEST(WireTests, EqualConnectTest) {
    WireTestHelper wire_helper;

    wire_helper.val = 0x55;

    EXPECT_EQ(wire_helper.val, wire_helper.out().toBigUInt()) << "Wire connection mismatch, in " << wire_helper.val << " out " << wire_helper.out().toBigUInt();

    wire_helper.val = 0xAA;
    EXPECT_EQ(wire_helper.val, wire_helper.out().toBigUInt()) << "Wire connection mismatch, in " << wire_helper.val << " out " << wire_helper.out().toBigUInt();
}

TEST(WireTests, AssignBeforeInit) {
    Wire<UInt> in1;
    Wire<UInt> out1;
    int val = 0x55;

    out1 = in1;

    in1 = [&val]() -> UInt {
        return UInt(8, val);
    };


    EXPECT_EQ(val, out1().toBigUInt()) << "Wire connection mismatch, in " << val << " out " << out1().toBigUInt();

    val = 0xAA;
    EXPECT_EQ(val, out1().toBigUInt()) << "Wire connection mismatch, in " << val << " out " << out1().toBigUInt();
}

TEST(WireTests, ChainTwoTest) {
    Wire<UInt> w0, w1, w2;
    int val = 0x55;

    w2 = w1;
    w1 = w0;

    w0 = [&val]() -> UInt {
        return UInt(8, val);
    };


    EXPECT_EQ(val, w2().toBigUInt()) << "Wire chain two mismatch, in " << val << " out " << w2().toBigUInt();

    val = 0xAA;
    EXPECT_EQ(val, w2().toBigUInt()) << "Wire chain two mismatch, in " << val << " out " << w2().toBigUInt();
}

TEST(WireTests, CopyConstructorTest) {
    Wire<UInt> w0;
    Wire<UInt> w1(w0);
    Wire<UInt> w2(w1);
    int val = 0x55;

    w0 = [&val]() -> UInt {
        return UInt(8, val);
    };


    EXPECT_EQ(val, w2().toBigUInt()) << "Wire chain two mismatch, in " << val << " out " << w2().toBigUInt();

    val = 0xAA;
    EXPECT_EQ(val, w2().toBigUInt()) << "Wire chain two mismatch, in " << val << " out " << w2().toBigUInt();
}

TEST(WireTests, AssignBool) {
    Wire<Bool> w0;
    bool val = false;

    w0.capture(val);

    val = false;
    EXPECT_EQ(val, static_cast<bool>(w0()));

    val = true;
    EXPECT_EQ(val, static_cast<bool>(w0()));
}

TEST(WireTests, AssignBigUInt) {
    Wire<UInt> w0;
    BigUInt val = 0;

    w0.capture(8, val);

    val = 10;
    EXPECT_EQ(val, w0().toBigUInt());

    val = 33;
    EXPECT_EQ(val, w0().toBigUInt());
}

TEST(WireTests, AssignRegToWire) {
    Wire<UInt> w;
    RegPtr<UInt> r = std::make_shared<Reg<UInt> >(UInt(8, 0));

    w = r;

    r->write(UInt(8, 0));
    r->update();
    EXPECT_EQ(w().toBigUInt(), 0);

    r->write(UInt(8, 33));
    r->update();
    EXPECT_EQ(w().toBigUInt(), 33);
}
