#include "logphy/link_training_fsm.hpp"
#include "mb_init_fsm_test.hpp"
#include "interfaces/rdi.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;


static const LinkTrainingParams link_training_params{.pll_wait_time=20};
static const SidebandParams sb_params;
static const AfeParams afe_params;
static const RdiParams rdi_params{128, 128};
static const int sb_w = sb_params.sb_node_msg_width;

// IOs
static bool io_mainband_fsm_io_pll_lock = false;
static bool io_sideband_fsm_io_rx_data_valid = false;
static BigUInt io_sideband_fsm_io_rx_data_bits = 0;
static bool io_sideband_fsm_io_pattern_tx_data_ready = false;
static bool io_sideband_fsm_io_packet_tx_data_ready = false;
static bool io_sideband_fsm_io_pll_lock = false;
static bool io_rdi_rdi_bringup_io_lp_clk_ack = false;
static bool io_rdi_rdi_bringup_io_lp_wake_req = false;
static BigUInt io_rdi_rdi_bringup_io_lp_state_req = 0;
static bool io_rdi_rdi_bringup_io_lp_stall_ack = false;
static bool io_rdi_rdi_bringup_io_lp_link_error = false;

void initMB(LinkTrainingFSM &c) {
    EXPECT_EQ_BigUInt(c.io.current_state(), LinkTrainingState::mbInit);

    io_sideband_fsm_io_packet_tx_data_ready = true;
    while (!c.io.sideband_fsm_io.packet_tx_data.isValid()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.packet_tx_data.isValid(), true);
    EXPECT_EQ_BigUInt(
        c.io.sideband_fsm_io.packet_tx_data.bits(),
        formParamsReqMsgMsg(
            true,
            link_training_params.mb_training_params.voltage_swing,
            link_training_params.mb_training_params.maximum_data_rate,
            link_training_params.mb_training_params.clock_mode,
            link_training_params.mb_training_params.clock_phase,
            link_training_params.mb_training_params.module_id,
            link_training_params.mb_training_params.CCPS_ax32
        )
    );
    c.step();
    io_sideband_fsm_io_packet_tx_data_ready = false;

    io_sideband_fsm_io_rx_data_valid = true;
    while (!c.io.sideband_fsm_io.rx_data.isReady()) {
        c.step();
    }
    io_sideband_fsm_io_rx_data_bits = formParamsReqMsgMsg(
        true,
        link_training_params.mb_training_params.voltage_swing,
        link_training_params.mb_training_params.maximum_data_rate,
        link_training_params.mb_training_params.clock_mode,
        link_training_params.mb_training_params.clock_phase,
        link_training_params.mb_training_params.module_id,
        link_training_params.mb_training_params.CCPS_ax32
    );
    c.step();
    io_sideband_fsm_io_rx_data_valid = false;
    c.step(4);

    io_sideband_fsm_io_rx_data_valid = true;
    while (!c.io.sideband_fsm_io.rx_data.isReady()) {
        c.step();
    }
    io_sideband_fsm_io_rx_data_bits = formParamsReqMsgMsg(
        false,
        link_training_params.mb_training_params.voltage_swing,
        link_training_params.mb_training_params.maximum_data_rate,
        link_training_params.mb_training_params.clock_mode,
        link_training_params.mb_training_params.clock_phase,
        link_training_params.mb_training_params.module_id,
        link_training_params.mb_training_params.CCPS_ax32
    );
    c.step();
    io_sideband_fsm_io_rx_data_valid = false;

    io_sideband_fsm_io_packet_tx_data_ready = true;
    while (!c.io.sideband_fsm_io.packet_tx_data.isValid()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.packet_tx_data.isValid(), true);
    EXPECT_EQ_BigUInt(
        c.io.sideband_fsm_io.packet_tx_data.bits(),
        formParamsReqMsgMsg(
            false,
            link_training_params.mb_training_params.voltage_swing,
            link_training_params.mb_training_params.maximum_data_rate,
            link_training_params.mb_training_params.clock_mode,
            link_training_params.mb_training_params.clock_phase,
            link_training_params.mb_training_params.module_id,
            link_training_params.mb_training_params.CCPS_ax32
        )
    );
    c.step();
    io_sideband_fsm_io_packet_tx_data_ready = false;
    c.step(3);
}

