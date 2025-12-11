#include <gtest/gtest.h>
#include "sideband/sideband_node.hpp"
#include "test_utils.hpp"

using namespace CCPS;

TEST(SidebandNodeTests, SimpleSidebandNodeTxSanity) {
    FdiParams fdi_params{8, 8, 32};
    SidebandParams sb_params;

    unsigned sb_node_msg_width = sb_params.sb_node_msg_width;
    unsigned sb_w = fdi_params.sb_width;

    auto top = createTopModule<SidebandNode>(sb_params, fdi_params);
    auto &c = *top;

    bool io_inner_layer_to_node_valid = false;
    BigUInt io_inner_layer_to_node_bits = 0;
    bool io_outer_tx_credit = false;
    bool io_inner_node_to_layer_ready = false;
    BigUInt io_outer_rx_bits = 0;
    bool io_outer_rx_valid = false;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_inner_layer_to_node_valid_func = [&io_inner_layer_to_node_valid] () -> Bool {
        return Bool(io_inner_layer_to_node_valid);
    };
    c.io.inner.layer_to_node.assignValid(io_inner_layer_to_node_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_bits_func = [&io_inner_layer_to_node_bits, &sb_node_msg_width] () -> UInt {
        return UInt(sb_node_msg_width, io_inner_layer_to_node_bits);
    };
    c.io.inner.layer_to_node.assignBits(io_inner_layer_to_node_bits_func);

    Wire<Bool>::TPFUNC io_outer_tx_credit_func = [&io_outer_tx_credit] () -> Bool {
        return Bool(io_outer_tx_credit);
    };
    c.io.outer.tx.credit = io_outer_tx_credit_func;

    Wire<Bool>::TPFUNC io_inner_node_to_layer_ready_func = [&io_inner_node_to_layer_ready] () -> Bool {
        return Bool(io_inner_node_to_layer_ready);
    };
    c.io.inner.node_to_layer.assignReady(io_inner_node_to_layer_ready_func);

    Wire<UInt>::TPFUNC io_outer_rx_bits_func = [&io_outer_rx_bits, &sb_w] () -> UInt {
        return UInt(sb_w, io_outer_rx_bits);
    };
    c.io.outer.rx.bits = (io_outer_rx_bits_func);

    Wire<Bool>::TPFUNC io_outer_rx_valid_func = [&io_outer_rx_valid] () -> Bool {
        return Bool(io_outer_rx_valid);
    };
    c.io.outer.rx.valid = (io_outer_rx_valid_func);


    // ======================== run ========================
    //init
    io_inner_layer_to_node_valid = false;
    io_outer_tx_credit = false;
    c.step();

    ////Layer send data to sidebandNode
    io_inner_layer_to_node_valid = true;
    io_inner_layer_to_node_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    EXPECT_EQ_BOOL(c.io.outer.tx.valid(), false);
    c.step();

    //Check tx data from sidebandNode
    for (int i = 0; i < sb_node_msg_width/sb_w; i++) {
        BigUInt serialized_data = (io_inner_layer_to_node_bits >> i * sb_w) & ((BigUInt(1) << sb_w) - 1);
        EXPECT_EQ_BOOL(c.io.inner.layer_to_node.isReady(), false) << "i " << i;
        EXPECT_EQ_BOOL(c.io.outer.tx.valid(), true) << "i " << i;
        EXPECT_EQ_BigUInt(c.io.outer.tx.bits(), serialized_data) << "i " << i;
        c.step();
    }

    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.inner.layer_to_node.isReady(), true);
    EXPECT_EQ_BOOL(c.io.outer.tx.valid(), false);
}

