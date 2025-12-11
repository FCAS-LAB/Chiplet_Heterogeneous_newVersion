#include "d2dadapter/link_management_controller.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include "interfaces/fdi.hpp"
#include "interfaces/rdi.hpp"
#include "sideband/sideband_io.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static const FdiParams fdi_params(8, 8, 32);
static const RdiParams rdi_params(8, 32);
static const SidebandParams sb_params;

TEST (LinkManagementControllerTest, InitAndThenLinkError) {
    auto top = createTopModule<LinkManagementController>(fdi_params, rdi_params, sb_params);
    auto &c = *top;

    // ======================== signals ========================
    BigUInt io_fdi_lp_state_req = PhyStateReq::nop;
    bool io_fdi_lp_linkerror = false;
    bool io_fdi_lp_rx_active_sts = false;

    BigUInt io_rdi_pl_state_sts = PhyState::active;
    bool io_rdi_pl_inband_pres = false;

    BigUInt io_sb_rcv = 0;
    bool io_sb_rdy = false;

    bool io_linkmgmt_stalldone = false;
    BigUInt io_cycles_1us = 0;
    bool io_parity_tx_sw_en = false;
    bool io_parity_rx_sw_en = false;

    // ======================== connect ========================
    c.io.fdi_lp_state_req.capture(4, io_fdi_lp_state_req);
    c.io.fdi_lp_linkerror.capture(io_fdi_lp_linkerror);
    c.io.fdi_lp_rx_active_sts.capture(io_fdi_lp_rx_active_sts);

    c.io.rdi_pl_state_sts.capture(4, io_rdi_pl_state_sts);
    c.io.rdi_pl_inband_pres.capture(io_rdi_pl_inband_pres);

    c.io.sb_rcv.capture(6, io_sb_rcv);
    c.io.sb_rdy.capture(io_sb_rdy);

    c.io.linkmgmt_stalldone.capture(io_linkmgmt_stalldone);
    c.io.cycles_1us.capture(32, io_cycles_1us);
    c.io.parity_tx_sw_en.capture(io_parity_tx_sw_en);
    c.io.parity_rx_sw_en.capture(io_parity_rx_sw_en);

    // ======================== run ========================
    BigUInt rand = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;

    // init
    io_cycles_1us = 100;
    io_fdi_lp_state_req = PhyStateReq::nop;
    io_fdi_lp_linkerror = false;
    io_rdi_pl_state_sts = PhyState::reset;
    io_rdi_pl_inband_pres = false;

    io_linkmgmt_stalldone = false;
    c.step();
    for (int i = 0; i < 10; i++) {
        // should not give any signal
        EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::nop);
        EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), false);
        EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
        EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
        EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
        c.step();
    }
    // nothing
    for (int i = 0; i < 5; i++) {
        //println("Start Link Init!!")
        io_rdi_pl_inband_pres = true;
        c.step();

        while (c.io.rdi_lp_state_req().toBigUInt() != PhyStateReq::active) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::nop);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), false);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        // waiting for RDI to be active
        for (int i = 0; i < 10; i++) {
            // should not give any signal
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), false);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        io_rdi_pl_state_sts = PhyState::active;
        // RDI bring up complete

        // parameter exchange
        //println("Parameter Exchage!!")
        while (c.io.sb_snd().toBigUInt() != SideBandMessage::ADV_CAP) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), false);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        // give adv_cap rcv
        io_sb_rcv = SideBandMessage::ADV_CAP;
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), false);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::ADV_CAP);
            c.step();
        }
        // sideband send
        io_sb_rdy = true;
        c.step();
        io_sb_rdy = false;
        c.step();
        // FDI bring up
        // wait for FDI bring up begins
        while (!c.io.fdi_pl_inband_pres()) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), false);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), true);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        // inband_pres = true -> begin FDI bring up
        // Condition one: protocol layer request first
        io_fdi_lp_state_req = PhyStateReq::nop;
        c.step();
        io_fdi_lp_state_req = PhyStateReq::active;
        c.step();
        // should send req sideband
        while (c.io.sb_snd().toBigUInt() != SideBandMessage::REQ_ACTIVE) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), true);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), true);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::REQ_ACTIVE);
            c.step();
        }
        io_sb_rdy = true;
        c.step();
        io_sb_rdy = false;
        c.step();
        // at the same time, request for active
        io_sb_rcv = SideBandMessage::REQ_ACTIVE;
        c.step();
        // wait for rx_active_req
        while (!c.io.fdi_pl_rx_active_req()) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), true);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        // response come back
        io_sb_rcv = SideBandMessage::RSP_ACTIVE;
        c.step();
        // module wait for active_sts
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), true);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), true);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        io_fdi_lp_rx_active_sts = true;
        c.step();
        while (c.io.sb_snd().toBigUInt() != SideBandMessage::RSP_ACTIVE) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), true);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), true);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), true);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), true);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::RSP_ACTIVE);
            c.step();
        }
        io_sb_rdy = true;
        c.step();
        io_sb_rdy = false;
        c.step();
        // should complete handshake to active
        while (c.io.fdi_pl_state_sts().toBigUInt() != PhyState::active) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), true);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), true);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::reset);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BOOL(c.io.fdi_pl_inband_pres(), true);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), true);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::active);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        // link error
        io_fdi_lp_linkerror = true;
        c.step();
        // should request rdi to be linkerror
        while (!c.io.rdi_lp_linkerror()) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::active);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::active);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        // rdi goes to link error
        io_rdi_pl_state_sts = PhyState::linkError;
        c.step();
        // should go to linkerror
        while (c.io.fdi_pl_state_sts().toBigUInt() != PhyState::linkError) {
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::active);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::linkError);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        // linkerror resolved
        io_fdi_lp_linkerror = false;
        c.step();
        io_fdi_lp_state_req = PhyStateReq::active;
        c.step();
        // should request active to rdi
        while (c.io.rdi_lp_state_req().toBigUInt() != PhyStateReq::active) {
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::linkError);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::active);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::linkError);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        // rdi resets
        io_rdi_pl_state_sts = PhyState::reset;
        c.step();
        // should deassert rx
        while (c.io.fdi_pl_rx_active_req()) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::nop);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), true);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::linkError);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::nop);
            EXPECT_EQ_BOOL(c.io.fdi_pl_rx_active_req(), false);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::linkError);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
        io_fdi_lp_rx_active_sts = false;
        c.step();
        // should also goes to resets
        while (c.io.fdi_pl_state_sts().toBigUInt() != PhyState::reset) {
            EXPECT_EQ_BigUInt(c.io.rdi_lp_state_req(), PhyStateReq::nop);
            EXPECT_EQ_BigUInt(c.io.fdi_pl_state_sts(), PhyState::linkError);
            EXPECT_EQ_BigUInt(c.io.sb_snd(), SideBandMessage::NOP);
            c.step();
        }
    }
}