void initSB(LinkTrainingFSM &c) {
    EXPECT_EQ_BigUInt(c.io.current_state(), LinkTrainingState::sbInit);

    io_sideband_fsm_io_pattern_tx_data_ready = true;
    while (!c.io.sideband_fsm_io.pattern_tx_data.isValid()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.pattern_tx_data.isValid(), true);
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.pattern_tx_data.isReady(), true);
    EXPECT_EQ_BigUInt(
        c.io.sideband_fsm_io.pattern_tx_data.bits(),
        "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    );
    c.step();
    io_sideband_fsm_io_pattern_tx_data_ready = false;

    io_sideband_fsm_io_pattern_tx_data_ready = true;
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.pattern_tx_data.isValid(), true);
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.pattern_tx_data.isReady(), true);
    EXPECT_EQ_BigUInt(
        c.io.sideband_fsm_io.pattern_tx_data.bits(),
        "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    );
    c.step();
    io_sideband_fsm_io_pattern_tx_data_ready = false;

    io_sideband_fsm_io_rx_data_valid = true;
    io_sideband_fsm_io_rx_data_bits = BigUInt("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isValid(), true);
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isReady(), true);
    c.step();
    io_sideband_fsm_io_rx_data_valid = false;
    c.step(5);

    // rx_data
    io_sideband_fsm_io_rx_data_valid = true;
    while (!c.io.sideband_fsm_io.rx_data.isReady()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isValid(), true);
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isReady(), true);
    io_sideband_fsm_io_rx_data_bits = SBMessage_factory(SBM().SBINIT_OUT_OF_RESET, "PHY", true, "PHY").toBigUInt();
    c.step();
    io_sideband_fsm_io_rx_data_valid = false;

    // packet_tx_data
    io_sideband_fsm_io_packet_tx_data_ready = true;
    while (!c.io.sideband_fsm_io.packet_tx_data.isValid()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.packet_tx_data.isValid(), true);
    EXPECT_EQ_BigUInt(
        c.io.sideband_fsm_io.packet_tx_data.bits(),
        SBMessage_factory(SBM().SBINIT_OUT_OF_RESET, "PHY", true, "PHY").toBigUInt()
    );
    c.step();
    io_sideband_fsm_io_packet_tx_data_ready = false;

    // rx_data
    io_sideband_fsm_io_rx_data_valid = true;
    while (!c.io.sideband_fsm_io.rx_data.isReady()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isValid(), true);
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isReady(), true);
    io_sideband_fsm_io_rx_data_bits = SBMessage_factory(SBM().SBINIT_OUT_OF_RESET, "PHY", true, "PHY").toBigUInt();
    c.step();
    io_sideband_fsm_io_rx_data_valid = false;

    // rx_data
    io_sideband_fsm_io_rx_data_valid = true;
    while (!c.io.sideband_fsm_io.rx_data.isReady()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isValid(), true);
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isReady(), true);
    io_sideband_fsm_io_rx_data_bits = SBMessage_factory(SBM().SBINIT_OUT_OF_RESET, "PHY", true, "PHY").toBigUInt();
    c.step();
    io_sideband_fsm_io_rx_data_valid = false;

    c.step(2);

    // rx_data
    io_sideband_fsm_io_rx_data_valid = true;
    while (!c.io.sideband_fsm_io.rx_data.isReady()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isValid(), true);
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isReady(), true);
    io_sideband_fsm_io_rx_data_bits = SBMessage_factory(SBM().SBINIT_DONE_REQ, "PHY", true, "PHY").toBigUInt();
    c.step();
    io_sideband_fsm_io_rx_data_valid = false;

    // packet_tx_data
    io_sideband_fsm_io_packet_tx_data_ready = true;
    while (!c.io.sideband_fsm_io.packet_tx_data.isValid()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.packet_tx_data.isValid(), true);
    EXPECT_EQ_BigUInt(
        c.io.sideband_fsm_io.packet_tx_data.bits(),
        SBMessage_factory(SBM().SBINIT_DONE_REQ, "PHY", true, "PHY").toBigUInt()
    );
    c.step();
    io_sideband_fsm_io_packet_tx_data_ready = false;
    c.step(3);

    // rx_data
    io_sideband_fsm_io_rx_data_valid = true;
    while (!c.io.sideband_fsm_io.rx_data.isReady()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isValid(), true);
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.rx_data.isReady(), true);
    io_sideband_fsm_io_rx_data_bits = SBMessage_factory(SBM().SBINIT_DONE_RESP, "PHY", true, "PHY").toBigUInt();
    c.step();
    io_sideband_fsm_io_rx_data_valid = false;

    // packet_tx_data
    io_sideband_fsm_io_packet_tx_data_ready = true;
    EXPECT_EQ_BOOL(c.io.sideband_fsm_io.packet_tx_data.isValid(), true);
    EXPECT_EQ_BigUInt(
        c.io.sideband_fsm_io.packet_tx_data.bits(),
        SBMessage_factory(SBM().SBINIT_DONE_RESP, "PHY", true, "PHY").toBigUInt()
    );
    c.step();
    io_sideband_fsm_io_packet_tx_data_ready = false;
    c.step(3);
}

