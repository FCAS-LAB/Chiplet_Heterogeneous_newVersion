#include "d2dadapter/d2d_adapter_constants.hpp"
#include "d2dadapter/parity_generator.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/time_slice.hpp"
#include "utils/common.hpp"
#include "interfaces/fdi.hpp"
#include "interfaces/rdi.hpp"
#include "sideband/sideband_io.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

#if ENABLE_LONG_TIME_CASE_
static const FdiParams fdi_params(8, 8, 32);

TEST (ParityGeneratorTest, GenerateCorrectParityData) {
    auto top = createTopModule<ParityGenerator>(fdi_params);
    auto &c = *top;

    // ======================== signals ========================
    std::vector<Wire<UInt>> snd_data;   // I
    Wire<Bool> snd_data_vld; // I
    std::vector<Wire<UInt>> rcv_data;   // I
    Wire<Bool> rcv_data_vld; // I

    std::vector<Wire<UInt>> parity_data; // O
    Wire<Bool> parity_insert; // O
    Wire<Bool> parity_check; // O
    Wire<Bool> parity_rdy; // I

    std::vector<Wire<Bool>> parity_check_result; // O
    Wire<Bool> parity_check_result_valid; // O
    Wire<UInt> rdi_state; // I

    Wire<Bool> parity_rx_enable; // I
    Wire<Bool> parity_tx_enable; // I
    Wire<UInt> parity_n; // I



    std::vector<BigUInt> io_snd_data(fdi_params.width, 0);
    bool io_snd_data_vld = false;
    std::vector<BigUInt> io_rcv_data(fdi_params.width, 0);
    bool io_rcv_data_vld = false;

    bool io_parity_rdy = false;

    BigUInt io_rdi_state = PhyState::active;

    bool io_parity_rx_enable = false;
    bool io_parity_tx_enable = false;
    BigUInt io_parity_n = ParityN::ONE;

    // ======================== connect ========================
    for (int i = 0; i < fdi_params.width; i++) {
        c.io.snd_data[i].capture(8, io_snd_data[i]);
    }
    c.io.snd_data_vld.capture(io_snd_data_vld);
    for (int i = 0; i < fdi_params.width; i++) {
        c.io.rcv_data[i].capture(8, io_rcv_data[i]);
    }
    c.io.rcv_data_vld.capture(io_rcv_data_vld);

    c.io.parity_rdy.capture(io_parity_rdy);

    c.io.rdi_state.capture(4, io_rdi_state);

    c.io.parity_rx_enable.capture(io_parity_rx_enable);
    c.io.parity_tx_enable.capture(io_parity_tx_enable);
    c.io.parity_n.capture(4, io_parity_n);


    // ======================== run ========================
    BigUInt rand = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    std::vector<BigUInt> parity_data_gold(ParityAmount::PARITY_DATA_NBYTE_1, 0);

    // init
    io_parity_tx_enable = true;
    io_parity_n = ParityN::ONE;
    io_rdi_state = PhyState::active;
    io_parity_rdy = false;
    io_snd_data_vld = false;
    io_rcv_data_vld = false;
    c.step();

    // start sending data
    for (int i = 0; i < ParityAmount::DATA_NBYTE_1 / fdi_params.width; i++) {
        EXPECT_EQ_BOOL(c.io.parity_insert(), false);
        for (int j = 0; j < fdi_params.width; j++) {
            BigUInt data = rand % 256;
            io_snd_data[j] = data;
            //std::cout << "i " << i << " j " << j << std::endl;
            parity_data_gold[(i * fdi_params.width + j) % ParityAmount::PARITY_DATA_NBYTE_1] = parity_data_gold[(i * fdi_params.width + j) % ParityAmount::PARITY_DATA_NBYTE_1] ^ data;
        }
        io_snd_data_vld = false;
        EXPECT_EQ_BOOL(c.io.parity_insert(), false);
        c.step(2);
        io_snd_data_vld = true;
        EXPECT_EQ_BOOL(c.io.parity_insert(), false);
        c.step(1);
        io_snd_data_vld = false;
        c.step(2);
    }
    std::cout << "Start Insert Parity" << std::endl;
    EXPECT_EQ_BOOL(c.io.parity_insert(), true);
    for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_1 / fdi_params.width; i++) {
        c.step(2);
        EXPECT_EQ_BOOL(c.io.parity_insert(), true);
        c.step(2);
        EXPECT_EQ_BOOL(c.io.parity_insert(), true);
        for (int j = 0; j < fdi_params.width; j++) {
            // get the parity
            BigUInt parity = 0;
            for (int c = 0; c < 64; c++) {
                parity = parity ^ ((parity_data_gold[i * fdi_params.width + j % ParityAmount::PARITY_DATA_NBYTE_1] & (1 << c)) >> c);
            }
            EXPECT_EQ_BigUInt(c.io.parity_data[j](), parity);
        }
        io_parity_rdy = true;
        c.step(1);
        io_parity_rdy = false;
        c.step(1);
    }
}


