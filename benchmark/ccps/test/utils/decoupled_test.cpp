#include "utils/decoupled.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <iostream>

using namespace CCPS;

TEST(DecoupledTests, ChainOneTest) {
    Decoupled<UInt> upper_stream;
    Decoupled<UInt> lower_stream{true};

    unsigned val = 0x55;
    bool valid = true;
    bool ready = true;

    Wire<UInt>::TPFUNC upper_stream_bits_func{
        [&val]() -> UInt {
            return UInt(8, val);
        }
    };
    upper_stream.assignBits(upper_stream_bits_func);

    Wire<Bool>::TPFUNC upper_stream_valid_func{
        [&valid]() -> Bool {
            return Bool(valid);
        }
    };
    upper_stream.assignValid(upper_stream_valid_func);

    Wire<Bool>::TPFUNC lower_stream_ready_func {
        [&ready]() -> Bool {
            return Bool(ready);
        }
    };
    lower_stream.assignReady(lower_stream_ready_func);

    lower_stream.connect(upper_stream);
    EXPECT_EQ_BigUInt(upper_stream.bits(), val) << "Decoupled connection upper stream bits failed.";
    EXPECT_EQ_BigUInt(lower_stream.bits(), val) << "Decoupled connection lower stream bits failed.";
    EXPECT_EQ_BOOL(valid, lower_stream.isValid()) << "Decoupled connection lower stream valid failed.";
    EXPECT_EQ_BOOL(ready, upper_stream.isReady()) << "Decoupled connection upper stream ready failed.";

    val = 0xAA;
    valid = false;
    ready = true;

    EXPECT_EQ_BigUInt(lower_stream.bits(), val) << "Decoupled connection lower stream bits failed.";
    EXPECT_EQ_BOOL(valid, lower_stream.isValid()) << "Decoupled connection lower stream valid failed.";
    EXPECT_EQ_BOOL(ready, upper_stream.isReady()) << "Decoupled connection upper stream ready failed.";

    val = 0x00;
    valid = true;
    ready = false;

    EXPECT_EQ_BigUInt(lower_stream.bits(), val) << "Decoupled connection lower stream bits failed.";
    EXPECT_EQ_BOOL(valid, lower_stream.isValid()) << "Decoupled connection lower stream valid failed.";
    EXPECT_EQ_BOOL(ready, upper_stream.isReady()) << "Decoupled connection upper stream ready failed.";
}

TEST(DecoupledTests, assignBool) {
    bool valid = false;
    BigUInt bits = 0;
    bool ready = false;

    Decoupled<UInt> d0;

    d0.assignValid(valid);
    d0.assignBits(8, bits);
    d0.assignReady(ready);

    valid = false;
    bits = 0;
    ready = false;
    EXPECT_EQ_BOOL(d0.isValid(), valid);
    EXPECT_EQ_BigUInt(d0.bits(), bits);
    EXPECT_EQ_BOOL(d0.isReady(), ready);

    valid = true;
    bits = 30;
    ready = true;
    EXPECT_EQ_BOOL(d0.isValid(), valid);
    EXPECT_EQ_BigUInt(d0.bits(), bits);
    EXPECT_EQ_BOOL(d0.isReady(), ready);

    valid = true;
    bits = 100;
    ready = false;
    EXPECT_EQ_BOOL(d0.isValid(), valid);
    EXPECT_EQ_BigUInt(d0.bits(), bits);
    EXPECT_EQ_BOOL(d0.isReady(), ready);
}

TEST(DecoupledTests, assignBool2) {
    Decoupled<UInt> d0;

    d0.assignValid(false);
    d0.assignBits(8, 10);
    d0.assignReady(true);

    EXPECT_EQ_BOOL(d0.isValid(), false);
    EXPECT_EQ_BigUInt(d0.bits(), 10);
    EXPECT_EQ_BOOL(d0.isReady(), true);
}

Wire<UInt>::TPFUNC getBitsFunc(BigUInt &val) {
    return [&val]() -> UInt {
        return UInt(8, val);
    };
}

TEST(DecoupledTests, TPFUNCTest) {
    Decoupled<UInt> d0;
    BigUInt val = 0x1;

    d0.assignValid(false);
    d0.assignBits(getBitsFunc(val));
    d0.assignReady(true);

    EXPECT_EQ_BOOL(d0.isValid(), false);
    EXPECT_EQ_BigUInt(d0.bits(), val);
    EXPECT_EQ_BOOL(d0.isReady(), true);

    val = 0x5;
    EXPECT_EQ_BigUInt(d0.bits(), val);

    val = 0x55;
    EXPECT_EQ_BigUInt(d0.bits(), val);
}

TEST(DecoupledTests, AssignRegTest) {
    Decoupled<UInt> d0;

    RegPtr<Bool> reg_valid = std::make_shared<Reg<Bool>>(Bool(false));
    RegPtr<Bool> reg_ready = std::make_shared<Reg<Bool>>(Bool(true));
    RegPtr<UInt> reg = std::make_shared<Reg<UInt>>(UInt(8, 0x55));

    d0.assignValid(reg_valid);
    d0.assignBits(reg);
    d0.assignReady(reg_ready);
    EXPECT_EQ_BOOL(d0.isValid(), false);
    EXPECT_EQ_BOOL(d0.isReady(), true);
    EXPECT_EQ_BigUInt(d0.bits(), 0x55);

    *reg_valid = Bool(true);
    *reg = UInt(8, 0xAA);
    *reg_ready = Bool(false);
    reg_valid->update();
    reg->update();
    reg_ready->update();
    EXPECT_EQ_BOOL(d0.isValid(), true);
    EXPECT_EQ_BOOL(d0.isReady(), false);
    EXPECT_EQ_BigUInt(d0.bits(), 0xAA);
}