void testTransitionOutOfReset(LinkTrainingFSM &c) {
    EXPECT_EQ_BigUInt(c.io.current_state(), LinkTrainingState::reset);
    EXPECT_EQ_BigUInt(c.io.mainband_fsm_io.tx_freq_sel(), SpeedMode::speed4);
    c.step();
    io_mainband_fsm_io_pll_lock = true;
    io_sideband_fsm_io_pll_lock = true;

    for (int i = 0; i < 30; i++) {
        EXPECT_EQ_BigUInt(c.io.current_state(), LinkTrainingState::reset);
        EXPECT_EQ_BigUInt(c.io.mainband_fsm_io.tx_freq_sel(), SpeedMode::speed4);
        c.step();
        io_mainband_fsm_io_pll_lock = false;
        io_sideband_fsm_io_pll_lock = false;
    }

    io_mainband_fsm_io_pll_lock = true;
    io_sideband_fsm_io_pll_lock = true;
    c.step();
}

void initPorts(LinkTrainingFSM &c) {
    return;
}


TEST (LinkTrainingFSMTest, CorrectlyTransitionBetweenStatesBasicSimulation) {
    auto top = createTopModule<LinkTrainingFSM>(link_training_params, sb_params, afe_params);
    auto &c = *top;

    // connect
    c.io.mainband_fsm_io.pll_lock.capture(io_mainband_fsm_io_pll_lock);
    c.io.sideband_fsm_io.rx_data.assignValid(io_sideband_fsm_io_rx_data_valid);
    c.io.sideband_fsm_io.rx_data.assignBits(sb_w, io_sideband_fsm_io_rx_data_bits);
    c.io.sideband_fsm_io.pattern_tx_data.assignReady(io_sideband_fsm_io_pattern_tx_data_ready);
    c.io.sideband_fsm_io.packet_tx_data.assignReady(io_sideband_fsm_io_packet_tx_data_ready);
    c.io.sideband_fsm_io.pll_lock.capture(io_sideband_fsm_io_pll_lock);
    c.io.rdi.rdi_bringup_io.lp_clk_ack.capture(io_rdi_rdi_bringup_io_lp_clk_ack);
    c.io.rdi.rdi_bringup_io.lp_wake_req.capture(io_rdi_rdi_bringup_io_lp_wake_req);
    c.io.rdi.rdi_bringup_io.lp_state_req.capture(4, io_rdi_rdi_bringup_io_lp_state_req);
    c.io.rdi.rdi_bringup_io.lp_stall_ack.capture(io_rdi_rdi_bringup_io_lp_stall_ack);
    c.io.rdi.rdi_bringup_io.lp_link_error.capture(io_rdi_rdi_bringup_io_lp_link_error);

    // runs
    initPorts(c);
    testTransitionOutOfReset(c);
    initSB(c);
    initMB(c);

    EXPECT_EQ_BOOL(c.io.current_state(), LinkTrainingState::linkInit);

}