#include "d2dadapter/d2d_adapter_constants.hpp"
#include "d2dadapter/link_init_submodule.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/time_slice.hpp"
#include "utils/common.hpp"
#include "interfaces/fdi.hpp"
#include "interfaces/rdi.hpp"
#include "sideband/sideband_io.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static FdiParams fdi_params{8, 8, 32};
static RdiParams rdi_params{8, 32};
static SidebandParams sb_params;

TEST (LinkInitSubmoduleTest, BringUpFdiAndRdiForLinkInitialization) {
    auto top = createTopModule<LinkInitSubmodule>();
    auto &c = *top;

    // ======================== signals ========================
    //protocol to d2d
    BigUInt io_fdi_lp_state_req = 0;
    BigUInt io_fdi_lp_state_req_prev = 0;
    bool io_fdi_lp_rxactive_sts = 0;

    BigUInt io_rdi_pl_state_sts = 0;
    bool io_rdi_pl_inband_pres = 0;

    BigUInt io_link_state = 0;
    BigUInt io_linkinit_sb_rcv = 0;
    bool io_linkinit_sb_rdy = 0;

    // ======================== connect ========================
    c.io.fdi_lp_state_req.capture(4, io_fdi_lp_state_req);
    c.io.fdi_lp_state_req_prev.capture(4, io_fdi_lp_state_req_prev);
    c.io.fdi_lp_rxactive_sts.capture(io_fdi_lp_rxactive_sts);

    c.io.rdi_pl_state_sts.capture(4, io_rdi_pl_state_sts);
    c.io.rdi_pl_inband_pres.capture(io_rdi_pl_inband_pres);
    c.io.link_state.capture(4, io_link_state);
    c.io.linkinit_sb_rcv.capture(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, io_linkinit_sb_rcv);
    c.io.linkinit_sb_rdy.capture(io_linkinit_sb_rdy);

    // ======================== run ========================
    // init
    io_fdi_lp_state_req = PhyStateReq::nop;
    io_fdi_lp_state_req_prev = PhyStateReq::nop;
    io_fdi_lp_rxactive_sts = false;
    io_rdi_pl_state_sts = PhyState::reset;
    io_rdi_pl_inband_pres = false;
    io_linkinit_sb_rcv = SideBandMessage::NOP;
    io_linkinit_sb_rdy = false;
    io_link_state = PhyState::reset;
    c.step();

    // Start test
    std::cout << "Test started" << std::endl;
    for (int i = 0; i < 10; ++i) {
        // Should not give any signal
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::nop);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        c.step();
    }

    // RDI bring up
    io_rdi_pl_inband_pres = true;
    c.step();
    while (c.io.linkinit_rdi_lp_state_req().toBigUInt() != PhyStateReq::active) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::nop);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        c.step();
    }

    // Waiting for RDI to be active
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    io_rdi_pl_state_sts = PhyState::active;
    //c.step();
    std::cout << "RDI Bringup complete" << std::endl;

    // Parameter exchange
    while (c.io.linkinit_sb_snd().toBigUInt() != SideBandMessage::ADV_CAP) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        c.step();
    }

    // Give ADV_CAP receive signal
    io_linkinit_sb_rcv = SideBandMessage::ADV_CAP;
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::ADV_CAP);
        c.step();
    }
    // Sideband send
    io_linkinit_sb_rdy = true;
    c.step();
    io_linkinit_sb_rdy = false;
    c.step();

    // FDI bring up
    // Wait for FDI bring up to begin
    while (!c.io.linkinit_fdi_pl_inband_pres()) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        c.step();
    }

    // Begin FDI bring up
    io_fdi_lp_state_req = PhyStateReq::active;
    c.step();

    // Should send REQ_ACTIVE sideband
    while (c.io.linkinit_sb_snd().toBigUInt() != SideBandMessage::REQ_ACTIVE) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), true);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        c.step();
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), true);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::REQ_ACTIVE);
        c.step();
    }

    io_linkinit_sb_rdy = true;
    c.step();
    io_linkinit_sb_rdy = false;
    c.step();

    // Simultaneously, request for active
    io_linkinit_sb_rcv = SideBandMessage::REQ_ACTIVE;
    c.step();

    // Wait for RX_ACTIVE_REQ
    while (!c.io.linkinit_fdi_pl_rxactive_req()) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), true);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        c.step();
    }

    // Response comes back
    io_linkinit_sb_rcv = SideBandMessage::RSP_ACTIVE;
    c.step();

    // Module waits for active_sts
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), true);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), true);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        c.step();
    }

    io_fdi_lp_rxactive_sts = true;
    c.step();

    while (c.io.linkinit_sb_snd().toBigUInt() != SideBandMessage::RSP_ACTIVE) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), true);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), true);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::RSP_ACTIVE);
        c.step();
    }

    io_linkinit_sb_rdy = true;
    c.step();
    io_linkinit_sb_rdy = false;
    c.step();

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), true);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), true);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::active);
        EXPECT_EQ_BOOL(c.io.active_entry(), true);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        c.step();
    }

    // Leave reset
    io_link_state = PhyState::active;
    c.step();

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::nop);
        c.step();
    }

    io_link_state = PhyState::reset;
    io_rdi_pl_inband_pres = false;
    c.step();

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_rxactive_req(), false);
        EXPECT_EQ_BOOL(c.io.linkinit_fdi_pl_inband_pres(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_sb_snd(), SideBandMessage::NOP);
        EXPECT_EQ_BOOL(c.io.active_entry(), false);
        EXPECT_EQ_BigUInt(c.io.linkinit_rdi_lp_state_req(), PhyStateReq::nop);
        c.step();
    }
}