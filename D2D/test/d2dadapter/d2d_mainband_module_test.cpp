#include "d2dadapter/d2d_adapter_constants.hpp"
#include "d2dadapter/d2d_mainband_module.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/time_slice.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static FdiParams fdi_params{8, 8, 32};
static RdiParams rdi_params{8, 32};
static SidebandParams sb_params;

TEST (D2DMainbandModuleTest, PassMainbandDataFromFdiToRdi) {
    auto top = createTopModule<D2DMainbandModule>(
        fdi_params,
        rdi_params,
        sb_params
    );
    auto &c = *top;

    // ======================== signals ========================
    //protocol to d2d
    bool io_fdi_lp_irdy = false;
    bool io_fdi_lp_valid = false;
    BigUInt io_fdi_lp_data = 0;
    BigUInt io_fdi_lp_stream_proto_stack = 0;
    BigUInt io_fdi_lp_stream_proto_type = 0;
    // d2d to physical
    bool io_rdi_pl_trdy = false;
    // physical to d2d
    bool io_rdi_pl_valid = false;
    BigUInt io_rdi_pl_data = 0;
    BigUInt io_d2d_state = PhyState::active;
    bool io_mainband_stallreq = false;
    bool io_parity_insert = false;
    BigUInt io_parity_data = 0;
    bool io_parity_check = false;

    // ======================== connect ========================
    c.io.fdi_lp_irdy.capture(io_fdi_lp_irdy);
    c.io.fdi_lp_valid.capture(io_fdi_lp_valid);
    c.io.fdi_lp_data.capture(8*fdi_params.width, io_fdi_lp_data);
    c.io.fdi_lp_stream.proto_stack.capture(4, io_fdi_lp_stream_proto_stack);
    c.io.fdi_lp_stream.proto_type.capture(4, io_fdi_lp_stream_proto_type);
    c.io.rdi_pl_trdy.capture(io_rdi_pl_trdy);
    c.io.rdi_pl_valid.capture(io_rdi_pl_valid);
    c.io.rdi_pl_data.capture(8*rdi_params.width, io_rdi_pl_data);
    c.io.d2d_state.capture(4, io_d2d_state);
    c.io.mainband_stallreq.capture(io_mainband_stallreq);
    c.io.parity_insert.capture(io_parity_insert);
    c.io.parity_data.capture(8*fdi_params.width, io_parity_data);
    c.io.parity_check.capture(io_parity_check);

    // ======================== run ========================
    BigUInt rand = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    // init
    io_fdi_lp_irdy = false;
    io_fdi_lp_valid = false;
    io_parity_insert = false;
    io_rdi_pl_trdy = false;
    io_d2d_state = PhyState::active;
    io_mainband_stallreq = false;
    c.step();
    // ensure no absurd data goes to rdi
    for(int i = 0; i < 10; i++){
        EXPECT_EQ_BOOL(c.io.rdi_lp_valid(), false);
        EXPECT_EQ_BOOL(c.io.rdi_lp_irdy(), false);
        c.step();
    }

    // fdi send a data to mainband
    BigUInt data = rand % 256;
    io_fdi_lp_data = data;
    io_fdi_lp_irdy = true;
    io_fdi_lp_valid = true;

    while (!c.io.fdi_pl_trdy()) {
        EXPECT_EQ_BOOL(c.io.rdi_lp_valid(), false);
        EXPECT_EQ_BOOL(c.io.rdi_lp_irdy(), false);
        c.step();
    }
    // data taken by mainband module // check when the data is to be sent
    while (!c.io.rdi_lp_valid() || !c.io.rdi_lp_irdy()) {
        c.step();
    }
    // check if the data match the original one
    io_rdi_pl_trdy = true;
    EXPECT_EQ_BigUInt(c.io.rdi_lp_data(), data);
}


