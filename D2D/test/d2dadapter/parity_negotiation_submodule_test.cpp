#include "d2dadapter/d2d_adapter_constants.hpp"
#include "d2dadapter/parity_negotiation_submodule.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/time_slice.hpp"
#include "utils/common.hpp"
#include "interfaces/fdi.hpp"
#include "interfaces/rdi.hpp"
#include "sideband/sideband_io.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

TEST (ParityNegotiationSubmoduleTest, ComplteNegotiationBothFalseAndThenTxOpen) {
    auto top = createTopModule<ParityNegotiationSubmodule>();
    auto &c = *top;

    // ======================== signals ========================
    bool io_start_negotiation = false;

    BigUInt io_parity_sb_rcv = 0;
    bool io_parity_sb_rdy = false;

    bool io_parity_tx_sw_en = false;
    bool io_parity_rx_sw_en = false;

    BigUInt io_cycles_1us = 0;

    // ======================== connect ========================
    c.io.start_negotiation.capture(io_start_negotiation);
    c.io.parity_sb_rcv.capture(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, io_parity_sb_rcv);
    c.io.parity_sb_rdy.capture(io_parity_sb_rdy);
    c.io.parity_tx_sw_en.capture(io_parity_tx_sw_en);
    c.io.parity_rx_sw_en.capture(io_parity_rx_sw_en);
    c.io.cycles_1us.capture(32, io_cycles_1us);

    // ======================== run ========================
    // init
    io_start_negotiation = false;
    io_cycles_1us = 100;
    io_parity_tx_sw_en = false;
    io_parity_rx_sw_en = false;
    io_parity_sb_rcv = SideBandMessage::NOP;
    io_parity_sb_rdy = false;

    c.step();
    // start both are false
    io_start_negotiation = true;
    c.step();
    //should timeout and complete
    while(!c.io.negotiation_complete()) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), false);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), false);
    c.step(20);
    io_start_negotiation = false;
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), false);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), false);
    c.step(20);
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), false);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), false);
    // tx ok
    io_start_negotiation = true;
    io_parity_tx_sw_en = true;
    io_parity_rx_sw_en = false;
    c.step();
    while(c.io.parity_sb_snd().toBigUInt() != SideBandMessage::PARITY_FEATURE_REQ) {
        std::cout << "waiting for PARITY_FEATURE_REQ(" << SideBandMessage::PARITY_FEATURE_REQ
                  << "), got " << c.io.parity_sb_snd().toBigUInt() << std::endl;
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    // send req
    io_parity_sb_rdy = true;
    c.step();
    io_parity_sb_rdy = false;
    c.step();
    // give req
    io_parity_sb_rcv = SideBandMessage::PARITY_FEATURE_REQ;
    c.step();
    io_parity_sb_rcv = SideBandMessage::NOP;
    c.step();
    while(c.io.parity_sb_snd().toBigUInt() != SideBandMessage::PARITY_FEATURE_NAK) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    // send nack
    io_parity_sb_rdy = true;
    c.step();
    io_parity_sb_rdy = false;
    c.step();
    // give ack
    io_parity_sb_rcv = SideBandMessage::PARITY_FEATURE_ACK;
    c.step();
    io_parity_sb_rcv = SideBandMessage::NOP;
    c.step();

    while(!c.io.negotiation_complete()) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), false);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), true);
    c.step(20);
    io_start_negotiation = false;
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), false);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), true);
    c.step(20);
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), false);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), true);
}


TEST (ParityNegotiationSubmoduleTest, RequestTxButGetNack) {
    auto top = createTopModule<ParityNegotiationSubmodule>();
    auto &c = *top;

    // ======================== signals ========================
    bool io_start_negotiation = false;

    BigUInt io_parity_sb_rcv = 0;
    bool io_parity_sb_rdy = false;

    bool io_parity_tx_sw_en = false;
    bool io_parity_rx_sw_en = false;

    BigUInt io_cycles_1us = 0;

    // ======================== connect ========================
    c.io.start_negotiation.capture(io_start_negotiation);
    c.io.parity_sb_rcv.capture(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, io_parity_sb_rcv);
    c.io.parity_sb_rdy.capture(io_parity_sb_rdy);
    c.io.parity_tx_sw_en.capture(io_parity_tx_sw_en);
    c.io.parity_rx_sw_en.capture(io_parity_rx_sw_en);
    c.io.cycles_1us.capture(32, io_cycles_1us);

    // ======================== run ========================
    // init
    io_start_negotiation = false;
    io_cycles_1us = 100;
    io_parity_tx_sw_en = false;
    io_parity_rx_sw_en = false;
    io_parity_sb_rcv = SideBandMessage::NOP;
    io_parity_sb_rdy = false;

    c.step();
    // tx
    io_start_negotiation = true;
    io_parity_tx_sw_en = true;
    io_parity_rx_sw_en = false;
    c.step();
    while(c.io.parity_sb_snd().toBigUInt() != SideBandMessage::PARITY_FEATURE_REQ) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    // send req
    io_parity_sb_rdy = true;
    c.step();
    io_parity_sb_rdy = false;
    c.step();
    // give req
    io_parity_sb_rcv = SideBandMessage::PARITY_FEATURE_REQ;
    c.step();
    io_parity_sb_rcv = SideBandMessage::NOP;
    c.step();
    while(c.io.parity_sb_snd().toBigUInt() != SideBandMessage::PARITY_FEATURE_NAK) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    // send nack
    io_parity_sb_rdy = true;
    c.step();
    io_parity_sb_rdy = false;
    c.step();
    // give nack
    io_parity_sb_rcv = SideBandMessage::PARITY_FEATURE_NAK;
    c.step();
    io_parity_sb_rcv = SideBandMessage::NOP;
    c.step();

    while(!c.io.negotiation_complete()) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), false);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), false);
    c.step(20);
    io_start_negotiation = false;
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), false);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), false);
    c.step(20);
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), false);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), false);
}

