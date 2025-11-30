#include "logphy/pattern_generator.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static const AfeParams afe_params;
static const int mb_lanes = afe_params.mb_lanes;
static const int mb_serializer_ratio = afe_params.mb_serializer_ratio;
static const SidebandParams sb_params;
static const int sb_w = sb_params.sb_node_msg_width;


// ======================== signals ========================
static bool io_pattern_generator_io_transmit_req_valid = false;
static BigUInt io_pattern_generator_io_transmit_req_pattern = 0;
static BigUInt io_pattern_generator_io_transmit_req_timeout_cycles = 0;
static bool io_pattern_generator_io_transmit_req_sideband = 0;
static bool io_pattern_generator_io_transmit_pattern_status_ready = false;
static bool io_sideband_lane_io_tx_data_ready = false;
static BigUInt io_sideband_lane_io_rx_data_bits = 0;
static bool io_sideband_lane_io_rx_data_valid = false;

void testClockPatternSideband(PatternGenerator &c) {
    EXPECT_EQ_BOOL(c.io.pattern_generator_io.transmit_req.ready(), true);
    EXPECT_EQ_BOOL(c.io.sideband_lane_io.rx_data.isReady(), false);
    EXPECT_EQ_BOOL(c.io.sideband_lane_io.tx_data.fire(), false);
    EXPECT_EQ_BOOL(c.io.pattern_generator_io.transmit_pattern_status.isValid(), false);
    c.step();

    io_pattern_generator_io_transmit_req_valid = true;
    io_pattern_generator_io_transmit_req_pattern = TransmitPattern::CLOCK_64_LOW_32;
    io_pattern_generator_io_transmit_req_timeout_cycles = 80;
    io_pattern_generator_io_transmit_req_sideband = true;
    c.step();
    io_pattern_generator_io_transmit_req_valid = false;
    c.step();

    BigUInt test_vector[2] = {
        BigUInt("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
        BigUInt("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
    };

    for (size_t i = 0; i < sizeof(test_vector) / sizeof(test_vector[0]); i++) {
        io_sideband_lane_io_rx_data_valid = true;
        io_sideband_lane_io_rx_data_bits = test_vector[i];
        io_sideband_lane_io_tx_data_ready = true;
        EXPECT_EQ_BOOL(c.io.sideband_lane_io.rx_data.isReady(), true);
        EXPECT_EQ_BOOL(c.io.sideband_lane_io.tx_data.isValid(), true);
        EXPECT_EQ_BigUInt(c.io.sideband_lane_io.tx_data.bits(), test_vector[i]);
        c.step();
    }
    io_sideband_lane_io_rx_data_valid = false;
    io_sideband_lane_io_tx_data_ready = false;

    io_pattern_generator_io_transmit_pattern_status_ready = true;
    while (!c.io.pattern_generator_io.transmit_pattern_status.isValid()) {
        EXPECT_EQ_BOOL(c.io.pattern_generator_io.transmit_pattern_status.isValid(), false);
        EXPECT_EQ_BOOL(c.io.pattern_generator_io.transmit_pattern_status.isReady(), true);
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.pattern_generator_io.transmit_pattern_status.isValid(), true);
    EXPECT_EQ_BOOL(c.io.pattern_generator_io.transmit_pattern_status.isReady(), true);
    EXPECT_EQ_BigUInt(c.io.pattern_generator_io.transmit_pattern_status.bits(), static_cast<BigUInt>(MessageRequestStatusType::SUCCESS));
    c.step();
    io_pattern_generator_io_transmit_pattern_status_ready = false;
    c.step();
}

TEST (PatternGeneratorTest, DetectClockPatternNoDelay) {
    auto top = createTopModule<PatternGenerator>(afe_params, sb_params);
    auto &c = *top;

    // ======================== connect ========================
    c.io.pattern_generator_io.transmit_req.valid.capture(io_pattern_generator_io_transmit_req_valid);
    c.io.pattern_generator_io.transmit_req.pattern.capture(1, io_pattern_generator_io_transmit_req_pattern);
    c.io.pattern_generator_io.transmit_req.timeout_cycles.capture(32, io_pattern_generator_io_transmit_req_timeout_cycles);
    c.io.pattern_generator_io.transmit_req.sideband.capture(io_pattern_generator_io_transmit_req_sideband);
    c.io.pattern_generator_io.transmit_pattern_status.assignReady(io_pattern_generator_io_transmit_pattern_status_ready);
    c.io.sideband_lane_io.tx_data.assignReady(io_sideband_lane_io_tx_data_ready);
    c.io.sideband_lane_io.rx_data.assignBits(sb_w, io_sideband_lane_io_rx_data_bits);
    c.io.sideband_lane_io.rx_data.assignValid(io_sideband_lane_io_rx_data_valid);

    // ======================== run ========================
    testClockPatternSideband(c);
}

TEST (PatternGeneratorTest, DetectClockPatternNoDelayTwice) {
    auto top = createTopModule<PatternGenerator>(afe_params, sb_params);
    auto &c = *top;

    // ======================== connect ========================
    c.io.pattern_generator_io.transmit_req.valid.capture(io_pattern_generator_io_transmit_req_valid);
    c.io.pattern_generator_io.transmit_req.pattern.capture(1, io_pattern_generator_io_transmit_req_pattern);
    c.io.pattern_generator_io.transmit_req.timeout_cycles.capture(32, io_pattern_generator_io_transmit_req_timeout_cycles);
    c.io.pattern_generator_io.transmit_req.sideband.capture(io_pattern_generator_io_transmit_req_sideband);
    c.io.pattern_generator_io.transmit_pattern_status.assignReady(io_pattern_generator_io_transmit_pattern_status_ready);
    c.io.sideband_lane_io.tx_data.assignReady(io_sideband_lane_io_tx_data_ready);
    c.io.sideband_lane_io.rx_data.assignBits(sb_w, io_sideband_lane_io_rx_data_bits);
    c.io.sideband_lane_io.rx_data.assignValid(io_sideband_lane_io_rx_data_valid);

    // ======================== run ========================
    testClockPatternSideband(c);
    testClockPatternSideband(c);
}