#include "sideband/sideband_node.hpp"
#include "test_utils.hpp"
#include "utils/time_slice.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static const FdiParams fdi_params{8, 8, 32};
static const FdiParams simple_fdiParams{8, 8, 1};
static const SidebandParams sb_params;
static int msg_w = sb_params.sb_node_msg_width;

TEST(LinkSerDesTest, InstantiateSerTest) {
    auto top = createTopModule<SidebandLinkSerializer>(sb_params, fdi_params);
    auto &c = *top;

    bool io_in_valid = false;
    BigUInt io_in_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;


    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_in_valid_func = [&io_in_valid]() -> Bool {
        return Bool(io_in_valid);
    };
    c.io.in.assignValid(io_in_valid_func);

    Wire<UInt>::TPFUNC io_in_bits_func = [&io_in_bits]() -> UInt {
        return UInt(msg_w, io_in_bits);
    };
    c.io.in.assignBits(io_in_bits_func);

    // ======================== run ========================
    // init
    io_in_valid = false;
    c.step();
}

TEST(LinkSerDesTest, SimpleSer) {
    bool io_in_valid = false;
    bool io_out_credit = false;
    BigUInt io_in_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;
    int msg_w = sb_params.sb_node_msg_width;  // 128
    int sb_w = simple_fdiParams.sb_width;           // 32

    auto top = createTopModule<SidebandSerializer>(sb_params, simple_fdiParams);
    auto &c = *top;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_in_valid_func = [&io_in_valid] () -> Bool {
        return Bool(io_in_valid);
    };
    c.io.in.assignValid(io_in_valid_func);

    Wire<Bool>::TPFUNC io_out_credit_func = [&io_out_credit]() -> Bool {
        return Bool(io_out_credit);
    };
    c.io.out.credit = io_out_credit_func;

    Wire<UInt>::TPFUNC io_in_bits_func = [&msg_w, &io_in_bits]() -> UInt {
        return UInt(msg_w, io_in_bits);
    };
    c.io.in.assignBits(io_in_bits_func);

    // ======================== run ========================
    std::cout << "Test started" << std::endl;

    // init
    io_in_valid = false;
    io_out_credit = false;
    c.step();

    //Send data to serializer
    std::cout << ("Send data") << std::endl;
    io_in_valid = true;
    EXPECT_EQ_BOOL(c.io.out.valid(), false);
    c.step();

    //Check serialized data
    io_in_valid = false;
    for (int i = 0; i < msg_w / sb_w; i++) {
        BigUInt serialized_data = (io_in_bits >> i * sb_w) & ((BigUInt(1) << sb_w) - 1);
        EXPECT_EQ_BOOL(c.io.in.isReady(), false) << "c.io.in.ready mismatch, i " << i;
        EXPECT_EQ_BOOL(c.io.out.valid(), true) << "c.io.in.valid mismatch, i " << i;
        EXPECT_EQ_BigUInt(c.io.out.bits(), serialized_data) << "c.io.in.bits mismatch, i " << i;
        c.step();
    }

    //make sure nothing is there
    EXPECT_EQ_BOOL(c.io.in.isReady(), true);
    EXPECT_EQ_BOOL(c.io.out.valid(), false);
}

TEST(LinkSerDesTest, SimpleLinkSer) {
    auto top = createTopModule<SidebandLinkSerializer>(sb_params, simple_fdiParams);
    auto &c = *top;

    int sb_w = simple_fdiParams.sb_width;

    bool io_in_valid = false;
    BigUInt io_in_bits = BigUInt(0x1234567823456789) << 64 | 0xa5a5a5a535353535;


    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_in_valid_func = [&io_in_valid]() -> Bool {
        return Bool(io_in_valid);
    };
    c.io.in.assignValid(io_in_valid_func);

    Wire<UInt>::TPFUNC io_in_bits_func = [&io_in_bits]() -> UInt {
        return UInt(msg_w, io_in_bits);
    };
    c.io.in.assignBits(io_in_bits_func);

    // ======================== run ========================
    // init
    io_in_valid = false;
    c.step();

    // Send data to serializer
    io_in_valid = true;
    c.step();

    // Check serialized data
    io_in_valid = false;
    for (int i = 0; i < msg_w / sb_w; i++) {
        BigUInt serialized_data = (io_in_bits >> i * sb_w) & ((BigUInt(1) << sb_w) - 1);
        EXPECT_EQ_BOOL(c.io.in.isReady(), false) << "i " << i;
        EXPECT_EQ_BigUInt(c.io.out.bits(), serialized_data) << "i " << i;
        c.step();
    }

    // make sure it does not take anything for 32 cycles
    for (int i = 0; i < 32; i++) {
        EXPECT_EQ_BOOL(c.io.in.isReady(), false) << "i " << i;
        c.step();
    }

    // make sure it takes something
    EXPECT_EQ_BOOL(c.io.in.isReady(), true);
}


class LinkNodePair: public WireModule {
public:
    struct {
        struct {
            Decoupled<UInt> layer_to_node{true};
            Decoupled<UInt> node_to_layer;
        } A;
        struct {
            Decoupled<UInt> layer_to_node{true};
            Decoupled<UInt> node_to_layer;
        } B;
    } io;

    ModulePtr<SidebandLinkNode> nodeA;
    ModulePtr<SidebandLinkNode> nodeB;

