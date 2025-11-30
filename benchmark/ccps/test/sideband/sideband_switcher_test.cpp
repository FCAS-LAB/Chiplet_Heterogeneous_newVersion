#include "sideband_switcher_test.hpp"
#include "sideband/sb_msg_encoding.hpp"
#include "utils/wire.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace CCPS;

// =========================== dummyfactory ============================
dummyfactory::dummyfactory() {
    io.output_foryou = (
        []() -> UInt {
            return SBMessage_factory(
                SBM().LINK_MGMT_ADAPTER0_REQ_DISABLE,
                "Protocol_0",
                false,
                "D2D"
            );
        }
    );
    io.output_notforyou = (
        []() -> UInt {
            return SBMessage_factory(
                SBM().MBINIT_REVERSALMB_CLEAR_ERROR_REQ,
                "Protocol_0",
                false,
                "PHY"
            );
        }
    );
}
// =========================== switcher_wrapper ============================
switcher_wrapper::switcher_wrapper() {
    io.inner.node_to_layer_above.connect(s.io.inner.node_to_layer_above);
    s.io.inner.layer_to_node_above.connect(io.inner.layer_to_node_above);
    io.inner.node_to_layer_below.connect(s.io.inner.node_to_layer_below);
    s.io.inner.layer_to_node_below.connect(io.inner.layer_to_node_below);
    s.io.outer.node_to_layer_above.connect(io.outer.node_to_layer_above);
    io.outer.layer_to_node_above.connect(s.io.outer.layer_to_node_above);
    s.io.outer.node_to_layer_below.connect(io.outer.node_to_layer_below);
    io.outer.layer_to_node_below.connect(s.io.outer.layer_to_node_below);
    io.dummy_foryou = (
        [this]() -> UInt {
            return d.io.output_foryou();
        }
    );
    io.dummy_notforyou = (
        [this]() -> UInt {
            return d.io.output_notforyou();
        }
    );
}
 // =========================== racefactory ============================
 racefactory::racefactory() {
    io.output_complete = (
        []() -> UInt {
            return SBMessage_factory(
                SBM().COMP_0,
                "Protocol_0",
                false,
                "D2D"
            );
        }
    );
    io.output_message = (
        []() -> UInt {
            return SBMessage_factory(
                SBM().MBINIT_REVERSALMB_CLEAR_ERROR_REQ,
                "Protocol_0",
                false,
                "PHY"
            );
        }
    );
}
// =========================== race_switcher_wrapper ============================
race_switcher_wrapper::race_switcher_wrapper() {
    io.inner.node_to_layer_above.connect(s.io.inner.node_to_layer_above);
    s.io.inner.layer_to_node_above.connect(io.inner.layer_to_node_above);
    io.inner.node_to_layer_below.connect(s.io.inner.node_to_layer_below);
    s.io.inner.layer_to_node_below.connect(io.inner.layer_to_node_below);
    s.io.outer.node_to_layer_above.connect(io.outer.node_to_layer_above);
    io.outer.layer_to_node_above.connect(s.io.outer.layer_to_node_above);
    s.io.outer.node_to_layer_below.connect(io.outer.node_to_layer_below);
    io.outer.layer_to_node_below.connect(s.io.outer.layer_to_node_below);
    io.output_complete = (
        [this]() -> UInt {
            return d.io.output_complete();
        }
    );
    io.output_message = (
        [this]() -> UInt {
            return d.io.output_message();
        }
    );
}
// ========================== TEST body ========================================
TEST(SidebandSwitcherTests, InstantiateTest) {
    SidebandSwitcher c{0x001, SidebandParams()};
}