TEST (D2DMainbandModuleTest, PassParityDataFromToRdi) {
    auto top = createTopModule<D2DMainbandModule>(
        fdi_params,
        rdi_params,
        sb_params
    );
    auto &c = *top;

    // ======================== signals ========================
    //protocol to d2d
    bool io_fdi_lp_irdy = false;
    bool io_fdi_lp_valid = false;
    BigUInt io_fdi_lp_data = 0;
    BigUInt io_fdi_lp_stream_proto_stack = 0;
    BigUInt io_fdi_lp_stream_proto_type = 0;
    // d2d to physical
    bool io_rdi_pl_trdy = false;
    // physical to d2d
    bool io_rdi_pl_valid = false;
    BigUInt io_rdi_pl_data = 0;
    BigUInt io_d2d_state = PhyState::active;
    bool io_mainband_stallreq = false;
    bool io_parity_insert = false;
    BigUInt io_parity_data = 0;
    bool io_parity_check = false;

    BigUInt rand = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    BigUInt data = rand % 256;

    // ======================== connect ========================
    c.io.fdi_lp_irdy.capture(io_fdi_lp_irdy);
    c.io.fdi_lp_valid.capture(io_fdi_lp_valid);
    c.io.fdi_lp_data.capture(8*fdi_params.width, io_fdi_lp_data);
    c.io.fdi_lp_stream.proto_stack.capture(4, io_fdi_lp_stream_proto_stack);
    c.io.fdi_lp_stream.proto_type.capture(4, io_fdi_lp_stream_proto_type);
    c.io.rdi_pl_trdy.capture(io_rdi_pl_trdy);
    c.io.rdi_pl_valid.capture(io_rdi_pl_valid);
    c.io.rdi_pl_data.capture(8*rdi_params.width, io_rdi_pl_data);
    c.io.d2d_state.capture(4, io_d2d_state);
    c.io.mainband_stallreq.capture(io_mainband_stallreq);
    c.io.parity_insert.capture(io_parity_insert);
    c.io.parity_data.capture(8*fdi_params.width, io_parity_data);
    c.io.parity_check.capture(io_parity_check);

    // ======================== run ========================
    // init
    io_fdi_lp_irdy = false;
    io_fdi_lp_valid = false;
    io_parity_check = false;
    io_d2d_state = PhyState::active;
    io_mainband_stallreq = false;
    c.step();
    // ensure no absurd data goes to rdi
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.rdi_lp_valid(), false);
        EXPECT_EQ_BOOL(c.io.rdi_lp_irdy(), false);
        c.step();
    }

    // parity send data
    // start sending data
    io_parity_data = data;
    io_parity_insert = true;

    // data taken by mainband module // check when the data is to be sent
    while (!c.io.rdi_lp_valid() || !c.io.rdi_lp_irdy()) {
        c.step();
    }
    // check if the data matches the original one
    io_rdi_pl_trdy = true;
    EXPECT_EQ_BigUInt(c.io.rdi_lp_data(), data);
}


TEST (D2DMainbandModuleTest, PassMainbandDataFromRdiToFdi) {
    auto top = createTopModule<D2DMainbandModule>(
        fdi_params,
        rdi_params,
        sb_params
    );
    auto &c = *top;

    // ======================== signals ========================
    //protocol to d2d
    bool io_fdi_lp_irdy = false;
    bool io_fdi_lp_valid = false;
    BigUInt io_fdi_lp_data = 0;
    BigUInt io_fdi_lp_stream_proto_stack = 0;
    BigUInt io_fdi_lp_stream_proto_type = 0;
    // d2d to physical
    bool io_rdi_pl_trdy = false;
    // physical to d2d
    bool io_rdi_pl_valid = false;
    BigUInt io_rdi_pl_data = 0;
    BigUInt io_d2d_state = PhyState::active;
    bool io_mainband_stallreq = false;
    bool io_parity_insert = false;
    BigUInt io_parity_data = 0;
    bool io_parity_check = false;

    BigUInt rand = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    BigUInt data = rand % 256;

    // ======================== connect ========================
    c.io.fdi_lp_irdy.capture(io_fdi_lp_irdy);
    c.io.fdi_lp_valid.capture(io_fdi_lp_valid);
    c.io.fdi_lp_data.capture(8*fdi_params.width, io_fdi_lp_data);
    c.io.fdi_lp_stream.proto_stack.capture(4, io_fdi_lp_stream_proto_stack);
    c.io.fdi_lp_stream.proto_type.capture(4, io_fdi_lp_stream_proto_type);
    c.io.rdi_pl_trdy.capture(io_rdi_pl_trdy);
    c.io.rdi_pl_valid.capture(io_rdi_pl_valid);
    c.io.rdi_pl_data.capture(8*rdi_params.width, io_rdi_pl_data);
    c.io.d2d_state.capture(4, io_d2d_state);
    c.io.mainband_stallreq.capture(io_mainband_stallreq);
    c.io.parity_insert.capture(io_parity_insert);
    c.io.parity_data.capture(8*fdi_params.width, io_parity_data);
    c.io.parity_check.capture(io_parity_check);

    // ======================== run ========================
    // init
    io_fdi_lp_irdy = false;
    io_fdi_lp_valid = false;
    io_parity_check = false;
    io_d2d_state = PhyState::active;
    io_mainband_stallreq = false;
    c.step();

    // ensure no absurd data goes to fdi
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.fdi_pl_valid(), false);
        c.step();
    }

    // rdi send a data to mainband
    io_rdi_pl_data = data;
    io_rdi_pl_valid = true;
    c.step();

    // data taken by mainband module // check when the data is to be sent
    while (!c.io.fdi_pl_valid()) {
        c.step();
    }
    // check if the data matches the original one
    EXPECT_EQ_BigUInt(c.io.fdi_pl_data(), data);
}


