#include "d2dadapter/d2d_adapter_constants.hpp"
#include "d2dadapter/link_reset_submodule.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/time_slice.hpp"
#include "utils/common.hpp"
#include "interfaces/fdi.hpp"
#include "interfaces/rdi.hpp"
#include "sideband/sideband_io.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

TEST (LinkResetSubmoduleTest, CompleteLinkresetFlowAsDPAdapter) {
    auto top = createTopModule<LinkResetSubmodule>();
    auto &c = *top;

    // ======================== signals ========================
    BigUInt io_fdi_lp_state_req = 0;
    BigUInt io_fdi_lp_state_req_prev = 0;
    BigUInt io_link_state = 0;
    BigUInt io_linkreset_sb_rcv = 0;
    bool io_linkreset_sb_rdy = 0;

    // ======================== connect ========================
    c.io.fdi_lp_state_req.capture(4, io_fdi_lp_state_req);
    c.io.fdi_lp_state_req_prev.capture(4, io_fdi_lp_state_req_prev);
    c.io.link_state.capture(4, io_link_state);
    c.io.linkreset_sb_rcv.capture(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, io_linkreset_sb_rcv);
    c.io.linkreset_sb_rdy.capture(io_linkreset_sb_rdy);

    // ======================== run ========================
    // init
    io_fdi_lp_state_req = PhyStateReq::active;
    io_link_state = PhyState::active;
    io_linkreset_sb_rcv = SideBandMessage::NOP;
    io_linkreset_sb_rdy = false;
    c.step();

    for (int i = 0; i < 5; i++) {
        // fdi request linkreset
        io_fdi_lp_state_req = PhyStateReq::linkReset;       // 1
        c.step();
        while(c.io.linkreset_sb_snd().toBigUInt() != SideBandMessage::REQ_LINKRESET) {  // 2
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::REQ_LINKRESET);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        // send req
        io_linkreset_sb_rdy = true; // 3
        c.step();
        io_linkreset_sb_rdy = false;
        c.step();
        //
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        // get rsp linkreset
        io_linkreset_sb_rcv = SideBandMessage::RSP_LINKRESET;   // 4
        c.step();
        io_linkreset_sb_rcv = SideBandMessage::NOP;
        c.step();

        while(!c.io.linkreset_entry()) {        // 5
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }

        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), true);
            c.step();
        }
        // transition to linkreset
        io_link_state = PhyState::linkReset;
        c.step();
        // should request for rdi to linkreset
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        // transition to reset go again
        io_fdi_lp_state_req = PhyStateReq::active;
        c.step(100);
        io_link_state = PhyState::reset;
        c.step(100);
        io_fdi_lp_state_req = PhyStateReq::nop;
        EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
        c.step(1);

        io_fdi_lp_state_req = PhyStateReq::active;
        EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);

        c.step(100);
        io_link_state = PhyState::active;
        EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);

        c.step(100);
        EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);

        c.step(100);
    }
}


TEST (LinkResetSubmoduleTest, CompleteLinkresetFlowAsUPAdapter) {
    auto top = createTopModule<LinkResetSubmodule>();
    auto &c = *top;

    // ======================== signals ========================
    BigUInt io_fdi_lp_state_req = 0;
    BigUInt io_fdi_lp_state_req_prev = 0;
    BigUInt io_link_state = 0;
    BigUInt io_linkreset_sb_rcv = 0;
    bool io_linkreset_sb_rdy = 0;

    // ======================== connect ========================
    c.io.fdi_lp_state_req.capture(4, io_fdi_lp_state_req);
    c.io.fdi_lp_state_req_prev.capture(4, io_fdi_lp_state_req_prev);
    c.io.link_state.capture(4, io_link_state);
    c.io.linkreset_sb_rcv.capture(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, io_linkreset_sb_rcv);
    c.io.linkreset_sb_rdy.capture(io_linkreset_sb_rdy);

    // ======================== run ========================
    // init
    io_fdi_lp_state_req = PhyStateReq::active;
    io_link_state = PhyState::active;
    io_linkreset_sb_rcv = SideBandMessage::NOP;
    io_linkreset_sb_rdy = false;
    c.step();
    for (int i = 0; i < 5; i++) {
        // fdi request linkreset
        io_linkreset_sb_rcv = SideBandMessage::REQ_LINKRESET;
        c.step();
        io_linkreset_sb_rcv = SideBandMessage::NOP;
        c.step();

        while(c.io.linkreset_sb_snd().toBigUInt() != SideBandMessage::RSP_LINKRESET){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::RSP_LINKRESET);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        // send rsp
        io_linkreset_sb_rdy = true;
        c.step();
        io_linkreset_sb_rdy = false;
        c.step();

        while(c.io.linkreset_entry().toBigUInt() == false){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        // complete linkreset
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), true);
            c.step();
        }
        // transition to linkreset
        io_link_state = PhyState::linkReset;
        c.step();
        // no need to request for rdi
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        // transition to reset go again
        io_fdi_lp_state_req = PhyStateReq::active;
        c.step(100);
        io_link_state = PhyState::reset;
        c.step(100);
        io_fdi_lp_state_req = PhyStateReq::nop;
        EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);

        c.step(1);
        io_fdi_lp_state_req = PhyStateReq::active;
        EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);

        c.step(100);
        io_link_state = PhyState::active;
        EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);

        c.step(100);
        EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);

        c.step(100);
    }
}


