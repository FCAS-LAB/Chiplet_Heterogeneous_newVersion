#include "sideband/sideband_node.hpp"
#include "sideband/sideband_channel.hpp"
#include "test_utils.hpp"
#include "sideband_switcher_test.hpp"
#include "utils/time_slice.hpp"
#include "utils/common.hpp"
#include "sideband/sb_msg_encoding.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static const FdiParams fdi_params{8, 8, 32};
static const FdiParams simple_fdiParams{8, 8, 1};
static const SidebandParams sb_params;
static int msg_w = sb_params.sb_node_msg_width;
static int sb_w = fdi_params.sb_width;

TEST(ChannelTest, InstantiateD2DChannel) {
    auto top = createTopModule<D2DSidebandChannel>(sb_params, fdi_params, 1);
    auto &c = *top;

    bool io_to_upper_layer_tx_credit = false;
    BigUInt io_to_upper_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    bool io_to_upper_layer_rx_valid = false;
    bool io_to_lower_layer_tx_credit = false;
    BigUInt io_to_lower_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    bool io_to_lower_layer_rx_valid = false;

    bool io_inner_node_to_layer_above_ready = false;
    bool io_inner_layer_to_node_above_valid = false;
    BigUInt io_inner_layer_to_node_above_bits = 0;
    bool io_inner_node_to_layer_below_ready = false;
    bool io_inner_layer_to_node_below_valid = false;
    BigUInt io_inner_layer_to_node_below_bits = 0;

    // ======================== connect ========================
    c.io.to_upper_layer.tx.credit.capture(io_to_upper_layer_tx_credit);
    c.io.to_upper_layer.rx.bits.capture(sb_w, io_to_upper_layer_rx_bits);
    c.io.to_upper_layer.rx.valid.capture(io_to_upper_layer_rx_valid);
    c.io.to_lower_layer.tx.credit.capture(io_to_lower_layer_tx_credit);
    c.io.to_lower_layer.rx.bits.capture(sb_w, io_to_lower_layer_rx_bits);
    c.io.to_lower_layer.rx.valid.capture(io_to_lower_layer_rx_valid);

    c.io.inner.node_to_layer_above.assignReady(io_inner_node_to_layer_above_ready);
    c.io.inner.layer_to_node_above.assignValid(io_inner_layer_to_node_above_valid);
    c.io.inner.layer_to_node_above.assignBits(msg_w, io_inner_layer_to_node_above_bits);
    c.io.inner.node_to_layer_below.assignReady(io_inner_node_to_layer_below_ready);
    c.io.inner.layer_to_node_below.assignValid(io_inner_layer_to_node_below_valid);
    c.io.inner.layer_to_node_below.assignBits(msg_w, io_inner_layer_to_node_below_bits);

    // ======================== run ========================
    run(c.getClock()->getPeriod());
    run(c.getClock()->getPeriod());
}

//TEST(ChannelTest, InstantiatePHYChannel) {
//    auto top = createTopModule<PHYSidebandChannel>(sb_params, fdi_params, 2);
//    auto &c = *top;
//
//    bool io_to_upper_layer_tx_credit = false;
//    BigUInt io_to_upper_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
//    bool io_to_upper_layer_rx_valid = false;
//    BigUInt io_to_lower_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
//
//    BigUInt io_inner_input_mode = 0;
//    BigUInt io_inner_rx_mode = 0;
//
//    bool io_inner_switcher_bundle_node_to_layer_above_ready = false;
//    bool io_inner_switcher_bundle_layer_to_node_above_valid = false;
//    BigUInt io_inner_switcher_bundle_layer_to_node_above_bits = 0;
//    bool io_inner_switcher_bundle_node_to_layer_below_ready = false;
//    bool io_inner_switcher_bundle_layer_to_node_below_valid = false;
//    BigUInt io_inner_switcher_bundle_layer_to_node_below_bits = 0;
//
//    // ======================== connect ========================
//    c.io.to_upper_layer.tx.credit.capture(io_to_upper_layer_tx_credit);
//    c.io.to_upper_layer.rx.bits.capture(sb_w, io_to_upper_layer_rx_bits);
//    c.io.to_upper_layer.rx.valid.capture(io_to_upper_layer_rx_valid);
//    c.io.to_lower_layer.rx.bits.capture(1, io_to_lower_layer_rx_bits);
//    c.io.to_lower_layer.rx.clock = // TODO
//
//    c.io.inner.node_to_layer_above.assignReady(io_inner_node_to_layer_above_ready);
//    c.io.inner.layer_to_node_above.assignValid(io_inner_layer_to_node_above_valid);
//    c.io.inner.layer_to_node_above.assignBits(msg_w, io_inner_layer_to_node_above_bits);
//    c.io.inner.node_to_layer_below.assignReady(io_inner_node_to_layer_below_ready);
//    c.io.inner.layer_to_node_below.assignValid(io_inner_layer_to_node_below_valid);
//    c.io.inner.layer_to_node_below.assignBits(msg_w, io_inner_layer_to_node_below_bits);
//
//    // ======================== run ========================
//    run(c.getClock()->getPeriod());
//    run(c.getClock()->getPeriod());
//}