TEST(SidebandNodeTests, SimpleSidebandNodeRxSanity) {
    FdiParams fdi_params{8, 8, 32};
    SidebandParams sb_params;

    unsigned sb_node_msg_width = sb_params.sb_node_msg_width;
    unsigned sb_w = fdi_params.sb_width;

    auto top = createTopModule<SidebandNode>(sb_params, fdi_params);
    auto &c = *top;

    bool io_inner_layer_to_node_valid = false;
    BigUInt io_inner_layer_to_node_bits = 0;
    bool io_outer_tx_credit = false;
    bool io_inner_node_to_layer_ready = false;
    BigUInt io_outer_rx_bits = 0;
    bool io_outer_rx_valid = false;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_inner_layer_to_node_valid_func = [&io_inner_layer_to_node_valid] () -> Bool {
        return Bool(io_inner_layer_to_node_valid);
    };
    c.io.inner.layer_to_node.assignValid(io_inner_layer_to_node_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_bits_func = [&io_inner_layer_to_node_bits, &sb_node_msg_width] () -> UInt {
        return UInt(sb_node_msg_width, io_inner_layer_to_node_bits);
    };
    c.io.inner.layer_to_node.assignBits(io_inner_layer_to_node_bits_func);

    Wire<Bool>::TPFUNC io_outer_tx_credit_func = [&io_outer_tx_credit] () -> Bool {
        return Bool(io_outer_tx_credit);
    };
    c.io.outer.tx.credit = io_outer_tx_credit_func;

    Wire<Bool>::TPFUNC io_inner_node_to_layer_ready_func = [&io_inner_node_to_layer_ready] () -> Bool {
        return Bool(io_inner_node_to_layer_ready);
    };
    c.io.inner.node_to_layer.assignReady(io_inner_node_to_layer_ready_func);

    Wire<UInt>::TPFUNC io_outer_rx_bits_func = [&io_outer_rx_bits, &sb_w] () -> UInt {
        return UInt(sb_w, io_outer_rx_bits);
    };
    c.io.outer.rx.bits = (io_outer_rx_bits_func);

    Wire<Bool>::TPFUNC io_outer_rx_valid_func = [&io_outer_rx_valid] () -> Bool {
        return Bool(io_outer_rx_valid);
    };
    c.io.outer.rx.valid = (io_outer_rx_valid_func);


    // ======================== run ========================
    //init
    io_outer_rx_valid = false;
    io_inner_node_to_layer_ready = false;
    c.step();

    //Interface send data to sidebandNode
    // suzhifeng. Be careful not to generate invalid messages, because SidebandEnqArbiter will filter them.
    BigUInt data = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353525;
    for (int i = 0; i < sb_node_msg_width/sb_w; i++) {
        io_outer_rx_valid = true;
        BigUInt serialized_data = (data >> i * sb_w) & ((BigUInt(1) << sb_w) - 1);
        io_outer_rx_bits = serialized_data;
        EXPECT_EQ_BOOL(c.io.inner.node_to_layer.isValid(), false) << "i " << i;
        c.step();
    }

    //Check data from node to layer, and credit return
    io_outer_rx_valid = false;
    io_inner_node_to_layer_ready = true;
    c.step();
    EXPECT_EQ_BOOL(c.io.inner.node_to_layer.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.inner.node_to_layer.bits(), data);
    EXPECT_EQ_BOOL(c.io.outer.rx.credit(), true);
    c.step();

    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.inner.node_to_layer.isValid(), false);
}