TEST(SidebandSwitcherTests, SimpleForYouSanity) {
    switcher_wrapper c;
    SidebandParams sb_params;

    bool io_outer_node_to_layer_above_valid_val = false;
    CCPS::BigUInt io_outer_node_to_layer_above_bits_val = 0;
    bool io_inner_node_to_layer_above_ready_val = false;

    // ===================== connection =======================
    // init
    Wire<Bool>::TPFUNC io_inner_layer_to_node_above_valid_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.layer_to_node_above.assignValid(io_inner_layer_to_node_above_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_above_bits_func{
        [&sb_params]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, 0);
        }
    };
    c.io.inner.layer_to_node_above.assignBits(io_inner_layer_to_node_above_bits_func);

    Wire<Bool>::TPFUNC io_inner_node_to_layer_above_ready_func{
        [&io_inner_node_to_layer_above_ready_val]() -> Bool {
            return Bool(io_inner_node_to_layer_above_ready_val);
        }
    };
    c.io.inner.node_to_layer_above.assignReady(io_inner_node_to_layer_above_ready_func);

    Wire<Bool>::TPFUNC io_inner_layer_to_node_below_valid_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.layer_to_node_below.assignValid(io_inner_layer_to_node_below_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_below_bits_func{
        [&sb_params]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, 0);
        }
    };
    c.io.inner.layer_to_node_below.assignBits(io_inner_layer_to_node_below_bits_func);

    Wire<Bool>::TPFUNC io_inner_node_to_layer_below_ready_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.node_to_layer_below.assignReady(io_inner_node_to_layer_below_ready_func);

    Wire<Bool>::TPFUNC io_outer_node_to_layer_above_valid_func{
        [&io_outer_node_to_layer_above_valid_val]() -> Bool {
            return Bool(io_outer_node_to_layer_above_valid_val);
        }
    };
    c.io.outer.node_to_layer_above.assignValid(io_outer_node_to_layer_above_valid_func);

    Wire<UInt>::TPFUNC io_outer_node_to_layer_above_bits_func{
        [&sb_params, &io_outer_node_to_layer_above_bits_val]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, io_outer_node_to_layer_above_bits_val);
        }
    };
    c.io.outer.node_to_layer_above.assignBits(io_outer_node_to_layer_above_bits_func);

    Wire<Bool>::TPFUNC io_outer_layer_to_node_above_ready_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.outer.layer_to_node_above.assignReady(io_outer_layer_to_node_above_ready_func);

    Wire<Bool>::TPFUNC io_outer_node_to_layer_below_valid_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.outer.node_to_layer_below.assignValid(io_outer_node_to_layer_below_valid_func);

    Wire<UInt>::TPFUNC io_outer_node_to_layer_below_bits_func{
        [&sb_params]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, 0);
        }
    };
    c.io.outer.node_to_layer_below.assignBits(io_outer_node_to_layer_below_bits_func);

    Wire<Bool>::TPFUNC io_outer_layer_to_node_below_ready_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.outer.layer_to_node_below.assignReady(io_outer_layer_to_node_below_ready_func);

    // ====================== step ======================
    // init
    io_outer_node_to_layer_above_valid_val = false;
    io_outer_node_to_layer_above_bits_val = 0;
    io_inner_node_to_layer_above_ready_val = false;

    // send a packet from outer_above to inner_above
    io_outer_node_to_layer_above_valid_val = true;
    io_outer_node_to_layer_above_bits_val = c.io.dummy_foryou().toBigUInt();
    io_inner_node_to_layer_above_ready_val = true;

    // ====================== check =======================
    // check that the packet was received
    EXPECT_EQ_BOOL(
        c.io.inner.node_to_layer_above.isValid(),
        true
    ) << "inner node_to_layer_above valid failed.";
    auto data = c.io.inner.node_to_layer_above.bits().toBigUInt();
    EXPECT_EQ_BigUInt(
        c.io.inner.node_to_layer_above.bits(),
        c.io.dummy_foryou().toBigUInt()
    ) << "inner node_to_layer_above bits failed.";
    EXPECT_EQ_BOOL(
        c.io.outer.node_to_layer_above.isReady(),
        true
    ) << "outer node_to_layer_above ready failed.";
}