class ChannelWrapper: public WireModule {
public:
    struct {
        D2DSidebandChannelIO channel;
        Wire<UInt> dummy_foryou;
        Wire<UInt> dummy_notforyou;
    } io;

    ChannelWrapper() {
        // Instantiate submodules
        s = createSubmodule<D2DSidebandChannel>("D2DSidebandChannel", sb_params, fdi_params, 1);
        d = createSubmodule<dummyfactory>("dummyfactory");

        // connect
        io.channel.to_upper_layer.tx.bits = s->io.to_upper_layer.tx.bits;
        io.channel.to_upper_layer.tx.valid = s->io.to_upper_layer.tx.valid;
        s->io.to_upper_layer.tx.credit = io.channel.to_upper_layer.tx.credit;

        s->io.to_upper_layer.rx.bits = io.channel.to_upper_layer.rx.bits;
        s->io.to_upper_layer.rx.valid = io.channel.to_upper_layer.rx.valid;
        io.channel.to_upper_layer.rx.credit = s->io.to_upper_layer.rx.credit;

        io.channel.to_lower_layer.tx.bits = s->io.to_lower_layer.tx.bits;
        io.channel.to_lower_layer.tx.valid = s->io.to_lower_layer.tx.valid;
        s->io.to_lower_layer.tx.credit = io.channel.to_lower_layer.tx.credit;

        s->io.to_lower_layer.rx.bits = io.channel.to_lower_layer.rx.bits;
        s->io.to_lower_layer.rx.valid = io.channel.to_lower_layer.rx.valid;
        io.channel.to_lower_layer.rx.credit = s->io.to_lower_layer.rx.credit;

        io.channel.inner.node_to_layer_above.connect(s->io.inner.node_to_layer_above);
        s->io.inner.layer_to_node_above.connect(io.channel.inner.layer_to_node_above);
        io.channel.inner.node_to_layer_below.connect(s->io.inner.node_to_layer_below);
        s->io.inner.layer_to_node_below.connect(io.channel.inner.layer_to_node_below);

        io.dummy_foryou = d->io.output_foryou;
        io.dummy_notforyou = d->io.output_notforyou;
    }

private:
    ModulePtr<D2DSidebandChannel> s;
    ModulePtr<dummyfactory> d;

};

