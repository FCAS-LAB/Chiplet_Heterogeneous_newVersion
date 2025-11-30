#include "logphy/logical_phy.hpp"
#include "interfaces/rdi.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include "utils/time_slice.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

TEST (LogPhyTest, CompilationTest) {
    const LinkTrainingParams link_training_params;
    const AfeParams afe_params;
    const RdiParams rdi_params;
    const FdiParams fdi_params;
    const SidebandParams sb_params;
    const AsyncQueueParams lane_async_queue_params;

    const int CLOCK_CYCLE = 100'000;

    // Cannot use createTopModule, because this module needs to set io.sb_afe.rx_clock before propagateClock.
    clearAllClockModules();
    resetSimTime();

    auto m = std::make_shared<LogicalPhy>(1, link_training_params, afe_params, rdi_params, fdi_params, sb_params, lane_async_queue_params);
    m->setModuleName("top");
    m->buildModuleTree("");
    addModuleToClock(createClock(CLOCK_CYCLE, "main_clock"), m);
    //return m;
    auto &c = *m;
    c.io.sb_afe.rx_clock = createClock(CLOCK_CYCLE, "io_sb_afe_rx_clock");
    c.io.mb_afe.fifo_params.clk = createClock(CLOCK_CYCLE, "io_mb_afe_fifo_params_clk");
    m->topPropagateClock();
    EXPECT_NE(c.io.sb_afe.tx_clock, nullptr);
    //TimeSlice::getInstance().showClockTree();

    // This module needs reset
    Wire<Bool> reset;
    reset.capture(false);
    c.setReset(reset);

    // IOs
    bool io_rdi_lp_data_valid = false;
    BigUInt io_rdi_lp_data_bits = 0;
    bool io_rdi_lp_data_irdy = false;
    bool io_rdi_lp_retimer_crd = false;
    BigUInt io_rdi_lp_state_req = 0;
    bool io_rdi_lp_link_error = false;
    bool io_rdi_lp_stall_ack = false;
    bool io_rdi_lp_clk_ack = false;
    bool io_rdi_lp_wake_req = false;
    bool io_rdi_pl_config_credit = false;
    bool io_rdi_lp_config_valid = false;
    BigUInt io_rdi_lp_config_bits = 0;

    bool io_mb_afe_fifo_params_reset = false;
    std::vector<bool> io_mb_afe_tx_data_ready(c.io.mb_afe.tx_data.size());
    std::vector<bool> io_mb_afe_rx_data_valid(c.io.mb_afe.rx_data.size());
    std::vector<BigUInt> io_mb_afe_rx_data_bits(c.io.mb_afe.rx_data.size());
    bool io_mb_afe_pll_lock = false;

    bool io_sb_afe_fifo_params_reset = false;
    BigUInt io_sb_afe_rx_data = 0;
    bool io_sb_afe_pll_lock = false;

    // connect
    c.io.rdi.lp_data.assignValid(io_rdi_lp_data_valid);
    c.io.rdi.lp_data.assignBits(8*rdi_params.width, io_rdi_lp_data_bits);
    c.io.rdi.lp_data_irdy.capture(io_rdi_lp_data_irdy);
    c.io.rdi.lp_retimer_crd.capture(io_rdi_lp_retimer_crd);
    c.io.rdi.lp_state_req.capture(4, io_rdi_lp_state_req);
    c.io.rdi.lp_link_error.capture(io_rdi_lp_link_error);
    c.io.rdi.lp_stall_ack.capture(io_rdi_lp_stall_ack);
    c.io.rdi.lp_clk_ack.capture(io_rdi_lp_clk_ack);
    c.io.rdi.lp_wake_req.capture(io_rdi_lp_wake_req);
    c.io.rdi.pl_config_credit.capture(io_rdi_pl_config_credit);
    c.io.rdi.lp_config.assignValid(io_rdi_lp_config_valid);
    c.io.rdi.lp_config.assignBits(rdi_params.sb_width, io_rdi_lp_config_bits);

    c.io.mb_afe.fifo_params.reset.capture(io_mb_afe_fifo_params_reset);
    for (size_t i = 0; i < c.io.mb_afe.tx_data.size(); i++) {
        c.io.mb_afe.tx_data[i].assignReady(io_mb_afe_tx_data_ready[i]);
    }
    for (size_t i = 0; i < c.io.mb_afe.rx_data.size(); i++) {
        c.io.mb_afe.rx_data[i].assignValid(io_mb_afe_rx_data_valid[i]);
        c.io.mb_afe.rx_data[i].assignBits(afe_params.mb_serializer_ratio, io_mb_afe_rx_data_bits[i]);
    }
    c.io.mb_afe.pll_lock.capture(io_mb_afe_pll_lock);

    c.io.sb_afe.fifo_params.reset.capture(io_sb_afe_fifo_params_reset);
    c.io.sb_afe.rx_data.capture(afe_params.sb_width, io_sb_afe_rx_data);
    c.io.sb_afe.pll_lock.capture(io_sb_afe_pll_lock);

    // run
    run(CLOCK_CYCLE * 10);
}