TEST (ParityGeneratorTest, GetCorrectParityResult) {
    auto top = createTopModule<ParityGenerator>(fdi_params);
    auto &c = *top;

    // ======================== signals ========================
    std::vector<Wire<UInt>> snd_data;   // I
    Wire<Bool> snd_data_vld; // I
    std::vector<Wire<UInt>> rcv_data;   // I
    Wire<Bool> rcv_data_vld; // I

    std::vector<Wire<UInt>> parity_data; // O
    Wire<Bool> parity_insert; // O
    Wire<Bool> parity_check; // O
    Wire<Bool> parity_rdy; // I

    std::vector<Wire<Bool>> parity_check_result; // O
    Wire<Bool> parity_check_result_valid; // O
    Wire<UInt> rdi_state; // I

    Wire<Bool> parity_rx_enable; // I
    Wire<Bool> parity_tx_enable; // I
    Wire<UInt> parity_n; // I



    std::vector<BigUInt> io_snd_data(fdi_params.width, 0);
    bool io_snd_data_vld = false;
    std::vector<BigUInt> io_rcv_data(fdi_params.width, 0);
    bool io_rcv_data_vld = false;

    bool io_parity_rdy = false;

    BigUInt io_rdi_state = PhyState::active;

    bool io_parity_rx_enable = false;
    bool io_parity_tx_enable = false;
    BigUInt io_parity_n = ParityN::ONE;

    // ======================== connect ========================
    for (int i = 0; i < fdi_params.width; i++) {
        c.io.snd_data[i].capture(8, io_snd_data[i]);
    }
    c.io.snd_data_vld.capture(io_snd_data_vld);
    for (int i = 0; i < fdi_params.width; i++) {
        c.io.rcv_data[i].capture(8, io_rcv_data[i]);
    }
    c.io.rcv_data_vld.capture(io_rcv_data_vld);

    c.io.parity_rdy.capture(io_parity_rdy);

    c.io.rdi_state.capture(4, io_rdi_state);

    c.io.parity_rx_enable.capture(io_parity_rx_enable);
    c.io.parity_tx_enable.capture(io_parity_tx_enable);
    c.io.parity_n.capture(4, io_parity_n);


    // ======================== run ========================
    BigUInt rand = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    std::vector<BigUInt> parity_data_gold(ParityAmount::PARITY_DATA_NBYTE_1, 0);

    // init
    io_parity_rx_enable = true;
    io_parity_n = ParityN::ONE;
    io_rdi_state = PhyState::active;
    io_parity_rdy = false;
    io_snd_data_vld = false;
    io_rcv_data_vld = false;
    c.step();
    // start sending data
    for (int i = 0; i < ParityAmount::DATA_NBYTE_1 / fdi_params.width; i++) {
        EXPECT_EQ_BOOL(c.io.parity_check(), false);
        for (int j = 0; j < fdi_params.width; j++) {
            BigUInt data = rand % 256;
            io_rcv_data[j] = data;
            //std::cout << "i " << i << " j " << j << std::endl;
            parity_data_gold[(i * fdi_params.width + j) % ParityAmount::PARITY_DATA_NBYTE_1] = parity_data_gold[(i * fdi_params.width + j) % ParityAmount::PARITY_DATA_NBYTE_1] ^ data;
        }
        io_rcv_data_vld = false;
        EXPECT_EQ_BOOL(c.io.parity_check(), false);
        c.step(2);
        io_rcv_data_vld = true;
        EXPECT_EQ_BOOL(c.io.parity_check(), false);
        c.step(1);
        io_rcv_data_vld = false;
        c.step(1);
    }
    EXPECT_EQ_BOOL(c.io.parity_check(), true);
    for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_1 / fdi_params.width; i++) {
        c.step(2);
        EXPECT_EQ_BOOL(c.io.parity_check(), true);
        c.step(2);
        EXPECT_EQ_BOOL(c.io.parity_check(), true);
        for (int j = 0; j < fdi_params.width; j++) {
            // get the parity
            BigUInt parity = 0;
            for (int c = 0; c < 64; c++) {
                parity = parity ^ ((parity_data_gold[i * fdi_params.width + j % ParityAmount::PARITY_DATA_NBYTE_1] & (1 << c)) >> c);
            }
            io_rcv_data[j] = parity;
        }
        io_rcv_data_vld = true;
        c.step(1);
        io_rcv_data_vld = false;
        c.step(1);
    }
    EXPECT_EQ_BOOL(c.io.parity_check(), false);
    for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_1; i++) {
        EXPECT_EQ_BOOL(c.io.parity_check_result[i](), false);
    }
}
#endif // ENABLE_LONG_TIME_CASE_