TEST(SidebandNodeTests, StressSidebandNodeTxSanity) {
    FdiParams fdi_params{8, 8, 32};
    SidebandParams sb_params;

    unsigned sb_node_msg_width = sb_params.sb_node_msg_width;
    unsigned sb_w = fdi_params.sb_width;
    int max_crd = sb_params.max_crd;

    auto top = createTopModule<SidebandNode>(sb_params, fdi_params);
    auto &c = *top;

    bool io_inner_layer_to_node_valid = false;
    BigUInt io_inner_layer_to_node_bits = 0;
    bool io_outer_tx_credit = false;
    bool io_inner_node_to_layer_ready = false;
    BigUInt io_outer_rx_bits = 0;
    bool io_outer_rx_valid = false;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_inner_layer_to_node_valid_func = [&io_inner_layer_to_node_valid] () -> Bool {
        return Bool(io_inner_layer_to_node_valid);
    };
    c.io.inner.layer_to_node.assignValid(io_inner_layer_to_node_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_bits_func = [&io_inner_layer_to_node_bits, &sb_node_msg_width] () -> UInt {
        return UInt(sb_node_msg_width, io_inner_layer_to_node_bits);
    };
    c.io.inner.layer_to_node.assignBits(io_inner_layer_to_node_bits_func);

    Wire<Bool>::TPFUNC io_outer_tx_credit_func = [&io_outer_tx_credit] () -> Bool {
        return Bool(io_outer_tx_credit);
    };
    c.io.outer.tx.credit = io_outer_tx_credit_func;

    Wire<Bool>::TPFUNC io_inner_node_to_layer_ready_func = [&io_inner_node_to_layer_ready] () -> Bool {
        return Bool(io_inner_node_to_layer_ready);
    };
    c.io.inner.node_to_layer.assignReady(io_inner_node_to_layer_ready_func);

    Wire<UInt>::TPFUNC io_outer_rx_bits_func = [&io_outer_rx_bits, &sb_w] () -> UInt {
        return UInt(sb_w, io_outer_rx_bits);
    };
    c.io.outer.rx.bits = (io_outer_rx_bits_func);

    Wire<Bool>::TPFUNC io_outer_rx_valid_func = [&io_outer_rx_valid] () -> Bool {
        return Bool(io_outer_rx_valid);
    };
    c.io.outer.rx.valid = (io_outer_rx_valid_func);


    // ======================== run ========================
    //init
    io_inner_layer_to_node_valid = false;
    io_outer_tx_credit = false;
    c.step();

    //Transfer data 32 times until no credit left
    for (int i = 0; i < max_crd; i++) {
        //Send non-completion packet to serializer
        io_inner_layer_to_node_valid = true;
        io_inner_layer_to_node_bits = 1;
        const auto& data = io_inner_layer_to_node_bits;
        EXPECT_EQ_BOOL(c.io.outer.tx.valid(), false);
        c.step();

        //Check serialized data
        io_inner_layer_to_node_valid = false;
        for (int j = 0; j < (sb_node_msg_width / sb_w); j++) {
            BigUInt serialized_data = (data >> j * sb_w) & ((BigUInt(1) << sb_w) - 1);
            EXPECT_EQ_BOOL(c.io.inner.layer_to_node.isReady(), false) << "i " << i << " j " << j;
            EXPECT_EQ_BOOL(c.io.outer.tx.valid(), true) << "i " << i << " j " << j;
            EXPECT_EQ_BigUInt(c.io.outer.tx.bits(), serialized_data) << "i " << i << " j " << j;
            c.step();
        }
    }

    //Send non-completion packet to serializer when no credit
    io_inner_layer_to_node_valid = true;
    io_inner_layer_to_node_bits = 1;
    const auto& data = io_inner_layer_to_node_bits;
    EXPECT_EQ_BOOL(c.io.outer.tx.valid(), false);
    EXPECT_EQ_BOOL(c.io.inner.layer_to_node.isReady(), false);
    c.step();

    //Check not send out msg when no credit left
    io_inner_layer_to_node_valid = true;
    EXPECT_EQ_BOOL(c.io.outer.tx.valid(), false);
    EXPECT_EQ_BOOL(c.io.inner.layer_to_node.isReady(), false);
    c.step();

    //Send completion packet to serializer when no credit
    io_inner_layer_to_node_valid = true;
    io_inner_layer_to_node_bits = 16;
    EXPECT_EQ_BOOL(c.io.outer.tx.valid(), false);
    EXPECT_EQ_BOOL(c.io.inner.layer_to_node.isReady(), true);
    c.step();

    //Check still send out completion packet when no credit left
    io_inner_layer_to_node_valid = false;
    for (int j = 0; j < (sb_node_msg_width / sb_w); j++) {
        BigUInt serialized_data = (data >> j * sb_w) & ((BigUInt(1) << sb_w) - 1);
        EXPECT_EQ_BOOL(c.io.inner.layer_to_node.isReady(), false) << "j " << j;
        EXPECT_EQ_BOOL(c.io.outer.tx.valid(), true) << "j " << j;
        EXPECT_EQ_BigUInt(c.io.outer.tx.bits(), serialized_data) << "j " << j;
        c.step();
    }

    //Check sendout completion doesn't decrease credit,  ready = 0
    io_inner_layer_to_node_bits = 1;
    EXPECT_EQ_BOOL(c.io.inner.layer_to_node.isReady(), false);

    //Check credit increse if get credit return, ready = 1
    io_outer_tx_credit = true;
    c.step();
    io_outer_tx_credit = false;
    EXPECT_EQ_BOOL(c.io.inner.layer_to_node.isReady(), true);

    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.outer.tx.valid(), false);
}