    LinkNodePair(
        const SidebandParams& sb_params,
        const FdiParams& fdi_params)
    {

        nodeA = createSubmodule<SidebandLinkNode>("nodeA", sb_params, fdi_params);
        nodeB = createSubmodule<SidebandLinkNode>("nodeB", sb_params, fdi_params);

        _io_rx_mode = []() -> UInt {
            return UInt(2, RXTXMode::PACKET);
        };

        io.A.node_to_layer.connect(nodeA->io.inner.node_to_layer);
        nodeA->io.inner.layer_to_node.connect(io.A.layer_to_node);
        nodeA->io.rx_mode = _io_rx_mode;
        io.B.node_to_layer.connect(nodeB->io.inner.node_to_layer);
        nodeB->io.inner.layer_to_node.connect(io.B.layer_to_node);
        nodeA->io.outer.rx.bits = nodeB->io.outer.tx.bits;
        nodeB->io.outer.rx.bits = nodeA->io.outer.tx.bits;
        nodeB->io.rx_mode = _io_rx_mode;
    }

    bool propagateClock() override {
        bool submodule_success = Module::propagateClock();
        nodeA->io.outer.rx.clock = nodeB->io.outer.tx.clock;
        nodeB->io.outer.rx.clock = nodeA->io.outer.tx.clock;
        submodule_success &= nodeA->io.outer.rx.clock != nullptr;
        submodule_success &= nodeB->io.outer.rx.clock != nullptr;
        return submodule_success;
    }

    void calcNextState() {
        //std::cout << "LinkNodePair: " << " step" << std::endl;
    }

private:
    Wire<UInt> _io_rx_mode;
};

TEST(LinkSerDesTest, SimpleLinkNodePair) {
    auto top = createTopModule<LinkNodePair>(sb_params, simple_fdiParams);
    auto &c = *top;
    //TimeSlice::getInstance().showClockTree();

    int sb_w = simple_fdiParams.sb_width;

    bool io_A_layer_to_node_valid = false;
    BigUInt io_A_layer_to_node_bits = false;
    bool io_A_node_to_layer_ready = false;
    bool io_B_layer_to_node_valid = false;
    BigUInt io_B_layer_to_node_bits = false;
    bool io_B_node_to_layer_ready = false;

    // ======================== connect ========================
    Wire<Bool>::TPFUNC io_A_layer_to_node_valid_func = [&io_A_layer_to_node_valid]() -> Bool {
        return Bool(io_A_layer_to_node_valid);
    };
    c.io.A.layer_to_node.assignValid(io_A_layer_to_node_valid_func);

    Wire<UInt>::TPFUNC io_A_layer_to_node_bits_func = [&io_A_layer_to_node_bits]() -> UInt {
        return UInt(msg_w, io_A_layer_to_node_bits);
    };
    c.io.A.layer_to_node.assignBits(io_A_layer_to_node_bits_func);

    Wire<Bool>::TPFUNC io_A_node_to_layer_ready_func = [&io_A_node_to_layer_ready]() -> Bool {
        return Bool(io_A_node_to_layer_ready);
    };
    c.io.A.node_to_layer.assignReady(io_A_node_to_layer_ready_func);

    Wire<Bool>::TPFUNC io_B_layer_to_node_valid_func = [&io_B_layer_to_node_valid]() -> Bool {
        return Bool(io_B_layer_to_node_valid);
    };
    c.io.B.layer_to_node.assignValid(io_B_layer_to_node_valid_func);

    Wire<UInt>::TPFUNC io_B_layer_to_node_bits_func = [&io_B_layer_to_node_bits]() -> UInt {
        return UInt(msg_w, io_B_layer_to_node_bits);
    };
    c.io.B.layer_to_node.assignBits(io_B_layer_to_node_bits_func);

    Wire<Bool>::TPFUNC io_B_node_to_layer_ready_func = [&io_B_node_to_layer_ready]() -> Bool {
        return Bool(io_B_node_to_layer_ready);
    };
    c.io.B.node_to_layer.assignReady(io_B_node_to_layer_ready_func);

    // ======================== run ========================
    // init
    io_A_layer_to_node_valid = false;
    io_A_node_to_layer_ready = false;
    io_B_layer_to_node_valid = false;
    io_B_node_to_layer_ready = false;
    run(top->getClock()->getPeriod());

    // TODO, skip reset stage here.

    EXPECT_EQ_BOOL(c.io.B.node_to_layer.isValid(), false);

    // send something from A to B
    io_A_layer_to_node_valid = true;
    io_A_layer_to_node_bits = 3;

    // wait until B receives it
    //std::cout << "entering loop" << std::endl;
    while (true) {
        if (c.io.B.node_to_layer.isValid()) {
            break;
        }
        //std::cout << "waiting" << std::endl;
        run(top->getClock()->getPeriod());
    }

    EXPECT_EQ_BOOL(c.io.B.node_to_layer.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.B.node_to_layer.bits(), io_A_layer_to_node_bits);
    io_B_node_to_layer_ready = true;
    run(top->getClock()->getPeriod());

    // send another thing from A to B
    io_A_layer_to_node_valid = true;
    io_A_layer_to_node_bits = 2;

    // wait until B receives it
    //std::cout << "entering loop" << std::endl;
    while (true) {
        if (c.io.B.node_to_layer.isValid()) {
            break;
        }
        //std::cout << "waiting" << std::endl;
        run(top->getClock()->getPeriod());
    }

    EXPECT_EQ_BOOL(c.io.B.node_to_layer.isValid(), true);
    EXPECT_EQ_BigUInt(c.io.B.node_to_layer.bits(), io_A_layer_to_node_bits);
}