TEST(ChannelTest, InstantiateTest) {
    auto top = createTopModule<ChannelWrapper>();
    auto &c = *top;

    bool io_channel_to_upper_layer_tx_credit = false;
    BigUInt io_channel_to_upper_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    bool io_channel_to_upper_layer_rx_valid = false;
    bool io_channel_to_lower_layer_tx_credit = false;
    BigUInt io_channel_to_lower_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    bool io_channel_to_lower_layer_rx_valid = false;

    bool io_channel_inner_node_to_layer_above_ready = false;
    bool io_channel_inner_layer_to_node_above_valid = false;
    BigUInt io_channel_inner_layer_to_node_above_bits = 0;
    bool io_channel_inner_node_to_layer_below_ready = false;
    bool io_channel_inner_layer_to_node_below_valid = false;
    BigUInt io_channel_inner_layer_to_node_below_bits = 0;

    // ======================== connect ========================
    c.io.channel.to_upper_layer.tx.credit.capture(io_channel_to_upper_layer_tx_credit);
    c.io.channel.to_upper_layer.rx.bits.capture(sb_w, io_channel_to_upper_layer_rx_bits);
    c.io.channel.to_upper_layer.rx.valid.capture(io_channel_to_upper_layer_rx_valid);
    c.io.channel.to_lower_layer.tx.credit.capture(io_channel_to_lower_layer_tx_credit);
    c.io.channel.to_lower_layer.rx.bits.capture(sb_w, io_channel_to_lower_layer_rx_bits);
    c.io.channel.to_lower_layer.rx.valid.capture(io_channel_to_lower_layer_rx_valid);

    c.io.channel.inner.node_to_layer_above.assignReady(io_channel_inner_node_to_layer_above_ready);
    c.io.channel.inner.layer_to_node_above.assignValid(io_channel_inner_layer_to_node_above_valid);
    c.io.channel.inner.layer_to_node_above.assignBits(msg_w, io_channel_inner_layer_to_node_above_bits);
    c.io.channel.inner.node_to_layer_below.assignReady(io_channel_inner_node_to_layer_below_ready);
    c.io.channel.inner.layer_to_node_below.assignValid(io_channel_inner_layer_to_node_below_valid);
    c.io.channel.inner.layer_to_node_below.assignBits(msg_w, io_channel_inner_layer_to_node_below_bits);

    // ======================== run ========================
    run(c.getClock()->getPeriod());
    run(c.getClock()->getPeriod());
}

TEST(ChannelTest, SendSomethingForYou) {
    auto top = createTopModule<ChannelWrapper>();
    auto &c = *top;

    bool io_channel_to_upper_layer_tx_credit = false;
    BigUInt io_channel_to_upper_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    bool io_channel_to_upper_layer_rx_valid = false;
    bool io_channel_to_lower_layer_tx_credit = false;
    BigUInt io_channel_to_lower_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    bool io_channel_to_lower_layer_rx_valid = false;

    bool io_channel_inner_node_to_layer_above_ready = false;
    bool io_channel_inner_layer_to_node_above_valid = false;
    BigUInt io_channel_inner_layer_to_node_above_bits = 0;
    bool io_channel_inner_node_to_layer_below_ready = false;
    bool io_channel_inner_layer_to_node_below_valid = false;
    BigUInt io_channel_inner_layer_to_node_below_bits = 0;

    // ======================== connect ========================
    c.io.channel.to_upper_layer.tx.credit.capture(io_channel_to_upper_layer_tx_credit);
    c.io.channel.to_upper_layer.rx.bits.capture(sb_w, io_channel_to_upper_layer_rx_bits);
    c.io.channel.to_upper_layer.rx.valid.capture(io_channel_to_upper_layer_rx_valid);
    c.io.channel.to_lower_layer.tx.credit.capture(io_channel_to_lower_layer_tx_credit);
    c.io.channel.to_lower_layer.rx.bits.capture(sb_w, io_channel_to_lower_layer_rx_bits);
    c.io.channel.to_lower_layer.rx.valid.capture(io_channel_to_lower_layer_rx_valid);

    c.io.channel.inner.node_to_layer_above.assignReady(io_channel_inner_node_to_layer_above_ready);
    c.io.channel.inner.layer_to_node_above.assignValid(io_channel_inner_layer_to_node_above_valid);
    c.io.channel.inner.layer_to_node_above.assignBits(msg_w, io_channel_inner_layer_to_node_above_bits);
    c.io.channel.inner.node_to_layer_below.assignReady(io_channel_inner_node_to_layer_below_ready);
    c.io.channel.inner.layer_to_node_below.assignValid(io_channel_inner_layer_to_node_below_valid);
    c.io.channel.inner.layer_to_node_below.assignBits(msg_w, io_channel_inner_layer_to_node_below_bits);

    // ======================== run ========================
    // init
    io_channel_to_upper_layer_rx_valid = false;
    io_channel_to_upper_layer_tx_credit = false;

    io_channel_to_lower_layer_rx_valid = false;
    io_channel_to_lower_layer_tx_credit = false;

    io_channel_inner_node_to_layer_above_ready = false;

    run(c.getClock()->getPeriod());

    // send something for you
    io_channel_to_upper_layer_rx_valid = true;
    BigUInt packet = c.io.dummy_foryou().toBigUInt();

    // send this in MSG_Width/NC_width cycles
    for (int i = 0; i < msg_w / sb_w; i++) {
        io_channel_to_upper_layer_rx_bits = (packet >> (i * sb_w)) & ((BigUInt(1) << sb_w) - 1);
        std::cout << "io_channel_to_upper_layer_rx_bits " << std::hex << io_channel_to_upper_layer_rx_bits << std::endl;
        run(c.getClock()->getPeriod());
    }

    // wait for inner.node_to_layer_above.valid to be true
    std::cout << "entering loop" << std::endl;
    while(!c.io.channel.inner.node_to_layer_above.isValid()) {
        std::cout << "inside loop, step " << std::endl;
        run(c.getClock()->getPeriod());
    }

    // check that the packet has arrived
    EXPECT_EQ_BigUInt(c.io.channel.inner.node_to_layer_above.bits(), packet);
    EXPECT_EQ_BOOL(c.io.channel.inner.node_to_layer_above.isValid(), true);

    // assert the ready signal
    io_channel_inner_node_to_layer_above_ready = true;

    // wait for a credit return
    while (!c.io.channel.to_upper_layer.rx.credit()) {
        run(c.getClock()->getPeriod());
    }
}