TEST (ParityNegotiationSubmoduleTest, OpenRx) {
    auto top = createTopModule<ParityNegotiationSubmodule>();
    auto &c = *top;

    // ======================== signals ========================
    bool io_start_negotiation = false;

    BigUInt io_parity_sb_rcv = 0;
    bool io_parity_sb_rdy = false;

    bool io_parity_tx_sw_en = false;
    bool io_parity_rx_sw_en = false;

    BigUInt io_cycles_1us = 0;

    // ======================== connect ========================
    c.io.start_negotiation.capture(io_start_negotiation);
    c.io.parity_sb_rcv.capture(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, io_parity_sb_rcv);
    c.io.parity_sb_rdy.capture(io_parity_sb_rdy);
    c.io.parity_tx_sw_en.capture(io_parity_tx_sw_en);
    c.io.parity_rx_sw_en.capture(io_parity_rx_sw_en);
    c.io.cycles_1us.capture(32, io_cycles_1us);

    // ======================== run ========================
    // init
    io_start_negotiation = false;
    io_cycles_1us = 100;
    io_parity_tx_sw_en = false;
    io_parity_rx_sw_en = false;
    io_parity_sb_rcv = SideBandMessage::NOP;
    io_parity_sb_rdy = false;

    c.step();
    // tx
    io_start_negotiation = true;
    io_parity_tx_sw_en = false;
    io_parity_rx_sw_en = true;
    c.step();
    // give req
    io_parity_sb_rcv = SideBandMessage::PARITY_FEATURE_REQ;
    c.step();
    io_parity_sb_rcv = SideBandMessage::NOP;
    c.step();
    while(c.io.parity_sb_snd().toBigUInt() != SideBandMessage::PARITY_FEATURE_ACK) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    // send ack
    io_parity_sb_rdy = true;
    c.step();
    io_parity_sb_rdy = false;
    c.step();

    while(!c.io.negotiation_complete()) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), true);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), false);
    c.step(20);
    io_start_negotiation = false;
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), true);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), false);
    c.step(20);
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), true);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), false);
}


TEST (ParityNegotiationSubmoduleTest, OpenBothTxAndRx) {
    auto top = createTopModule<ParityNegotiationSubmodule>();
    auto &c = *top;

    // ======================== signals ========================
    bool io_start_negotiation = false;

    BigUInt io_parity_sb_rcv = 0;
    bool io_parity_sb_rdy = false;

    bool io_parity_tx_sw_en = false;
    bool io_parity_rx_sw_en = false;

    BigUInt io_cycles_1us = 0;

    // ======================== connect ========================
    c.io.start_negotiation.capture(io_start_negotiation);
    c.io.parity_sb_rcv.capture(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, io_parity_sb_rcv);
    c.io.parity_sb_rdy.capture(io_parity_sb_rdy);
    c.io.parity_tx_sw_en.capture(io_parity_tx_sw_en);
    c.io.parity_rx_sw_en.capture(io_parity_rx_sw_en);
    c.io.cycles_1us.capture(32, io_cycles_1us);

    // ======================== run ========================
    // init
    io_start_negotiation = false;
    io_cycles_1us = 100;
    io_parity_tx_sw_en = false;
    io_parity_rx_sw_en = false;
    io_parity_sb_rcv = SideBandMessage::NOP;
    io_parity_sb_rdy = false;

    c.step();
    // tx
    io_start_negotiation = true;
    io_parity_tx_sw_en = true;
    io_parity_rx_sw_en = true;
    c.step();
    while(c.io.parity_sb_snd().toBigUInt() != SideBandMessage::PARITY_FEATURE_REQ) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    // send req
    io_parity_sb_rdy = true;
    c.step();
    io_parity_sb_rdy = false;
    c.step();
    // give req
    io_parity_sb_rcv = SideBandMessage::PARITY_FEATURE_REQ;
    c.step();
    io_parity_sb_rcv = SideBandMessage::NOP;
    c.step();
    while(c.io.parity_sb_snd().toBigUInt() != SideBandMessage::PARITY_FEATURE_ACK) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    // send ack
    io_parity_sb_rdy = true;
    c.step();
    io_parity_sb_rdy = false;
    c.step();
    // give ack
    io_parity_sb_rcv = SideBandMessage::PARITY_FEATURE_ACK;
    c.step();
    io_parity_sb_rcv = SideBandMessage::NOP;
    c.step();

    while(!c.io.negotiation_complete()) {
        EXPECT_EQ_BigUInt(c.io.parity_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), true);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), true);
    c.step(20);
    io_start_negotiation = false;
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), true);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), true);
    c.step(20);
    EXPECT_EQ_BOOL(c.io.parity_rx_enable(), true);
    EXPECT_EQ_BOOL(c.io.parity_tx_enable(), true);
}