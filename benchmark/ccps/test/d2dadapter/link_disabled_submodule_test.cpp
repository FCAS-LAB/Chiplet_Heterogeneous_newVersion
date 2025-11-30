#include "d2dadapter/d2d_adapter_constants.hpp"
#include "d2dadapter/link_disabled_submodule.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/time_slice.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

TEST(LinkDisabledSubmoduleTest, CompleteDisableFlowAsDPAdapter) {
    auto top = createTopModule<LinkDisabledSubmodule>();
    auto &c = *top;

    BigUInt io_fdi_lp_state_req = PhyStateReq::nop;
    BigUInt io_fdi_lp_state_req_prev = PhyStateReq::nop;
    BigUInt io_link_state = PhyStateReq::nop;
    BigUInt io_disabled_sb_rcv = SideBandMessage::NOP;
    bool io_disabled_sb_rdy = false;

    const int sw = D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH;

    // ======================== connect ========================
    c.io.fdi_lp_state_req.capture(4, io_fdi_lp_state_req);
    c.io.fdi_lp_state_req_prev.capture(4, io_fdi_lp_state_req_prev);
    c.io.link_state.capture(4, io_link_state);
    c.io.disabled_sb_rcv.capture(sw, io_disabled_sb_rcv);
    c.io.disabled_sb_rdy.capture(io_disabled_sb_rdy);

    // ======================== run ========================
    // init
    io_fdi_lp_state_req = PhyStateReq::active;
    io_link_state = PhyState::active;
    io_disabled_sb_rcv = SideBandMessage::NOP;
    io_disabled_sb_rdy = false;

    c.step();

    for (int i = 0; i < 5; i++) {
        // fdi request disabled
        io_fdi_lp_state_req = PhyStateReq::disabled;
        c.step();
        while (c.io.disabled_sb_snd().toBigUInt() != SideBandMessage::REQ_DISABLED) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }

        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::REQ_DISABLED);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }

        // send req
        io_disabled_sb_rdy = true;
        c.step();
        io_disabled_sb_rdy = false;
        c.step();

        //
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }

        // get rsp disabled
        io_disabled_sb_rcv = SideBandMessage::RSP_DISABLED;
        c.step();
        io_disabled_sb_rcv = SideBandMessage::NOP;
        c.step();

        while(static_cast<bool>(c.io.disabled_entry()) == false) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }

        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), true);
            c.step();
        }

        // transition to disabled
        io_link_state = PhyState::disabled;
        c.step();

        // should request for rdi to disabled
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }

        // transition to reset go again
        io_fdi_lp_state_req = PhyStateReq::active;
        c.step(100);
        io_link_state = PhyState::reset;
        c.step(100);
        io_fdi_lp_state_req = PhyStateReq::nop;
        EXPECT_EQ_BOOL(c.io.disabled_entry(), false);

        c.step();
        io_fdi_lp_state_req = PhyStateReq::active;
        EXPECT_EQ_BOOL(c.io.disabled_entry(), false);

        c.step(100);
        io_link_state = PhyState::active;
        EXPECT_EQ_BOOL(c.io.disabled_entry(), false);

        c.step(100);
        EXPECT_EQ_BOOL(c.io.disabled_entry(), false);

        c.step(100);
    }
}

TEST(LinkDisabledSubmoduleTest, CompleteDisableFlowAsUPAdapter) {
    auto top = createTopModule<LinkDisabledSubmodule>();
    auto &c = *top;

    BigUInt io_fdi_lp_state_req = PhyStateReq::nop;
    BigUInt io_fdi_lp_state_req_prev = PhyStateReq::nop;
    BigUInt io_link_state = PhyStateReq::nop;
    BigUInt io_disabled_sb_rcv = SideBandMessage::NOP;
    bool io_disabled_sb_rdy = false;

    const int sw = D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH;

    // ======================== connect ========================
    c.io.fdi_lp_state_req.capture(4, io_fdi_lp_state_req);
    c.io.fdi_lp_state_req_prev.capture(4, io_fdi_lp_state_req_prev);
    c.io.link_state.capture(4, io_link_state);
    c.io.disabled_sb_rcv.capture(sw, io_disabled_sb_rcv);
    c.io.disabled_sb_rdy.capture(io_disabled_sb_rdy);

    // ======================== run ========================
    // init
    io_fdi_lp_state_req = PhyStateReq::active;
    io_link_state = PhyState::active;
    io_disabled_sb_rcv = SideBandMessage::NOP;
    io_disabled_sb_rdy = false;
    c.step();

    for (int i = 0; i < 5; i++) {
        // fdi request disabled
        io_disabled_sb_rcv = SideBandMessage::REQ_DISABLED;
        c.step();
        io_disabled_sb_rcv = SideBandMessage::NOP;
        c.step();

        while (c.io.disabled_sb_snd().toBigUInt() != SideBandMessage::RSP_DISABLED) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::RSP_DISABLED);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }
        // send rsp
        io_disabled_sb_rdy = true;
        c.step();
        io_disabled_sb_rdy = false;
        c.step();

        while(static_cast<bool>(c.io.disabled_entry()) == false) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }
        // complete disabled
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), true);
            c.step();
        }
        // transition to disabled
        io_link_state = PhyState::disabled;
        c.step();
        // no need to request for rdi
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }
        // transition to reset go again
        io_fdi_lp_state_req = PhyStateReq::active;
        c.step(100);
        io_link_state = PhyState::reset;
        c.step(100);
        io_fdi_lp_state_req = PhyStateReq::nop;
        EXPECT_EQ_BOOL(c.io.disabled_entry(), false);

        c.step();
        io_fdi_lp_state_req = PhyStateReq::active;
        EXPECT_EQ_BOOL(c.io.disabled_entry(), false);

        c.step(100);
        io_link_state = PhyState::active;
        EXPECT_EQ_BOOL(c.io.disabled_entry(), false);

        c.step(100);
        EXPECT_EQ_BOOL(c.io.disabled_entry(), false);

        c.step(100);
    }
}