TEST (D2DMainbandModuleTest, NotPassParityDataFromRdiToFdi) {
    auto top = createTopModule<D2DMainbandModule>(
        fdi_params,
        rdi_params,
        sb_params
    );
    auto &c = *top;

    // ======================== signals ========================
    //protocol to d2d
    bool io_fdi_lp_irdy = false;
    bool io_fdi_lp_valid = false;
    BigUInt io_fdi_lp_data = 0;
    BigUInt io_fdi_lp_stream_proto_stack = 0;
    BigUInt io_fdi_lp_stream_proto_type = 0;
    // d2d to physical
    bool io_rdi_pl_trdy = false;
    // physical to d2d
    bool io_rdi_pl_valid = false;
    BigUInt io_rdi_pl_data = 0;
    BigUInt io_d2d_state = PhyState::active;
    bool io_mainband_stallreq = false;
    bool io_parity_insert = false;
    BigUInt io_parity_data = 0;
    bool io_parity_check = false;

    BigUInt rand = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    BigUInt data = rand % 256;

    // ======================== connect ========================
    c.io.fdi_lp_irdy.capture(io_fdi_lp_irdy);
    c.io.fdi_lp_valid.capture(io_fdi_lp_valid);
    c.io.fdi_lp_data.capture(8*fdi_params.width, io_fdi_lp_data);
    c.io.fdi_lp_stream.proto_stack.capture(4, io_fdi_lp_stream_proto_stack);
    c.io.fdi_lp_stream.proto_type.capture(4, io_fdi_lp_stream_proto_type);
    c.io.rdi_pl_trdy.capture(io_rdi_pl_trdy);
    c.io.rdi_pl_valid.capture(io_rdi_pl_valid);
    c.io.rdi_pl_data.capture(8*rdi_params.width, io_rdi_pl_data);
    c.io.d2d_state.capture(4, io_d2d_state);
    c.io.mainband_stallreq.capture(io_mainband_stallreq);
    c.io.parity_insert.capture(io_parity_insert);
    c.io.parity_data.capture(8*fdi_params.width, io_parity_data);
    c.io.parity_check.capture(io_parity_check);

    // ======================== run ========================
    // init
    io_fdi_lp_irdy = false;
    io_fdi_lp_valid = false;
    io_parity_check = false;
    io_d2d_state = PhyState::active;
    io_mainband_stallreq = false;
    c.step();

    // ensure no absurd data goes to fdi
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.fdi_pl_valid(), false);
        c.step();
    }

    // rdi send a data to mainband
    io_rdi_pl_data = data;
    io_parity_check = true;
    io_rdi_pl_valid = true;
    c.step();

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.fdi_pl_valid(), false);
        c.step();
    }

    // send another data
    io_rdi_pl_data = data + 1;
    io_rdi_pl_valid = true;
    EXPECT_EQ_BOOL(c.io.fdi_pl_valid(), false);
    io_parity_check = false;
    io_rdi_pl_valid = true;
    // data taken by mainband module // check when the data is to be sent
    while (!c.io.fdi_pl_valid()) {
        c.step();
    }
    // check if the data match the original one
    EXPECT_EQ_BigUInt(c.io.fdi_pl_data(), data+1);
}