TEST(SidebandNodeTests, StressSidebandNodeRxSanity) {
    FdiParams fdi_params{8, 8, 32};
    SidebandParams sb_params;

    unsigned sb_node_msg_width = sb_params.sb_node_msg_width;
    unsigned sb_w = fdi_params.sb_width;

    auto top = createTopModule<SidebandNode>(sb_params, fdi_params);
    auto &c = *top;

    bool io_inner_layer_to_node_valid = false;
    BigUInt io_inner_layer_to_node_bits = 0;
    bool io_outer_tx_credit = false;
    bool io_inner_node_to_layer_ready = false;
    BigUInt io_outer_rx_bits = 0;
    bool io_outer_rx_valid = false;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_inner_layer_to_node_valid_func = [&io_inner_layer_to_node_valid] () -> Bool {
        return Bool(io_inner_layer_to_node_valid);
    };
    c.io.inner.layer_to_node.assignValid(io_inner_layer_to_node_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_bits_func = [&io_inner_layer_to_node_bits, &sb_node_msg_width] () -> UInt {
        return UInt(sb_node_msg_width, io_inner_layer_to_node_bits);
    };
    c.io.inner.layer_to_node.assignBits(io_inner_layer_to_node_bits_func);

    Wire<Bool>::TPFUNC io_outer_tx_credit_func = [&io_outer_tx_credit] () -> Bool {
        return Bool(io_outer_tx_credit);
    };
    c.io.outer.tx.credit = io_outer_tx_credit_func;

    Wire<Bool>::TPFUNC io_inner_node_to_layer_ready_func = [&io_inner_node_to_layer_ready] () -> Bool {
        return Bool(io_inner_node_to_layer_ready);
    };
    c.io.inner.node_to_layer.assignReady(io_inner_node_to_layer_ready_func);

    Wire<UInt>::TPFUNC io_outer_rx_bits_func = [&io_outer_rx_bits, &sb_w] () -> UInt {
        return UInt(sb_w, io_outer_rx_bits);
    };
    c.io.outer.rx.bits = (io_outer_rx_bits_func);

    Wire<Bool>::TPFUNC io_outer_rx_valid_func = [&io_outer_rx_valid] () -> Bool {
        return Bool(io_outer_rx_valid);
    };
    c.io.outer.rx.valid = (io_outer_rx_valid_func);


    // ======================== run ========================
    //init
    io_outer_rx_valid = false;
    io_inner_node_to_layer_ready = false;
    c.step();

    //Interface send data to sidebandNode
    const BigUInt enq_data[] = {0, 16, 18, 17};
    const BigUInt deq_golden_data[] = {16, 17, 18, 0};
    for (int i = 0; i < 4; i++) {
        const auto& data = enq_data[i];
        for (int j = 0; j < (sb_node_msg_width / sb_w); j++) {
            io_outer_rx_valid = true;
            io_outer_rx_bits = (data >> j * sb_w) & ((BigUInt(1) << sb_w) - 1);
            if (i == 0) EXPECT_EQ_BOOL(c.io.inner.node_to_layer.isValid(), false) << "i " << i << " j " << j;
            else if (i > 1) EXPECT_EQ_BOOL(c.io.inner.node_to_layer.isValid(), true) << "i " << i << " j " << j;
            c.step();
        }
    }

    //Check data from node to layer should be in correct order, and credit return
    io_outer_rx_valid = false;
    for (int i = 0; i < 4; i++) {
        io_inner_node_to_layer_ready = true;
        EXPECT_EQ_BOOL(c.io.inner.node_to_layer.isValid(), true) << "i " << i;
        EXPECT_EQ_BigUInt(c.io.inner.node_to_layer.bits(), deq_golden_data[i]) << "i " << i;
        // The first two deque packet is completion packet
        if (i==0 || i==1) EXPECT_EQ_BOOL(c.io.outer.rx.credit(), false);
        else EXPECT_EQ_BOOL(c.io.outer.rx.credit(), true);
        c.step();
    }

    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.inner.node_to_layer.isValid(), false);
}