TEST(ChannelTest, SendSomethingNotForYou) {
    auto top = createTopModule<ChannelWrapper>();
    auto &c = *top;

    bool io_channel_to_upper_layer_tx_credit = false;
    BigUInt io_channel_to_upper_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    bool io_channel_to_upper_layer_rx_valid = false;
    bool io_channel_to_lower_layer_tx_credit = false;
    BigUInt io_channel_to_lower_layer_rx_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    bool io_channel_to_lower_layer_rx_valid = false;

    bool io_channel_inner_node_to_layer_above_ready = false;
    bool io_channel_inner_layer_to_node_above_valid = false;
    BigUInt io_channel_inner_layer_to_node_above_bits = 0;
    bool io_channel_inner_node_to_layer_below_ready = false;
    bool io_channel_inner_layer_to_node_below_valid = false;
    BigUInt io_channel_inner_layer_to_node_below_bits = 0;

    // ======================== connect ========================
    c.io.channel.to_upper_layer.tx.credit.capture(io_channel_to_upper_layer_tx_credit);
    c.io.channel.to_upper_layer.rx.bits.capture(sb_w, io_channel_to_upper_layer_rx_bits);
    c.io.channel.to_upper_layer.rx.valid.capture(io_channel_to_upper_layer_rx_valid);
    c.io.channel.to_lower_layer.tx.credit.capture(io_channel_to_lower_layer_tx_credit);
    c.io.channel.to_lower_layer.rx.bits.capture(sb_w, io_channel_to_lower_layer_rx_bits);
    c.io.channel.to_lower_layer.rx.valid.capture(io_channel_to_lower_layer_rx_valid);

    c.io.channel.inner.node_to_layer_above.assignReady(io_channel_inner_node_to_layer_above_ready);
    c.io.channel.inner.layer_to_node_above.assignValid(io_channel_inner_layer_to_node_above_valid);
    c.io.channel.inner.layer_to_node_above.assignBits(msg_w, io_channel_inner_layer_to_node_above_bits);
    c.io.channel.inner.node_to_layer_below.assignReady(io_channel_inner_node_to_layer_below_ready);
    c.io.channel.inner.layer_to_node_below.assignValid(io_channel_inner_layer_to_node_below_valid);
    c.io.channel.inner.layer_to_node_below.assignBits(msg_w, io_channel_inner_layer_to_node_below_bits);

    // ======================== run ========================
    // init
    io_channel_to_upper_layer_rx_valid = false;
    io_channel_to_upper_layer_tx_credit = false;

    io_channel_to_lower_layer_rx_valid = false;
    io_channel_to_lower_layer_tx_credit = false;

    io_channel_inner_node_to_layer_above_ready = false;

    run(c.getClock()->getPeriod());

    // send something for you
    io_channel_to_upper_layer_rx_valid = true;
    BigUInt packet = c.io.dummy_notforyou().toBigUInt();

    // send this in MSG_Width/NC_width cycles
    for (int i = 0; i < msg_w / sb_w; i++) {
        io_channel_to_upper_layer_rx_bits = (packet >> (i * sb_w)) & ((BigUInt(1) << sb_w) - 1);
        std::cout << "io_channel_to_upper_layer_rx_bits " << std::hex << io_channel_to_upper_layer_rx_bits << std::endl;
        run(c.getClock()->getPeriod());
    }

    // wait for the otherside tx valid to be high
    std::cout << "entering loop" << std::endl;
    while(!c.io.channel.to_lower_layer.tx.valid()) {
        std::cout << "inside loop, step " << std::endl;
        run(c.getClock()->getPeriod());
    }

    // check that the packet has arrived, MSG_Width/NC_width cycles
    for (int i = 0; i < msg_w / sb_w; i++) {
        EXPECT_EQ_BigUInt(c.io.channel.to_lower_layer.tx.bits(), (packet >> (i * sb_w)) & ((BigUInt(1) << sb_w) - 1));
        run(c.getClock()->getPeriod());
    }

    // check the credit
    EXPECT_EQ_BOOL(c.io.channel.to_upper_layer.rx.credit(), true);
}