TEST(LinkDisabledSubmoduleTest, CompleteDisableFlowAsDPAdapterFromReset) {
    auto top = createTopModule<LinkDisabledSubmodule>();
    auto &c = *top;

    BigUInt io_fdi_lp_state_req = PhyStateReq::nop;
    BigUInt io_fdi_lp_state_req_prev = PhyStateReq::nop;
    BigUInt io_link_state = PhyStateReq::nop;
    BigUInt io_disabled_sb_rcv = SideBandMessage::NOP;
    bool io_disabled_sb_rdy = false;

    const int sw = D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH;

    // ======================== connect ========================
    c.io.fdi_lp_state_req.capture(4, io_fdi_lp_state_req);
    c.io.fdi_lp_state_req_prev.capture(4, io_fdi_lp_state_req_prev);
    c.io.link_state.capture(4, io_link_state);
    c.io.disabled_sb_rcv.capture(sw, io_disabled_sb_rcv);
    c.io.disabled_sb_rdy.capture(io_disabled_sb_rdy);

    // ======================== run ========================
    // init
    io_fdi_lp_state_req = PhyStateReq::active;
    io_fdi_lp_state_req_prev = PhyStateReq::active;
    io_link_state = PhyState::reset;
    io_disabled_sb_rcv = SideBandMessage::NOP;
    io_disabled_sb_rdy = false;
    c.step();

    for (int i = 0; i < 5; i++) {
        // fdi request disabled but not from NOP
        io_fdi_lp_state_req = PhyStateReq::disabled;
        io_fdi_lp_state_req_prev = PhyStateReq::active;
        c.step();
        io_fdi_lp_state_req = PhyStateReq::disabled;
        io_fdi_lp_state_req_prev = PhyStateReq::disabled;
        // should do nothing
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }
        // give the correct transition
        io_fdi_lp_state_req = PhyStateReq::nop;
        io_fdi_lp_state_req_prev = PhyStateReq::disabled;
        c.step();
        io_fdi_lp_state_req = PhyStateReq::disabled;
        io_fdi_lp_state_req_prev = PhyStateReq::nop;
        c.step();
        io_fdi_lp_state_req = PhyStateReq::disabled;
        io_fdi_lp_state_req_prev = PhyStateReq::disabled;
        c.step();
        //
        while (c.io.disabled_sb_snd().toBigUInt() != SideBandMessage::REQ_DISABLED) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::REQ_DISABLED);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }
        // send req
        io_disabled_sb_rdy = true;
        c.step();
        io_disabled_sb_rdy = false;
        c.step();
        //
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }
        // get rsp disabled
        io_disabled_sb_rcv = SideBandMessage::RSP_DISABLED;
        c.step();
        io_disabled_sb_rcv = SideBandMessage::NOP;
        c.step();

        while (static_cast<bool>(c.io.disabled_entry()) == false) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }

        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), true);
            c.step();
        }
        // transition to disabled
        io_link_state = PhyState::disabled;
        c.step();
        // should request for rdi to disabled
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.disabled_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.disabled_entry(), false);
            c.step();
        }
        // transition to reset go again
        io_fdi_lp_state_req = PhyStateReq::active;
        c.step(100);
        io_link_state = PhyState::reset;
        c.step(100);
        io_fdi_lp_state_req = PhyStateReq::nop;

        c.step();
        io_fdi_lp_state_req = PhyStateReq::active;
        io_fdi_lp_state_req_prev = PhyStateReq::active;

        c.step(100);
    }
}