TEST(SidebandSwitcherTests, SimpleNotForYouSanity) {
    switcher_wrapper c;
    SidebandParams sb_params;

    bool io_outer_node_to_layer_above_valid_val = false;
    CCPS::BigUInt io_outer_node_to_layer_above_bits_val = 0;
    bool io_outer_layer_to_node_below_ready_val = false;

    // ===================== connection =======================
    // init
    Wire<Bool>::TPFUNC io_inner_layer_to_node_above_valid_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.layer_to_node_above.assignValid(io_inner_layer_to_node_above_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_above_bits_func{
        [&sb_params]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, 0);
        }
    };
    c.io.inner.layer_to_node_above.assignBits(io_inner_layer_to_node_above_bits_func);

    Wire<Bool>::TPFUNC io_inner_node_to_layer_above_ready_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.node_to_layer_above.assignReady(io_inner_node_to_layer_above_ready_func);

    Wire<Bool>::TPFUNC io_inner_layer_to_node_below_valid_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.layer_to_node_below.assignValid(io_inner_layer_to_node_below_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_below_bits_func{
        [&sb_params]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, 0);
        }
    };
    c.io.inner.layer_to_node_below.assignBits(io_inner_layer_to_node_below_bits_func);

    Wire<Bool>::TPFUNC io_inner_node_to_layer_below_ready_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.node_to_layer_below.assignReady(io_inner_node_to_layer_below_ready_func);

    Wire<Bool>::TPFUNC io_outer_node_to_layer_above_valid_func{
        [&io_outer_node_to_layer_above_valid_val]() -> Bool {
            return Bool(io_outer_node_to_layer_above_valid_val);
        }
    };
    c.io.outer.node_to_layer_above.assignValid(io_outer_node_to_layer_above_valid_func);

    Wire<UInt>::TPFUNC io_outer_node_to_layer_above_bits_func{
        [&sb_params, &io_outer_node_to_layer_above_bits_val]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, io_outer_node_to_layer_above_bits_val);
        }
    };
    c.io.outer.node_to_layer_above.assignBits(io_outer_node_to_layer_above_bits_func);

    Wire<Bool>::TPFUNC io_outer_layer_to_node_above_ready_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.outer.layer_to_node_above.assignReady(io_outer_layer_to_node_above_ready_func);

    Wire<Bool>::TPFUNC io_outer_node_to_layer_below_valid_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.outer.node_to_layer_below.assignValid(io_outer_node_to_layer_below_valid_func);

    Wire<UInt>::TPFUNC io_outer_node_to_layer_below_bits_func{
        [&sb_params]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, 0);
        }
    };
    c.io.outer.node_to_layer_below.assignBits(io_outer_node_to_layer_below_bits_func);

    Wire<Bool>::TPFUNC io_outer_layer_to_node_below_ready_func{
        [&io_outer_layer_to_node_below_ready_val]() -> Bool {
            return Bool(io_outer_layer_to_node_below_ready_val);
        }
    };
    c.io.outer.layer_to_node_below.assignReady(io_outer_layer_to_node_below_ready_func);

    // ====================== step ======================
    // init
    io_outer_node_to_layer_above_valid_val = false;
    io_outer_node_to_layer_above_bits_val = 0;
    io_outer_layer_to_node_below_ready_val = false;

    // send a packet from outer_above to inner_above
    io_outer_node_to_layer_above_valid_val = true;
    io_outer_node_to_layer_above_bits_val = c.io.dummy_notforyou().toBigUInt();
    io_outer_layer_to_node_below_ready_val = true;

    // ====================== check =======================
    // check that the packet was received
    EXPECT_EQ_BOOL(
        c.io.inner.node_to_layer_below.isValid(),
        false
    ) << "inner node_to_layer_above valid failed.";
    EXPECT_EQ_BOOL(
        c.io.outer.layer_to_node_below.isValid(),
        true
    ) << "inner node_to_layer_above valid failed.";
    auto data = c.io.outer.layer_to_node_below.bits().toBigUInt();
    EXPECT_EQ_BigUInt(
        c.io.outer.layer_to_node_below.bits(),
        c.io.dummy_notforyou().toBigUInt()
    ) << "inner node_to_layer_above bits failed.";
    EXPECT_EQ_BOOL(
        c.io.outer.node_to_layer_above.isReady(),
        true
    ) << "outer node_to_layer_above ready failed.";
}