class Channelmsgfactory: public WireModule {
public:
    struct {
        Wire<UInt> dtop;
        Wire<UInt> ptod;
        Wire<UInt> dtop_cmp;
    } io;

    Channelmsgfactory() {
        _io_dtop = SBMessage_factory(
            SBM().LINK_MGMT_ADAPTER0_REQ_DISABLE,
            "D2D",
            false,
            "PHY"
        ).toBigUInt();
        _io_ptod = SBMessage_factory(
            SBM().MBINIT_REVERSALMB_CLEAR_ERROR_REQ,
            "PHY",
            false,
            "D2D"
        ).toBigUInt();
        _io_dtop_cmp = SBMessage_factory(
            SBM().COMP_0,
            "D2D",
            false,
            "PHY"
        ).toBigUInt();

        io.dtop.capture(128, _io_dtop);
        io.ptod.capture(128, _io_ptod);
        io.dtop_cmp.capture(128, _io_dtop_cmp);
    }

private:
    BigUInt _io_dtop;
    BigUInt _io_ptod;
    BigUInt _io_dtop_cmp;

};

class ChannelPairWrapper: public WireModule {
public:
    struct {
        SidebandSwitcherbundle to_d2d{true};
        SidebandSwitcherbundle to_phy{true};
        SidebandNodeOuterIO to_upper;
        SidebandLinkNodeOuterIO to_lower;
        Wire<UInt> dtop;
        Wire<UInt> ptod;
        Wire<UInt> dtop_cmp;
    } io;

    SidebandParams sb_params;
    FdiParams fdi_params;

    ChannelPairWrapper() {
        sb_params.max_crd = 1;
        fdi_params.width = 8;
        fdi_params.dllp_width = 8;
        fdi_params.sb_width = 32;
        int msg_w = sb_params.sb_node_msg_width;

        // Instantiate
        _dietodie_channel = createSubmodule<D2DSidebandChannel>("dietodie_channel", sb_params, fdi_params, 1);
        _phy_channel = createSubmodule<PHYSidebandChannel>("phy_channel", sb_params, fdi_params, 2);
        _d = createSubmodule<Channelmsgfactory>("d");

        // connect
        io.to_d2d.connect(_dietodie_channel->io.inner);
        io.to_phy.connect(_phy_channel->io.inner.switcher_bundle);
        _phy_channel->io.inner.raw_input.assignValid(false);
        _phy_channel->io.inner.raw_input.assignBits(msg_w, std::move(BigUInt(0)));
        _phy_channel->io.inner.rx_mode.capture(1, RXTXMode::PACKET);
        _phy_channel->io.inner.input_mode.capture(1, RXTXMode::PACKET);

        io.to_upper.connect(_dietodie_channel->io.to_upper_layer);
        io.to_lower.connect(_phy_channel->io.to_lower_layer);
        // connect clock in propagateClock function.

        _phy_channel->io.to_upper_layer.rx.connect(_dietodie_channel->io.to_lower_layer.tx);
        _dietodie_channel->io.to_lower_layer.rx.connect(_phy_channel->io.to_upper_layer.tx);

        io.dtop = _d->io.dtop;
        io.ptod = _d->io.ptod;
        io.dtop_cmp = _d->io.dtop_cmp;
    }

