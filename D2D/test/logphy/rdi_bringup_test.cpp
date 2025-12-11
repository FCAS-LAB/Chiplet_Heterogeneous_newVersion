#include "logphy/rdi_bringup.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

TEST(RdiBringupTest, test) {
    auto top = createTopModule<RdiBringup>();
    auto &c = *top;

    // IOs
    bool io_rdi_io_lp_clk_ack = false;
    bool lp_rdi_io_lp_wake_req = false;
    BigUInt io_rdi_io_lp_state_req = 0;
    bool io_rdi_io_lp_stall_ack = false;
    bool io_rdi_io_lp_link_error = false;

    bool io_sb_train_io_msg_req_ready = false;
    bool io_sb_train_io_msg_req_status_valid = false;
    BigUInt io_sb_train_io_msg_req_status_data = 0;
    BigUInt io_sb_train_io_msg_req_status_status = 0;

    bool io_internal_error = false;
    bool io_intrnal_retrain = false;

    // connect
    c.io.rdi_io.lp_clk_ack.capture(io_rdi_io_lp_clk_ack);
    c.io.rdi_io.lp_wake_req.capture(lp_rdi_io_lp_wake_req);
    c.io.rdi_io.lp_state_req.capture(4, io_rdi_io_lp_state_req);
    c.io.rdi_io.lp_stall_ack.capture(io_rdi_io_lp_stall_ack);
    c.io.rdi_io.lp_link_error.capture(io_rdi_io_lp_link_error);

    c.io.sb_train_io.msg_req.ready.capture(io_sb_train_io_msg_req_ready);
    c.io.sb_train_io.msg_req_status.valid.capture(io_sb_train_io_msg_req_status_valid);
    c.io.sb_train_io.msg_req_status.data.capture(64, io_sb_train_io_msg_req_status_data);
    c.io.sb_train_io.msg_req_status.status.capture(1, io_sb_train_io_msg_req_status_status);

    c.io.internal_error.capture(io_internal_error);
    c.io.internal_retrain.capture(io_intrnal_retrain);

    // run
    c.step();
}