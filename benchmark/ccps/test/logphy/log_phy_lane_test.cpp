#include "logphy/lanes.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static const AfeParams afe_params;
static const AsyncQueueParams queue_params;
static const int mb_lanes = afe_params.mb_lanes;
static const int mb_serializer_ratio = afe_params.mb_serializer_ratio;

TEST (LogPhyLaneTest, CorrectlyMapTxBytesToTheirLanes) {
    auto top = createTopModule<SimLanes>(afe_params, queue_params);
    auto &c = *top;

    // ======================== signals ========================
    std::vector<bool> io_mainband_io_tx_data_ready(mb_lanes, false);
    std::vector<bool> io_mainband_io_rx_data_valid(mb_lanes, false);
    std::vector<BigUInt> io_mainband_io_rx_data_bits(mb_lanes, 0);
    bool io_mainband_lane_io_tx_data_valid = false;
    BigUInt io_mainband_lane_io_tx_data_bits = 0;

    // ======================== connect ========================
    EXPECT_EQ(c.io.mainband_io.tx_data.size(), mb_lanes);
    for (size_t i = 0; i < c.io.mainband_io.tx_data.size(); i++) {
        // vector operator [] returns an rvalue, which is unexpected.
        c.io.mainband_io.tx_data[i].assignReady(
            [&io_mainband_io_tx_data_ready, i]() -> Bool {
                return Bool(io_mainband_io_tx_data_ready[i]);
            }
        );
    }
    EXPECT_EQ(c.io.mainband_io.rx_data.size(), mb_lanes);
    for (size_t i = 0; i < c.io.mainband_io.rx_data.size(); i++) {
        c.io.mainband_io.rx_data[i].assignValid(io_mainband_io_rx_data_valid[i]);
        c.io.mainband_io.rx_data[i].assignBits(mb_serializer_ratio, io_mainband_io_rx_data_bits[i]);
    }
    c.io.mainband_lane_io.tx_data.assignValid(io_mainband_lane_io_tx_data_valid);
    c.io.mainband_lane_io.tx_data.assignBits(mb_lanes * mb_serializer_ratio, io_mainband_lane_io_tx_data_bits);

    // ======================== run ========================
    c.step();
    io_mainband_lane_io_tx_data_valid = true;
    BigUInt data("0x123456789abcdef00fedcba98765432111112222333344445555666677778888");
    io_mainband_lane_io_tx_data_bits = data;
    c.step();
    io_mainband_lane_io_tx_data_valid = false;

    const char *cvec[] = {
        "0x1211",
        "0x3411",
        "0x5622",
        "0x7822",
        "0x9a33",
        "0xbc33",
        "0xde44",
        "0xf044",
        "0x0f55",
        "0xed55",
        "0xcb66",
        "0xa966",
        "0x8777",
        "0x6577",
        "0x4388",
        "0x2188"
    };

    std::vector<BigUInt> vec;
    for (size_t i = 0; i < sizeof(cvec) / sizeof(cvec[0]); i++) {
        vec.emplace_back(cvec[i]);
    }

    for (int i = 0; i < mb_lanes; i++) {
        io_mainband_io_tx_data_ready[i] = true;
        EXPECT_EQ_BOOL(c.io.mainband_io.tx_data[i].isValid(), true);
        EXPECT_EQ_BigUInt(c.io.mainband_io.tx_data[i].bits(), vec[i]);
    }
    c.step();

    for (int i = 0; i < mb_lanes; i++) {
        EXPECT_EQ_BOOL(c.io.mainband_io.tx_data[i].isValid(), false) << "data mismatch, i " << i;
        c.step();
    }
}

TEST (LogPhyLaneTest, CorrectlyMapRxBytesToTheirLanes) {
    auto top = createTopModule<SimLanes>(afe_params, queue_params);
    auto &c = *top;

    // ======================== signals ========================
    std::vector<bool> io_mainband_io_tx_data_ready(mb_lanes, false);
    std::vector<bool> io_mainband_io_rx_data_valid(mb_lanes, false);
    std::vector<BigUInt> io_mainband_io_rx_data_bits(mb_lanes, 0);
    bool io_mainband_lane_io_tx_data_valid = false;
    BigUInt io_mainband_lane_io_tx_data_bits = 0;

    // ======================== connect ========================
    EXPECT_EQ(c.io.mainband_io.tx_data.size(), mb_lanes);
    for (size_t i = 0; i < c.io.mainband_io.tx_data.size(); i++) {
        // vector operator [] returns an rvalue, which is unexpected.
        c.io.mainband_io.tx_data[i].assignReady(
            [&io_mainband_io_tx_data_ready, i]() -> Bool {
                return Bool(io_mainband_io_tx_data_ready[i]);
            }
        );
    }
    EXPECT_EQ(c.io.mainband_io.rx_data.size(), mb_lanes);
    for (size_t i = 0; i < c.io.mainband_io.rx_data.size(); i++) {
        c.io.mainband_io.rx_data[i].assignValid(
            [&io_mainband_io_rx_data_valid, i]() -> Bool {
                return Bool(io_mainband_io_rx_data_valid[i]);
            }
        );
        c.io.mainband_io.rx_data[i].assignBits(mb_serializer_ratio, io_mainband_io_rx_data_bits[i]);
    }
    c.io.mainband_lane_io.tx_data.assignValid(io_mainband_lane_io_tx_data_valid);
    c.io.mainband_lane_io.tx_data.assignBits(mb_lanes * mb_serializer_ratio, io_mainband_lane_io_tx_data_bits);

    // ======================== run ========================
    c.step();

    const char *cvec[] = {
        "0x1211",
        "0x3411",
        "0x5622",
        "0x7822",
        "0x9a33",
        "0xbc33",
        "0xde44",
        "0xf044",
        "0x0f55",
        "0xed55",
        "0xcb66",
        "0xa966",
        "0x8777",
        "0x6577",
        "0x4388",
        "0x2188"
    };

    std::vector<BigUInt> vec;
    for (size_t i = 0; i < sizeof(cvec) / sizeof(cvec[0]); i++) {
        vec.emplace_back(cvec[i]);
    }

    for (int i = 0; i < mb_lanes; i++) {
        io_mainband_io_rx_data_valid[i] = true;
        io_mainband_io_rx_data_bits[i] = vec[i];
    }
    c.step();
    for (int i = 0; i < mb_lanes; i++) {
        io_mainband_io_rx_data_valid[i] = false;
    }

    EXPECT_EQ_BOOL(c.io.mainband_lane_io.rx_data.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.mainband_lane_io.rx_data.bits(), BigUInt("0x123456789abcdef00fedcba98765432111112222333344445555666677778888"));
    c.step();
    EXPECT_EQ_BOOL(c.io.mainband_lane_io.rx_data.isValid(), false);
}