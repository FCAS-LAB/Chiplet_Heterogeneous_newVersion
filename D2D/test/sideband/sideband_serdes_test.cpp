#include <gtest/gtest.h>
#include "sideband/sideband_node.hpp"
#include "interfaces/fdi.hpp"
#include "test_utils.hpp"

using namespace CCPS;

TEST(SidebandSerDesTests, SimpleSerializerSanity) {
    FdiParams fdi_params{8, 8, 32};
    SidebandParams sb_params;

    bool io_in_valid = false;
    bool io_out_credit = false;
    BigUInt io_in_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    int msg_w = sb_params.sb_node_msg_width;  // 128
    int sb_w = fdi_params.sb_width;           // 32

    auto top = createTopModule<SidebandSerializer>(sb_params, fdi_params);
    auto &c = *top;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_in_valid_func = [&io_in_valid] () -> Bool {
        return Bool(io_in_valid);
    };
    c.io.in.assignValid(io_in_valid_func);

    Wire<Bool>::TPFUNC io_out_credit_func = [&io_out_credit]() -> Bool {
        return Bool(io_out_credit);
    };
    c.io.out.credit = io_out_credit_func;

    Wire<UInt>::TPFUNC io_in_bits_func = [&msg_w, &io_in_bits]() -> UInt {
        return UInt(msg_w, io_in_bits);
    };
    c.io.in.assignBits(io_in_bits_func);

    // ======================== run ========================
    std::cout << "Test started" << std::endl;

    // init
    io_in_valid = false;
    io_out_credit = false;
    c.step();

    //Send data to serializer
    std::cout << ("Send data") << std::endl;
    io_in_valid = true;
    EXPECT_EQ_BOOL(c.io.out.valid(), false);
    c.step();

    //Check serialized data
    io_in_valid = false;
    for (int i = 0; i < msg_w / sb_w; i++) {
        BigUInt serialized_data = (io_in_bits >> i * sb_w) & ((BigUInt(1) << sb_w) - 1);
        EXPECT_EQ_BOOL(c.io.in.isReady(), false) << "c.io.in.ready mismatch, i " << i;
        EXPECT_EQ_BOOL(c.io.out.valid(), true) << "c.io.in.valid mismatch, i " << i;
        EXPECT_EQ_BigUInt(c.io.out.bits(), serialized_data) << "c.io.in.bits mismatch, i " << i;
        c.step();
    }

    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.in.isReady(), true);
    EXPECT_EQ_BOOL(c.io.out.valid(), false);
}
TEST(SidebandSerDesTests, SimpleDeserializerSanity) {
    FdiParams fdi_params{8, 8, 32};
    SidebandParams sb_params;

    bool io_in_valid = false;
    bool io_out_ready = false;
    BigUInt data = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    BigUInt io_in_bits;
    int msg_w = sb_params.sb_node_msg_width;  // 128
    int sb_w = fdi_params.sb_width;           // 32

    auto top = createTopModule<SidebandDeserializer>(sb_params, fdi_params);
    auto &c = *top;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_in_valid_func = [&io_in_valid] () -> Bool {
        return Bool(io_in_valid);
    };
    c.io.in.valid = io_in_valid_func;

    Wire<Bool>::TPFUNC io_out_ready_func = [&io_out_ready]() -> Bool {
        return Bool(io_out_ready);
    };
    c.io.out.assignReady(io_out_ready_func);

    Wire<UInt>::TPFUNC io_in_bits_func = [&sb_w, &io_in_bits]() -> UInt {
        return UInt(sb_w, io_in_bits);
    };
    c.io.in.bits = (io_in_bits_func);

    // ======================== run ========================
    // prepare random data generator
    std::cout << "Test started" << std::endl;

    // init
    io_in_valid = false;
    io_out_ready = false;
    c.step();

    //Send data to deserializer
    std::cout << "Send data" << std::endl;
    for (int i = 0; i < msg_w / sb_w; i++) {
        io_in_valid = true;
        io_in_bits = (data >> i * sb_w) & ((BigUInt(1) << sb_w) - 1);
        EXPECT_EQ_BOOL(c.io.out.isValid(), false);
        c.step();
    }

    //Check deserialized data and credit return
    io_in_valid = false;
    io_out_ready = true;
    EXPECT_EQ_BOOL(c.io.out.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.out.bits(), data);
    c.step();

    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.out.isValid(), false);
}
TEST(SidebandSerDesTests, StressSerializerSanity) {
    FdiParams fdi_params{8, 8, 32};
    SidebandParams sb_params;

    bool io_in_valid = false;
    bool io_out_credit = false;
    BigUInt data = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    BigUInt io_in_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    int msg_w = sb_params.sb_node_msg_width;    // 128
    int sb_w = fdi_params.sb_width;             // 32
    int cdt_max = sb_params.max_crd;            // 32

    auto top = createTopModule<SidebandSerializer>(sb_params, fdi_params);
    auto &c = *top;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_in_valid_func = [&io_in_valid] () -> Bool {
        return Bool(io_in_valid);
    };
    c.io.in.assignValid(io_in_valid_func);

    Wire<Bool>::TPFUNC io_out_credit_func = [&io_out_credit]() -> Bool {
        return Bool(io_out_credit);
    };
    c.io.out.credit = io_out_credit_func;

    Wire<UInt>::TPFUNC io_in_bits_func = [&msg_w, &io_in_bits]() -> UInt {
        return UInt(msg_w, io_in_bits);
    };
    c.io.in.assignBits(io_in_bits_func);

    // ======================== run ========================
    // prepare random data generator
    std::cout << ("Test started") << std::endl;

    //init
    io_in_valid = false;
    io_out_credit = false;
    c.step();

    //Transfer data 32 times until no credit left
    for (int i = 0; i < cdt_max; i++) {
        //Send data to serializer
        //std::cout << ("Send data") << std::endl;
        io_in_valid = true;
        io_in_bits = 1;
        EXPECT_EQ_BOOL(c.io.out.valid(), false);
        c.step();

        //Check serialized data
        io_in_valid = false;
        for (int j = 0; j < msg_w / sb_w; j++) {
            BigUInt serialized_data = (io_in_bits >> j * sb_w) & ((BigUInt(1) << sb_w) - 1);
            EXPECT_EQ_BOOL(c.io.in.isReady(), false) << "i " << i << " j " << j;
            EXPECT_EQ_BOOL(c.io.out.valid(), true) << "i " << i << " j " << j;
            EXPECT_EQ_BigUInt(c.io.out.bits(), serialized_data) << "i " << i << " j " << j;
            c.step();
        }
    }

    //Send data to serializer when no credit
    //std::cout << "Send data" << std::endl;
    io_in_valid = true;
    io_in_bits = 1;
    EXPECT_EQ_BOOL(c.io.out.valid(), false);
    c.step();

    //Check not send out msg when no credit left
    io_in_valid = false;
    EXPECT_EQ_BOOL(c.io.out.valid(), false);
    EXPECT_EQ_BOOL(c.io.in.isReady(), false);
    c.step();
}