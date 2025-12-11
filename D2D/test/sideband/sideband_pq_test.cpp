#include <gtest/gtest.h>
#include "sideband/sideband_node.hpp"
#include "test_utils.hpp"

using namespace CCPS;

TEST(SidebandPriorityQueueTests, SimpleEnqDeqSanity) {
    SidebandParams sb_params;
    int sb_node_msg_width = sb_params.sb_node_msg_width;

    auto top = createTopModule<SidebandPriorityQueue>(sb_params);
    auto &c = *top;

    bool io_enq_valid = false;
    BigUInt io_enq_bits = 0;
    bool io_deq_ready = false;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_enq_valid_func = [&io_enq_valid] () -> Bool {
        return Bool(io_enq_valid);
    };
    c.io.enq.assignValid(io_enq_valid_func);

    Wire<UInt>::TPFUNC io_enq_bits_func = [sb_node_msg_width, &io_enq_bits] () -> UInt {
        return UInt(sb_node_msg_width, io_enq_bits);
    };
    c.io.enq.assignBits(io_enq_bits_func);

    Wire<Bool>::TPFUNC io_deq_ready_func = [&io_deq_ready] () -> Bool {
        return Bool(io_deq_ready);
    };
    c.io.deq.assignReady(io_deq_ready_func);

    // ======================== run ========================
    //init
    io_enq_valid = false;
    io_deq_ready = false;
    c.step();

    //first enqueue
    io_enq_valid = true;
    io_enq_bits = 0;
    EXPECT_EQ_BOOL(c.io.deq.isValid(), false);
    c.step();

    //first dequeue
    io_enq_valid = false;
    io_deq_ready = true;
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.deq.bits(), io_enq_bits);
    c.step();

    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.deq.isValid(), false);
}

TEST(SidebandPriorityQueueTests, SimplePrioritySanity) {
    SidebandParams sb_params;
    int sb_node_msg_width = sb_params.sb_node_msg_width;

    auto top = createTopModule<SidebandPriorityQueue>(sb_params);
    auto &c = *top;

    bool io_enq_valid = false;
    BigUInt io_enq_bits = 0;
    bool io_deq_ready = false;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_enq_valid_func = [&io_enq_valid] () -> Bool {
        return Bool(io_enq_valid);
    };
    c.io.enq.assignValid(io_enq_valid_func);

    Wire<UInt>::TPFUNC io_enq_bits_func = [sb_node_msg_width, &io_enq_bits] () -> UInt {
        return UInt(sb_node_msg_width, io_enq_bits);
    };
    c.io.enq.assignBits(io_enq_bits_func);

    Wire<Bool>::TPFUNC io_deq_ready_func = [&io_deq_ready] () -> Bool {
        return Bool(io_deq_ready);
    };
    c.io.deq.assignReady(io_deq_ready_func);

    // ======================== run ========================
    //init
    io_enq_valid = false;
    io_deq_ready = false;
    c.step();

    //first enqueue
    io_enq_valid = true;
    io_enq_bits = 0; //priority 2
    EXPECT_EQ_BOOL(c.io.deq.isValid(), false);
    c.step();
    //second enqueue
    io_enq_valid = true;
    io_enq_bits = 16; //priority 0
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    c.step();
    //first dequeue
    io_enq_valid = false;
    io_deq_ready = true;
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.deq.bits(), 16);
    c.step();
    // second dequeue
    io_deq_ready = true;
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.deq.bits(), 0);
    c.step();
    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.deq.isValid(), false);
}

TEST(SidebandPriorityQueueTests, StressPrioritySanity) {
    SidebandParams sb_params;
    int sb_node_msg_width = sb_params.sb_node_msg_width;

    auto top = createTopModule<SidebandPriorityQueue>(sb_params);
    auto &c = *top;

    bool io_enq_valid = false;
    BigUInt io_enq_bits = 0;
    bool io_deq_ready = false;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_enq_valid_func = [&io_enq_valid] () -> Bool {
        return Bool(io_enq_valid);
    };
    c.io.enq.assignValid(io_enq_valid_func);

    Wire<UInt>::TPFUNC io_enq_bits_func = [sb_node_msg_width, &io_enq_bits] () -> UInt {
        return UInt(sb_node_msg_width, io_enq_bits);
    };
    c.io.enq.assignBits(io_enq_bits_func);

    Wire<Bool>::TPFUNC io_deq_ready_func = [&io_deq_ready] () -> Bool {
        return Bool(io_deq_ready);
    };
    c.io.deq.assignReady(io_deq_ready_func);

    // ======================== run ========================
    //init
    io_enq_valid = false;
    io_deq_ready = false;
    c.step();

    //first enqueue
    io_enq_valid = true;
    io_enq_bits = 0; //priority 2
    EXPECT_EQ_BOOL(c.io.deq.isValid(), false);
    c.step();
    //second enqueue
    io_enq_valid = true;
    io_enq_bits = 16; //priority 0
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    c.step();
    //thrid enqueue
    io_enq_valid = true;
    io_enq_bits = 18; //priority 1
    c.step();
    //fourth enqueue
    io_enq_valid = true;
    io_enq_bits = 17; //priority 0
    c.step();

    //first dequeue
    io_enq_valid = false;
    io_deq_ready = true;
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.deq.bits(), 16);
    c.step();
    // second dequeue
    io_deq_ready = true;
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.deq.bits(), 17);
    c.step();
    // third dequeue
    io_deq_ready = true;
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.deq.bits(), 18);
    c.step();
    // fourth dequeue
    io_deq_ready = true;
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.deq.bits(), 0);
    c.step();
    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.deq.isValid(), false);
}