    bool propagateClock() override {
        if (io.to_lower.rx.clock == nullptr) {
            _remote_clock_enable_func = [this]() -> bool {
                return false;
            };
            io.to_lower.rx.clock = createGatedClock(getClock(), getPathName() + "_gated_clock", _remote_clock_enable_func);
            _phy_channel->io.to_lower_layer.rx.clock = io.to_lower.rx.clock;
        }

        return Module::propagateClock();
    }

private:
    ModulePtr<D2DSidebandChannel> _dietodie_channel;
    ModulePtr<PHYSidebandChannel> _phy_channel;
    ModulePtr<Channelmsgfactory> _d;

    GatedClock::ENABLE_FUNC _remote_clock_enable_func;

};

TEST(ChannelTest, SendChannelPair) {
    auto top = createTopModule<ChannelPairWrapper>();
    auto &c = *top;
    int msg_w = c.sb_params.sb_node_msg_width;
    int sb_w = c.fdi_params.sb_width;

    bool io_to_d2d_node_to_layer_above_ready = false;
    bool io_to_d2d_layer_to_node_above_valid = false;
    BigUInt io_to_d2d_layer_to_node_above_bits = 0;
    bool io_to_d2d_node_to_layer_below_ready = false;
    bool io_to_d2d_layer_to_node_below_valid = false;
    BigUInt io_to_d2d_layer_to_node_below_bits = 0;

    bool io_to_phy_node_to_layer_above_ready = false;
    bool io_to_phy_layer_to_node_above_valid = false;
    BigUInt io_to_phy_layer_to_node_above_bits = 0;
    bool io_to_phy_node_to_layer_below_ready = false;
    bool io_to_phy_layer_to_node_below_valid = false;
    BigUInt io_to_phy_layer_to_node_below_bits = 0;

    bool io_to_upper_tx_credit = false;
    BigUInt io_to_upper_rx_bits = 0;
    bool io_to_upper_rx_valid = false;

    BigUInt io_to_lower_rx_bits = 0;

    // ======================== connect ========================
    c.io.to_d2d.node_to_layer_above.assignReady(io_to_d2d_node_to_layer_above_ready);
    c.io.to_d2d.layer_to_node_above.assignValid(io_to_d2d_layer_to_node_above_valid);
    c.io.to_d2d.layer_to_node_above.assignBits(msg_w, io_to_d2d_layer_to_node_above_bits);
    c.io.to_d2d.node_to_layer_below.assignReady(io_to_d2d_node_to_layer_below_ready);
    c.io.to_d2d.layer_to_node_below.assignValid(io_to_d2d_layer_to_node_below_valid);
    c.io.to_d2d.layer_to_node_below.assignBits(msg_w, io_to_d2d_layer_to_node_below_bits);

    c.io.to_phy.node_to_layer_above.assignReady(io_to_phy_node_to_layer_above_ready);
    c.io.to_phy.layer_to_node_above.assignValid(io_to_phy_layer_to_node_above_valid);
    c.io.to_phy.layer_to_node_above.assignBits(msg_w, io_to_phy_layer_to_node_above_bits);
    c.io.to_phy.node_to_layer_below.assignReady(io_to_phy_node_to_layer_below_ready);
    c.io.to_phy.layer_to_node_below.assignValid(io_to_phy_layer_to_node_below_valid);
    c.io.to_phy.layer_to_node_below.assignBits(msg_w, io_to_phy_layer_to_node_below_bits);

    c.io.to_upper.tx.credit.capture(io_to_upper_tx_credit);
    c.io.to_upper.rx.bits.capture(sb_w, io_to_upper_rx_bits);
    c.io.to_upper.rx.valid.capture(io_to_upper_rx_valid);

    c.io.to_lower.rx.bits.capture(1, io_to_lower_rx_bits);

    // ======================== run ========================
    // init
    run(c.getClock()->getPeriod());
    io_to_d2d_layer_to_node_below_valid = true;
    BigUInt dtop_packet = c.io.dtop().toBigUInt();
    io_to_d2d_layer_to_node_below_bits = dtop_packet;

    io_to_phy_layer_to_node_above_valid = true;
    BigUInt ptod_packet = c.io.ptod().toBigUInt();
    io_to_phy_layer_to_node_above_bits = ptod_packet;

    run(c.getClock()->getPeriod());
    io_to_d2d_layer_to_node_below_valid = false;
    io_to_phy_layer_to_node_above_valid = false;

    // wait for the packet to arrive
    while (!c.io.to_phy.node_to_layer_above.isValid()) {
        run(c.getClock()->getPeriod());
    }

    EXPECT_EQ_BigUInt(c.io.to_phy.node_to_layer_above.bits(), dtop_packet);

    // wait for the other packet to arrive
    while(!c.io.to_d2d.node_to_layer_below.isValid()) {
        run(c.getClock()->getPeriod());
    }

    EXPECT_EQ_BigUInt(c.io.to_d2d.node_to_layer_below.bits(), ptod_packet);

    // verify credit is zero
    EXPECT_EQ_BOOL(c.io.to_d2d.layer_to_node_below.isReady(), false);
    EXPECT_EQ_BOOL(c.io.to_phy.layer_to_node_above.isReady(), false);

    // send complete over
    io_to_d2d_layer_to_node_below_valid = true;
    BigUInt dtop_complete = c.io.dtop_cmp().toBigUInt();
    io_to_d2d_layer_to_node_below_bits = dtop_complete;

    EXPECT_EQ_BOOL(c.io.to_d2d.layer_to_node_below.isReady(), true);

    run(c.getClock()->getPeriod());

    //// change the input to something that is not complete, and shut valid
    //io_to_d2d_layer_to_node_below_valid = false;
    //BigUInt dtop_complete = c.io.dtop_cmp().toBigUInt();
    //io_to_d2d_layer_to_node_below_bits = dtop_complete;

    //EXPECT_EQ_BOOL(c.io.to_d2d.layer_to_node_below.isReady(), true);

    //run(c.getClock()->getPeriod());

    // change the input to something that is not complete, and shut valid
    io_to_d2d_layer_to_node_below_valid = false;
    io_to_d2d_layer_to_node_below_bits = dtop_packet;

    // wait for arbitrary time: 20 cycles
    for (int i = 0; i < 20; i++) {
        run(c.getClock()->getPeriod());
    }

    // verify the other side now sees the complete packet
    EXPECT_EQ_BOOL(c.io.to_phy.node_to_layer_above.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.to_phy.node_to_layer_above.bits(), dtop_complete);

    // dequeue the complete packet
    io_to_phy_node_to_layer_above_ready = true;

    run(c.getClock()->getPeriod());

    io_to_phy_node_to_layer_above_ready = false;

    // see the dtop packet there
    EXPECT_EQ_BigUInt(c.io.to_phy.node_to_layer_above.bits(), dtop_packet);
    // see the ready is still false
    for (int i = 0; i < 10; i++) {
        run(c.getClock()->getPeriod());
    }

    EXPECT_EQ_BOOL(c.io.to_d2d.layer_to_node_below.isReady(), false);

    // now dequeue the other packet
    io_to_phy_node_to_layer_above_ready = true;
    for (int i = 0; i < 10; i++) {
        run(c.getClock()->getPeriod());
    }

    // see credit return
    EXPECT_EQ_BOOL(c.io.to_d2d.layer_to_node_below.isReady(), true);
}