TEST (LinkResetSubmoduleTest, CompleteLinkresetFlowAsDPAdapterFromReset) {
    auto top = createTopModule<LinkResetSubmodule>();
    auto &c = *top;

    // ======================== signals ========================
    BigUInt io_fdi_lp_state_req = 0;
    BigUInt io_fdi_lp_state_req_prev = 0;
    BigUInt io_link_state = 0;
    BigUInt io_linkreset_sb_rcv = 0;
    bool io_linkreset_sb_rdy = 0;

    // ======================== connect ========================
    c.io.fdi_lp_state_req.capture(4, io_fdi_lp_state_req);
    c.io.fdi_lp_state_req_prev.capture(4, io_fdi_lp_state_req_prev);
    c.io.link_state.capture(4, io_link_state);
    c.io.linkreset_sb_rcv.capture(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, io_linkreset_sb_rcv);
    c.io.linkreset_sb_rdy.capture(io_linkreset_sb_rdy);

    // ======================== run ========================
    // init
    io_fdi_lp_state_req = PhyStateReq::active;
    io_fdi_lp_state_req_prev = PhyStateReq::active;
    io_link_state = PhyState::reset;
    io_linkreset_sb_rcv = SideBandMessage::NOP;
    io_linkreset_sb_rdy = false;

    c.step();
    for (int i = 0; i < 5; i++) {
        // fdi request linkreset but not from NOP
        io_fdi_lp_state_req = PhyStateReq::linkReset;
        io_fdi_lp_state_req_prev = PhyStateReq::active;
        c.step();
        io_fdi_lp_state_req = PhyStateReq::linkReset;
        io_fdi_lp_state_req_prev = PhyStateReq::linkReset;
        // should do nothing
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        // give the correct transition
        io_fdi_lp_state_req = PhyStateReq::nop;
        io_fdi_lp_state_req_prev = PhyStateReq::linkReset;
        c.step();
        io_fdi_lp_state_req = PhyStateReq::linkReset;
        io_fdi_lp_state_req_prev = PhyStateReq::nop;
        c.step();
        io_fdi_lp_state_req = PhyStateReq::linkReset;
        io_fdi_lp_state_req_prev = PhyStateReq::linkReset;
        c.step();
        //
        while(c.io.linkreset_sb_snd().toBigUInt() != SideBandMessage::REQ_LINKRESET){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::REQ_LINKRESET);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        // send req
        io_linkreset_sb_rdy = true;
        c.step();
        io_linkreset_sb_rdy = false;
        c.step();
        //
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }
        // get rsp linkreset
        io_linkreset_sb_rcv = SideBandMessage::RSP_LINKRESET;
        c.step();
        io_linkreset_sb_rcv = SideBandMessage::NOP;
        c.step();
        while(static_cast<bool>(c.io.linkreset_entry()) == false){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
            c.step();
        }

        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), true);
            c.step();
        }
        // transition to linkreset
        io_link_state = PhyState::linkReset;
        c.step();
        // should request for rdi to linkreset
        for(int i = 0; i < 10; i++){
            EXPECT_EQ_BigUInt(c.io.linkreset_sb_snd(), SideBandMessage::NOP);
            EXPECT_EQ_BOOL(c.io.linkreset_entry(), false);
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