TEST(SidebandSwitcherTests, RacingSendingCheck) {
    race_switcher_wrapper c;
    SidebandParams sb_params;

    bool io_outer_node_to_layer_above_ready_val = false;
    bool io_inner_layer_to_node_above_valid_val = false;
    CCPS::BigUInt io_inner_layer_to_node_above_bits_val = 0;

    bool io_outer_node_to_layer_above_valid_val = false;
    CCPS::BigUInt io_outer_node_to_layer_above_bits_val = 0;

    // ===================== connection =======================
    // init
    Wire<Bool>::TPFUNC io_inner_layer_to_node_above_valid_func{
        [&io_inner_layer_to_node_above_valid_val]() -> Bool {
            return Bool(io_inner_layer_to_node_above_valid_val);
        }
    };
    c.io.inner.layer_to_node_above.assignValid(io_inner_layer_to_node_above_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_above_bits_func{
        [&sb_params, &io_inner_layer_to_node_above_valid_val]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, io_inner_layer_to_node_above_valid_val);
        }
    };
    c.io.inner.layer_to_node_above.assignBits(io_inner_layer_to_node_above_bits_func);

    Wire<Bool>::TPFUNC io_inner_node_to_layer_above_ready_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.node_to_layer_above.assignReady(io_inner_node_to_layer_above_ready_func);

    Wire<Bool>::TPFUNC io_inner_layer_to_node_below_valid_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.layer_to_node_below.assignValid(io_inner_layer_to_node_below_valid_func);

    Wire<UInt>::TPFUNC io_inner_layer_to_node_below_bits_func{
        [&sb_params]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, 0);
        }
    };
    c.io.inner.layer_to_node_below.assignBits(io_inner_layer_to_node_below_bits_func);

    Wire<Bool>::TPFUNC io_inner_node_to_layer_below_ready_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.inner.node_to_layer_below.assignReady(io_inner_node_to_layer_below_ready_func);

    Wire<Bool>::TPFUNC io_outer_node_to_layer_above_valid_func{
        [&io_outer_node_to_layer_above_valid_val]() -> Bool {
            return Bool(io_outer_node_to_layer_above_valid_val);
        }
    };
    c.io.outer.node_to_layer_above.assignValid(io_outer_node_to_layer_above_valid_func);

    Wire<UInt>::TPFUNC io_outer_node_to_layer_above_bits_func{
        [&sb_params, &io_outer_node_to_layer_above_bits_val]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, io_outer_node_to_layer_above_bits_val);
        }
    };
    c.io.outer.node_to_layer_above.assignBits(io_outer_node_to_layer_above_bits_func);

    Wire<Bool>::TPFUNC io_outer_layer_to_node_above_ready_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.outer.layer_to_node_above.assignReady(io_outer_layer_to_node_above_ready_func);

    Wire<Bool>::TPFUNC io_outer_node_to_layer_below_valid_func{
        []() -> Bool {
            return Bool(false);
        }
    };
    c.io.outer.node_to_layer_below.assignValid(io_outer_node_to_layer_below_valid_func);

    Wire<UInt>::TPFUNC io_outer_node_to_layer_below_bits_func{
        [&sb_params]() -> UInt {
            return UInt(sb_params.sb_node_msg_width, 0);
        }
    };
    c.io.outer.node_to_layer_below.assignBits(io_outer_node_to_layer_below_bits_func);

    Wire<Bool>::TPFUNC io_outer_layer_to_node_below_ready_func{
        []() -> Bool {
            return Bool(true);
        }
    };
    c.io.outer.layer_to_node_below.assignReady(io_outer_layer_to_node_below_ready_func);

    //Wire<Bool>::TPFUNC io_outer_node_to_layer_above_ready_func{
    //    [&io_outer_node_to_layer_above_ready_val]() -> Bool {
    //        return Bool(io_outer_node_to_layer_above_ready_val);
    //    }
    //};
    //c.io.outer.node_to_layer_above.assignReady(io_outer_node_to_layer_above_ready_func);

    io_outer_node_to_layer_above_ready_val = true;
    // send a high priority complete message from inner layer to node above
    io_inner_layer_to_node_above_valid_val = true;
    io_inner_layer_to_node_above_bits_val = c.io.output_complete().toBigUInt();
    std::cout << "io_inner_layer_to_node_above_bits_val: " << io_inner_layer_to_node_above_bits_val << std::endl;

    // send a low priority message from outer node below to node above
    io_outer_node_to_layer_above_valid_val = true;
    io_outer_node_to_layer_above_bits_val = c.io.output_message().toBigUInt();
    std::cout << "io_outer_node_to_layer_above_bits_val: " << io_outer_node_to_layer_above_bits_val << std::endl;
}