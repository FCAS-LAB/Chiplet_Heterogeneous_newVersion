#include "d2dadapter/d2d_adapter.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static const FdiParams fdi_params(8, 8, 32);
static const RdiParams rdi_params(8, 32);
static const SidebandParams sb_params;

TEST (D2DAdapterTest, DontKnowHowTOCheck) {
    auto top = createTopModule<D2DAdapter>(fdi_params, rdi_params, sb_params);
    auto &c = *top;

    // ======================== signals ========================
    // fdi
    bool io_fdi_lp_data_valid = false;
    BigUInt io_fdi_lp_data_bits = 0;
    bool io_fdi_lp_data_irdy = false;
    bool io_fdi_lp_retimer_crd = false;
    bool io_fdi_lp_corrupt_crc = false;
    bool io_fdi_lp_dllp_valid = false;
    BigUInt io_fdi_lp_dllp_bits = 0;
    bool io_fdi_lp_dllp_ofc = false;
    BigUInt io_fdi_lp_stream_proto_stack = 0;
    BigUInt io_fdi_lp_stream_proto_type = 0;
    BigUInt io_fdi_lp_state_req = 0;
    bool io_fdi_lp_link_error = false;
    bool io_fdi_lp_rx_active_status = false;
    bool io_fdi_lp_stall_ack = false;
    bool io_fdi_lp_clk_ack = false;
    bool io_fdi_lp_wake_req = false;
    bool io_fdi_pl_config_credit = false;
    bool io_fdi_lp_config_valid = false;
    BigUInt io_fdi_lp_config_bits = 0;

    // rdi
    bool io_rdi_lp_data_ready = false;
    bool io_rdi_pl_data_valid = false;
    BigUInt io_rdi_pl_data_bits = 0;
    bool io_rdi_pl_retimer_crd = false;
    BigUInt io_rdi_pl_state_status = 0;
    bool io_rdi_pl_inband_pres = false;
    bool io_rdi_pl_error = false;
    bool io_rdi_pl_correctable_error = false;
    bool io_rdi_pl_non_fatal_error = false;
    bool io_rdi_pl_train_error = false;
    bool io_rdi_pl_phy_in_recenter = false;
    bool io_rdi_pl_stall_req = false;
    BigUInt io_rdi_pl_speed_mode = 0;
    BigUInt io_rdi_pl_link_width = 0;
    bool io_rdi_pl_clk_req = false;
    bool io_rdi_pl_wake_ack = false;
    bool io_rdi_pl_config_valid = false;
    BigUInt io_rdi_pl_config_bits = 0;
    bool io_rdi_lp_config_credit = false;

    // ======================== connect ========================
    // fdi
    c.io.fdi.lp_data.assignValid(io_fdi_lp_data_valid);
    c.io.fdi.lp_data.assignBits(8 * fdi_params.width, io_fdi_lp_data_bits);
    c.io.fdi.lp_data_irdy.capture(io_fdi_lp_data_irdy);
    c.io.fdi.lp_retimer_crd.capture(io_fdi_lp_retimer_crd);
    c.io.fdi.lp_corrupt_crc.capture(io_fdi_lp_corrupt_crc);
    c.io.fdi.lp_dllp.assignValid(io_fdi_lp_dllp_valid);
    c.io.fdi.lp_dllp.assignBits(fdi_params.dllp_width, io_fdi_lp_dllp_bits);
    c.io.fdi.lp_dllp_ofc.capture(io_fdi_lp_dllp_ofc);
    c.io.fdi.lp_stream.proto_stack.capture(4, io_fdi_lp_stream_proto_stack);
    c.io.fdi.lp_stream.proto_type.capture(4, io_fdi_lp_stream_proto_type);
    c.io.fdi.lp_state_req.capture(4, io_fdi_lp_state_req);
    c.io.fdi.lp_link_error.capture(io_fdi_lp_link_error);
    c.io.fdi.lp_rx_active_status.capture(io_fdi_lp_rx_active_status);
    c.io.fdi.lp_stall_ack.capture(io_fdi_lp_stall_ack);
    c.io.fdi.lp_clk_ack.capture(io_fdi_lp_clk_ack);
    c.io.fdi.lp_wake_req.capture(io_fdi_lp_wake_req);
    c.io.fdi.pl_config_credit.capture(io_fdi_pl_config_credit);
    c.io.fdi.lp_config.assignValid(io_fdi_lp_config_valid);
    c.io.fdi.lp_config.assignBits(fdi_params.sb_width, io_fdi_lp_config_bits);

    // rdi
    c.io.rdi.lp_data.assignReady(io_rdi_lp_data_ready);
    c.io.rdi.pl_data.assignValid(io_rdi_pl_data_valid);
    c.io.rdi.pl_data.assignBits(8 * rdi_params.width, io_rdi_pl_data_bits);
    c.io.rdi.pl_retimer_crd.capture(io_rdi_pl_retimer_crd);
    c.io.rdi.pl_state_status.capture(4, io_rdi_pl_state_status);
    c.io.rdi.pl_inband_pres.capture(io_rdi_pl_inband_pres);
    c.io.rdi.pl_error.capture(io_rdi_pl_error);
    c.io.rdi.pl_correctable_error.capture(io_rdi_pl_correctable_error);
    c.io.rdi.pl_non_fatal_error.capture(io_rdi_pl_non_fatal_error);
    c.io.rdi.pl_train_error.capture(io_rdi_pl_train_error);
    c.io.rdi.pl_phy_in_recenter.capture(io_rdi_pl_phy_in_recenter);
    c.io.rdi.pl_stall_req.capture(io_rdi_pl_stall_req);
    c.io.rdi.pl_speed_mode.capture(3, io_rdi_pl_speed_mode);
    c.io.rdi.pl_link_width.capture(3, io_rdi_pl_link_width);
    c.io.rdi.pl_clk_req.capture(io_rdi_pl_clk_req);
    c.io.rdi.pl_wake_ack.capture(io_rdi_pl_wake_ack);
    c.io.rdi.pl_config.assignValid(io_rdi_pl_config_valid);
    c.io.rdi.pl_config.assignBits(rdi_params.sb_width, io_rdi_pl_config_bits);
    c.io.rdi.lp_config_credit.capture(io_rdi_lp_config_credit);


    // ======================== run ========================
    BigUInt rand = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;

    // init
    c.step();
    c.step();
    c.step();

}