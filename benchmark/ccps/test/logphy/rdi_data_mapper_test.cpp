#include "logphy/rdi_data_mapper.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static const RdiParams rdi_params(128, 128);
static const AfeParams afe_params;

TEST (RdiDataMapperTest, CorrectlyOutputRxLaneData) {
    auto top = createTopModule<RdiDataMapper>(rdi_params, afe_params);
    auto &c = *top;

    // ======================== signals ========================
    bool io_rdi_lp_data_valid = false;
    BigUInt io_rdi_lp_data_bits = 0;
    bool io_rdi_lp_data_irdy = false;
    bool io_mainband_lane_io_tx_data_ready = false;
    bool io_mainband_lane_io_rx_data_valid = false;
    BigUInt io_mainband_lane_io_rx_data_bits = 0;

    // ======================== connect ========================
    c.io.rdi.lp_data.assignValid(io_rdi_lp_data_valid);
    c.io.rdi.lp_data.assignBits(8 * rdi_params.width, io_rdi_lp_data_bits);
    c.io.rdi.lp_data_irdy.capture(io_rdi_lp_data_irdy);
    c.io.mainband_lane_io.tx_data.assignReady(io_mainband_lane_io_tx_data_ready);
    c.io.mainband_lane_io.rx_data.assignValid(io_mainband_lane_io_rx_data_valid);
    c.io.mainband_lane_io.rx_data.assignBits(afe_params.mb_lanes * afe_params.mb_serializer_ratio, io_mainband_lane_io_rx_data_bits);

    // ======================== run ========================
    std::vector<BigUInt> data;
    const char *data_initializer_list[] = {
        "0x123456789abcdef00fedcba98765432111112222333344445555666677778888",
        "0x22222222333344445555666688888888223456889abcdef00fedcba988654322",
        "0x124466789abcdef00fedcba98766442111112222444444446666666677778888",
        "0x11113333333344445555666677778888133456789aacdef00fedcaa987654331"
    };
    for (auto &d : data_initializer_list) {
        data.emplace_back(d);
    }
    //std::cout << "data[0]: " << std::hex << data[0] << std::endl;

    BigUInt data_uint{"0x123456789abcdef00fedcba9876543211111222233334444555566667777888822222222333344445555666688888888223456889abcdef00fedcba988654322124466789abcdef00fedcba9876644211111222244444444666666667777888811113333333344445555666677778888133456789aacdef00fedcaa987654331"};
    //std::cout << "data_uint 0x" << std::hex << data_uint << std::endl;
    io_mainband_lane_io_rx_data_valid = false;
    c.step();
    for (int i = 0; i < 4; i++) {
        io_mainband_lane_io_rx_data_valid = false;
        for (int j = 0; j < 10; j++) {
            EXPECT_EQ_BOOL(c.io.rdi.pl_data.isValid(), false);
            c.step();
        }
        io_mainband_lane_io_rx_data_valid = true;
        io_mainband_lane_io_rx_data_bits = data[i];
        c.step();
        io_mainband_lane_io_rx_data_valid = false;
    }
    EXPECT_EQ_BOOL(c.io.rdi.pl_data.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.rdi.pl_data.bits(), data_uint);
    c.step();
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.rdi.pl_data.isValid(), false);
        c.step();
    }
}

TEST (RdiDataMapperTest, CorrectlyOutputTxLaneData) {
    auto top = createTopModule<RdiDataMapper>(rdi_params, afe_params);
    auto &c = *top;

    // ======================== signals ========================
    bool io_rdi_lp_data_valid = false;
    BigUInt io_rdi_lp_data_bits = 0;
    bool io_rdi_lp_data_irdy = false;
    bool io_mainband_lane_io_tx_data_ready = false;
    bool io_mainband_lane_io_rx_data_valid = false;
    BigUInt io_mainband_lane_io_rx_data_bits = 0;

    // ======================== connect ========================
    c.io.rdi.lp_data.assignValid(io_rdi_lp_data_valid);
    c.io.rdi.lp_data.assignBits(8 * rdi_params.width, io_rdi_lp_data_bits);
    c.io.rdi.lp_data_irdy.capture(io_rdi_lp_data_irdy);
    c.io.mainband_lane_io.tx_data.assignReady(io_mainband_lane_io_tx_data_ready);
    c.io.mainband_lane_io.rx_data.assignValid(io_mainband_lane_io_rx_data_valid);
    c.io.mainband_lane_io.rx_data.assignBits(afe_params.mb_lanes * afe_params.mb_serializer_ratio, io_mainband_lane_io_rx_data_bits);

    // ======================== run ========================
    std::vector<BigUInt> data;
    const char *data_initializer_list[] = {
        "0x123456789abcdef00fedcba98765432111112222333344445555666677778888",
        "0x22222222333344445555666688888888223456889abcdef00fedcba988654322",
        "0x124466789abcdef00fedcba98766442111112222444444446666666677778888",
        "0x11113333333344445555666677778888133456789aacdef00fedcaa987654331"
    };
    for (auto &d : data_initializer_list) {
        data.emplace_back(d);
    }
    //std::cout << "data[0]: " << std::hex << data[0] << std::endl;

    BigUInt data_uint{"0x123456789abcdef00fedcba9876543211111222233334444555566667777888822222222333344445555666688888888223456889abcdef00fedcba988654322124466789abcdef00fedcba9876644211111222244444444666666667777888811113333333344445555666677778888133456789aacdef00fedcaa987654331"};
    //std::cout << "data_uint 0x" << std::hex << data_uint << std::endl;

    io_mainband_lane_io_rx_data_valid = false;
    io_rdi_lp_data_valid = false;
    EXPECT_EQ_BOOL(c.io.rdi.lp_data_irdy(), false);
    EXPECT_EQ_BOOL(c.io.rdi.lp_data.isReady(), true);
    c.step();

    io_rdi_lp_data_valid = true;
    io_rdi_lp_data_irdy = true;
    io_rdi_lp_data_bits = data_uint;
    EXPECT_EQ_BOOL(c.io.rdi.lp_data.isReady(), true);
    c.step();

    io_rdi_lp_data_valid = false;

    for (int i = 0; i < 4; i++) {
        io_mainband_lane_io_tx_data_ready = false;
        for (int j = 0; j < 10; j++) {
            EXPECT_EQ_BOOL(c.io.rdi.lp_data.isReady(), false);
            c.step();
        }
        io_mainband_lane_io_tx_data_ready = true;
        EXPECT_EQ_BOOL(c.io.mainband_lane_io.tx_data.isValid(), true);
        EXPECT_EQ_BOOL(c.io.mainband_lane_io.tx_data.isReady(), true);
        EXPECT_EQ_BigUInt(c.io.mainband_lane_io.tx_data.bits(), data[i]);
        c.step();
    }

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.mainband_lane_io.tx_data.isValid(), false);
        EXPECT_EQ_BOOL(c.io.rdi.lp_data.isReady(), true);
        c.step();
    }
}
