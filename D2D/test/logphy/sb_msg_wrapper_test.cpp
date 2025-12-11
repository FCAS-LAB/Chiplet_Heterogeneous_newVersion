#include "logphy/sb_msg_wrapper.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include "sideband/sb_msg_encoding.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static const SidebandParams sb_params;
static const int sb_w = sb_params.sb_node_msg_width;


// signals
static bool io_train_io_msg_req_valid = false;
static BigUInt io_train_io_msg_req_msg = 0;
static BigUInt io_train_io_msg_req_timeout_cycles = 0;
static bool io_train_io_msg_req_status_ready = false;
static bool io_lane_io_tx_data_ready = false;
static BigUInt io_lane_io_rx_data_bits = 0;
static bool io_lane_io_rx_data_valid = false;

void testSBInitOutOfReset(SBMsgWrapper &c) {
    EXPECT_EQ_BOOL(c.io.lane_io.rx_data.isReady(), false);
    EXPECT_EQ_BOOL(c.io.lane_io.tx_data.isValid(), false);
    EXPECT_EQ_BOOL(c.io.train_io.msg_req.ready(), true);
    EXPECT_EQ_BOOL(c.io.train_io.msg_req_status.valid(), false);
    c.step();

    UInt sb_msg = SBMessage_factory(
        SBM().SBINIT_OUT_OF_RESET,
        "PHY",
        true,
        "PHY",
        UInt(64, 0),
        UInt(16, 0)
    );
    io_train_io_msg_req_valid = true;
    io_train_io_msg_req_msg = sb_msg.toBigUInt();
    io_train_io_msg_req_timeout_cycles = 80;
    EXPECT_EQ_BOOL(c.io.train_io.msg_req.ready(), true);
    c.step();
    io_train_io_msg_req_valid = false;

    EXPECT_EQ_BOOL(c.io.train_io.msg_req.ready(), false);
    for (int i = 0; i < 4; i++) {
        io_lane_io_tx_data_ready = true;
        EXPECT_EQ_BigUInt(c.io.lane_io.tx_data.bits(), sb_msg.toBigUInt());
        EXPECT_EQ_BOOL(c.io.lane_io.tx_data.isValid(), true);
        EXPECT_EQ_BOOL(c.io.lane_io.tx_data.isReady(), true);
        c.step();
    }
    io_lane_io_tx_data_ready = false;
    io_lane_io_rx_data_valid = true;
    io_lane_io_rx_data_bits = sb_msg.toBigUInt();
    EXPECT_EQ_BOOL(c.io.lane_io.rx_data.isReady(), true);
    c.step();
    io_lane_io_rx_data_valid = false;
    io_train_io_msg_req_status_ready = true;
    while (!c.io.train_io.msg_req_status.valid()) {
        EXPECT_EQ_BOOL(c.io.train_io.msg_req_status.valid(), false);
        EXPECT_EQ_BOOL(c.io.train_io.msg_req_status.ready(), true);
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.train_io.msg_req_status.valid(), true);
    EXPECT_EQ_BOOL(c.io.train_io.msg_req_status.ready(), true);
    EXPECT_EQ_BigUInt(c.io.train_io.msg_req_status.status(), MessageRequestStatusType::SUCCESS);
    EXPECT_EQ_BigUInt(c.io.train_io.msg_req_status.data(), 0);
    c.step();
    io_train_io_msg_req_status_ready = false;
    EXPECT_EQ_BOOL(c.io.train_io.msg_req_status.valid(), false);
    EXPECT_EQ_BOOL(c.io.train_io.msg_req_status.ready(), false);
    c.step();
}

TEST (SBMsgWrapperTest, CorrectlyExchangeSBOutOfResetMessage) {
    auto top = createTopModule<SBMsgWrapper>(sb_params);
    auto &c = *top;


    // connect
    c.io.train_io.msg_req.valid.capture(io_train_io_msg_req_valid);
    c.io.train_io.msg_req.msg.capture(128, io_train_io_msg_req_msg);
    c.io.train_io.msg_req.timeout_cycles.capture(64, io_train_io_msg_req_timeout_cycles);
    c.io.train_io.msg_req_status.ready.capture(io_train_io_msg_req_status_ready);
    c.io.lane_io.tx_data.assignReady(io_lane_io_tx_data_ready);
    c.io.lane_io.rx_data.assignBits(sb_w, io_lane_io_rx_data_bits);
    c.io.lane_io.rx_data.assignValid(io_lane_io_rx_data_valid);

    // run
    testSBInitOutOfReset(c);
}

TEST (SBMsgWrapperTest, CorrectlyExchangeSBOutOfResetMessageTwice) {
    auto top = createTopModule<SBMsgWrapper>(sb_params);
    auto &c = *top;


    // connect
    c.io.train_io.msg_req.valid.capture(io_train_io_msg_req_valid);
    c.io.train_io.msg_req.msg.capture(128, io_train_io_msg_req_msg);
    c.io.train_io.msg_req.timeout_cycles.capture(64, io_train_io_msg_req_timeout_cycles);
    c.io.train_io.msg_req_status.ready.capture(io_train_io_msg_req_status_ready);
    c.io.lane_io.tx_data.assignReady(io_lane_io_tx_data_ready);
    c.io.lane_io.rx_data.assignBits(sb_w, io_lane_io_rx_data_bits);
    c.io.lane_io.rx_data.assignValid(io_lane_io_rx_data_valid);

    // run
    testSBInitOutOfReset(c);
    testSBInitOutOfReset(c);
}