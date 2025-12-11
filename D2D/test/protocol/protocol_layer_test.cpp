#include "protocol/hamming_code.hpp"
#include "protocol/protocol_layer.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;
using namespace CCPS;

class HammingWrapper: public WireModule {
public:
    struct {
        Wire<UInt> tx_data;  // I protoParams.ccpsNonEccWidth
        Wire<UInt> rx_data;  // I protoParams.ccpsNonEccWidth
        Wire<UInt> checksum;  // I protoParams.ccpsEccWidth
        Wire<Bool> matches;  // O
    } io;

    HammingWrapper(const ProtocolLayerParams &proto_params) {
        // Instantiate
        _encoder = createSubmodule<HammingEncode>("encoder", proto_params);
        _decoder = createSubmodule<HammingDecode>("decoder", proto_params);

        // Connect
        _encoder->io.data = io.tx_data;
        _decoder->io.data = io.rx_data;
        _decoder->io.checksum = [this]() -> UInt {
            return static_cast<UInt>(_encoder->io.checksum() & io.checksum());
        };
        io.matches = _decoder->io.matches;
    }

private:
    ModulePtr<HammingEncode> _encoder;
    ModulePtr<HammingDecode> _decoder;
};

TEST(ProtocolLayerTest, TestHammingEncodeAndDecode) {
    const ProtocolLayerParams proto_params;
    auto top = createTopModule<HammingWrapper>(proto_params);
    auto &c = *top;

    // IOs
    BigUInt io_tx_data = 0;
    BigUInt io_rx_data = 0;
    BigUInt io_checksum = 0;

    // connect
    c.io.tx_data.capture(448, io_tx_data);
    c.io.rx_data.capture(448, io_rx_data);
    c.io.checksum.capture(64, io_checksum);

    // run
    std::cout << "Instantiating pattern buffer" << std::endl;
    BigUInt tuvok = BigUInt("0xBADDCAFE");
    BigUInt neelix = BigUInt("0xDEADBEEF");

    std::cout << "Test matching transport" << std::endl;
    io_tx_data = tuvok;
    io_rx_data = tuvok;
    io_checksum = BigUInt("0xFFFFFFFF");
    c.step();

    io_tx_data = neelix;
    io_rx_data = neelix;
    io_checksum = BigUInt("0xFFFFFFFF");
    EXPECT_EQ_BOOL(c.io.matches(), true);
    c.step();

    std::cout << "Test checksum error" << std::endl;
    io_tx_data = tuvok;
    io_rx_data = tuvok;
    io_checksum = BigUInt("0x1234ABCD");
    EXPECT_EQ_BOOL(c.io.matches(), false);
    c.step();

    io_tx_data = neelix;
    io_rx_data = neelix;
    io_checksum = BigUInt("0x1234ABCD");
    EXPECT_EQ_BOOL(c.io.matches(), false);
    c.step();

    std::cout << "Test mismatch transport" << std::endl;
    io_tx_data = tuvok;
    io_rx_data = neelix;
    io_checksum = BigUInt("0xFFFFFFFF");
    EXPECT_EQ_BOOL(c.io.matches(), false);
    c.step();

    io_tx_data = neelix;
    io_rx_data = tuvok;
    io_checksum = BigUInt("0xFFFFFFFF");
    EXPECT_EQ_BOOL(c.io.matches(), false);
    c.step();
}

static const FdiParams fdi_params{8, 8, 32};

TEST(ProtocolLayerTest, TestProtocolLayerFdiPlRxActiveReqAndLpRxActiveSts) {
    auto top = createTopModule<ProtocolLayer>(fdi_params);
    auto &c = *top;

    // IOs
    bool io_fdi_lp_data_ready = false;
    bool io_fdi_pl_retimer_crd = false;
    bool io_fdi_pl_dllp_valid = false;
    BigUInt io_fdi_pl_dllp_bits = 0;
    bool io_fdi_pl_dllp_ofc = false;
    BigUInt io_fdi_pl_stream_proto_stack = 0;
    BigUInt io_fdi_pl_stream_proto_type = 0;
    bool io_fdi_pl_flit_cancel = false;
    BigUInt io_fdi_pl_state_status = 0; // 4
    bool io_fdi_pl_inband_pres = false;
    bool io_fdi_pl_error = false;
    bool io_fdi_pl_cerror = false;
    bool io_fdi_pl_nf_error = false;
    bool io_fdi_pl_train_error = false;
    bool io_fdi_pl_rx_active_req = false;
    BigUInt io_fdi_pl_protocol = 0; // 3
    BigUInt io_fdi_pl_protocol_flit_format = 0; // 4
    bool io_fdi_pl_protocol_valid = false;
    bool io_fdi_pl_stall_req = false;
    bool io_fdi_pl_phy_in_recenter = false;
    bool io_fdi_pl_phy_in_l1 = false;
    bool io_fdi_pl_phy_in_l2 = false;
    BigUInt io_fdi_pl_speed_mode = 0; // 3
    BigUInt io_fdi_pl_link_width = 0; // 3
    bool io_fdi_pl_clk_req = false;
    bool io_fdi_pl_wake_ack = false;
    bool io_fdi_pl_config_valid = false;
    BigUInt io_fdi_pl_config_bits = 0; // params.sbWidth
    bool io_fdi_lp_config_credit = false;

    bool io_tl_lp_data_valid = false;
    BigUInt io_tl_lp_data_bits = 0; // 8 * fdiParams.width
    bool io_tl_lp_data_irdy = false;
    bool io_tl_ready_to_rcv = false;
    bool io_fault = false;
    bool io_soft_reset = false;

    // connect
    c.io.fdi.lp_data.assignReady(io_fdi_lp_data_ready);
    c.io.fdi.pl_retimer_crd.capture(io_fdi_pl_retimer_crd);
    c.io.fdi.pl_dllp.assignValid(io_fdi_pl_dllp_valid);
    c.io.fdi.pl_dllp.assignBits(fdi_params.dllp_width, io_fdi_pl_dllp_bits);
    c.io.fdi.pl_dllp_ofc.capture(io_fdi_pl_dllp_ofc);
    c.io.fdi.pl_stream.proto_stack.capture(4, io_fdi_pl_stream_proto_stack);
    c.io.fdi.pl_stream.proto_type.capture(4, io_fdi_pl_stream_proto_type);
    c.io.fdi.pl_flit_cancel.capture(io_fdi_pl_flit_cancel);
    c.io.fdi.pl_state_status.capture(4, io_fdi_pl_state_status);
    c.io.fdi.pl_inband_pres.capture(io_fdi_pl_inband_pres);
    c.io.fdi.pl_error.capture(io_fdi_pl_error);
    c.io.fdi.pl_cerror.capture(io_fdi_pl_cerror);
    c.io.fdi.pl_nf_error.capture(io_fdi_pl_nf_error);
    c.io.fdi.pl_train_error.capture(io_fdi_pl_train_error);
    c.io.fdi.pl_rx_active_req.capture(io_fdi_pl_rx_active_req);
    c.io.fdi.pl_protocol.capture(3, io_fdi_pl_protocol);
    c.io.fdi.pl_protocol_flit_format.capture(4, io_fdi_pl_protocol_flit_format);
    c.io.fdi.pl_protocol_valid.capture(io_fdi_pl_protocol_valid);
    c.io.fdi.pl_stall_req.capture(io_fdi_pl_stall_req);
    c.io.fdi.pl_phy_in_recenter.capture(io_fdi_pl_phy_in_recenter);
    c.io.fdi.pl_phy_in_l1.capture(io_fdi_pl_phy_in_l1);
    c.io.fdi.pl_phy_in_l2.capture(io_fdi_pl_phy_in_l2);
    c.io.fdi.pl_speed_mode.capture(3, io_fdi_pl_speed_mode);
    c.io.fdi.pl_link_width.capture(3, io_fdi_pl_link_width);
    c.io.fdi.pl_clk_req.capture(io_fdi_pl_clk_req);
    c.io.fdi.pl_wake_ack.capture(io_fdi_pl_wake_ack);
    c.io.fdi.pl_config.assignValid(io_fdi_pl_config_valid);
    c.io.fdi.pl_config.assignBits(fdi_params.sb_width, io_fdi_pl_config_bits);
    c.io.fdi.lp_config_credit.capture(io_fdi_lp_config_credit);

    c.io.tl_lp_data_valid.capture(io_tl_lp_data_valid);
    c.io.tl_lp_data_bits.capture(8 * fdi_params.width, io_tl_lp_data_bits);
    c.io.tl_lp_data_irdy.capture(io_tl_lp_data_irdy);
    c.io.tl_ready_to_rcv.capture(io_tl_ready_to_rcv);
    c.io.fault.capture(io_fault);
    c.io.soft_reset.capture(io_soft_reset);

    // runs
    io_tl_ready_to_rcv = true;
    io_fdi_pl_rx_active_req = true;
    io_fdi_pl_state_status = PhyState::active;
    c.step();

    io_tl_ready_to_rcv = false;
    io_fdi_pl_rx_active_req = true;
    io_fdi_pl_state_status = PhyState::active;
    c.step();

    io_tl_ready_to_rcv = true;
    io_fdi_pl_rx_active_req = false;
    io_fdi_pl_state_status = PhyState::active;
    c.step();

    io_tl_ready_to_rcv = true;
    io_fdi_pl_rx_active_req = true;
    io_fdi_pl_state_status = PhyState::reset;
    c.step();

    io_tl_ready_to_rcv = true;
    io_fdi_pl_rx_active_req = true;
    io_fdi_pl_state_status = PhyState::linkReset;
    c.step();
}

TEST(ProtocolLayerTest, TestProtocolLayerStallReqAck) {
    auto top = createTopModule<ProtocolLayer>(fdi_params);
    auto &c = *top;

    // IOs
    bool io_fdi_lp_data_ready = false;
    bool io_fdi_pl_retimer_crd = false;
    bool io_fdi_pl_dllp_valid = false;
    BigUInt io_fdi_pl_dllp_bits = 0;
    bool io_fdi_pl_dllp_ofc = false;
    BigUInt io_fdi_pl_stream_proto_stack = 0;
    BigUInt io_fdi_pl_stream_proto_type = 0;
    bool io_fdi_pl_flit_cancel = false;
    BigUInt io_fdi_pl_state_status = 0; // 4
    bool io_fdi_pl_inband_pres = false;
    bool io_fdi_pl_error = false;
    bool io_fdi_pl_cerror = false;
    bool io_fdi_pl_nf_error = false;
    bool io_fdi_pl_train_error = false;
    bool io_fdi_pl_rx_active_req = false;
    BigUInt io_fdi_pl_protocol = 0; // 3
    BigUInt io_fdi_pl_protocol_flit_format = 0; // 4
    bool io_fdi_pl_protocol_valid = false;
    bool io_fdi_pl_stall_req = false;
    bool io_fdi_pl_phy_in_recenter = false;
    bool io_fdi_pl_phy_in_l1 = false;
    bool io_fdi_pl_phy_in_l2 = false;
    BigUInt io_fdi_pl_speed_mode = 0; // 3
    BigUInt io_fdi_pl_link_width = 0; // 3
    bool io_fdi_pl_clk_req = false;
    bool io_fdi_pl_wake_ack = false;
    bool io_fdi_pl_config_valid = false;
    BigUInt io_fdi_pl_config_bits = 0; // params.sbWidth
    bool io_fdi_lp_config_credit = false;

    bool io_tl_lp_data_valid = false;
    BigUInt io_tl_lp_data_bits = 0; // 8 * fdiParams.width
    bool io_tl_lp_data_irdy = false;
    bool io_tl_ready_to_rcv = false;
    bool io_fault = false;
    bool io_soft_reset = false;

    // connect
    c.io.fdi.lp_data.assignReady(io_fdi_lp_data_ready);
    c.io.fdi.pl_retimer_crd.capture(io_fdi_pl_retimer_crd);
    c.io.fdi.pl_dllp.assignValid(io_fdi_pl_dllp_valid);
    c.io.fdi.pl_dllp.assignBits(fdi_params.dllp_width, io_fdi_pl_dllp_bits);
    c.io.fdi.pl_dllp_ofc.capture(io_fdi_pl_dllp_ofc);
    c.io.fdi.pl_stream.proto_stack.capture(4, io_fdi_pl_stream_proto_stack);
    c.io.fdi.pl_stream.proto_type.capture(4, io_fdi_pl_stream_proto_type);
    c.io.fdi.pl_flit_cancel.capture(io_fdi_pl_flit_cancel);
    c.io.fdi.pl_state_status.capture(4, io_fdi_pl_state_status);
    c.io.fdi.pl_inband_pres.capture(io_fdi_pl_inband_pres);
    c.io.fdi.pl_error.capture(io_fdi_pl_error);
    c.io.fdi.pl_cerror.capture(io_fdi_pl_cerror);
    c.io.fdi.pl_nf_error.capture(io_fdi_pl_nf_error);
    c.io.fdi.pl_train_error.capture(io_fdi_pl_train_error);
    c.io.fdi.pl_rx_active_req.capture(io_fdi_pl_rx_active_req);
    c.io.fdi.pl_protocol.capture(3, io_fdi_pl_protocol);
    c.io.fdi.pl_protocol_flit_format.capture(4, io_fdi_pl_protocol_flit_format);
    c.io.fdi.pl_protocol_valid.capture(io_fdi_pl_protocol_valid);
    c.io.fdi.pl_stall_req.capture(io_fdi_pl_stall_req);
    c.io.fdi.pl_phy_in_recenter.capture(io_fdi_pl_phy_in_recenter);
    c.io.fdi.pl_phy_in_l1.capture(io_fdi_pl_phy_in_l1);
    c.io.fdi.pl_phy_in_l2.capture(io_fdi_pl_phy_in_l2);
    c.io.fdi.pl_speed_mode.capture(3, io_fdi_pl_speed_mode);
    c.io.fdi.pl_link_width.capture(3, io_fdi_pl_link_width);
    c.io.fdi.pl_clk_req.capture(io_fdi_pl_clk_req);
    c.io.fdi.pl_wake_ack.capture(io_fdi_pl_wake_ack);
    c.io.fdi.pl_config.assignValid(io_fdi_pl_config_valid);
    c.io.fdi.pl_config.assignBits(fdi_params.sb_width, io_fdi_pl_config_bits);
    c.io.fdi.lp_config_credit.capture(io_fdi_lp_config_credit);

    c.io.tl_lp_data_valid.capture(io_tl_lp_data_valid);
    c.io.tl_lp_data_bits.capture(8 * fdi_params.width, io_tl_lp_data_bits);
    c.io.tl_lp_data_irdy.capture(io_tl_lp_data_irdy);
    c.io.tl_ready_to_rcv.capture(io_tl_ready_to_rcv);
    c.io.fault.capture(io_fault);
    c.io.soft_reset.capture(io_soft_reset);

    // runs
    io_fdi_pl_stall_req = false;
    c.step();

    io_fdi_pl_stall_req = false;
    io_fdi_pl_stall_req = true;
    EXPECT_EQ_BOOL(c.io.fdi.lp_stall_ack(), false);
    c.step();
    EXPECT_EQ_BOOL(c.io.fdi.lp_stall_ack(), true);
}

TEST(ProtocolLayerTest, TestProtocolLayerLinkError) {
    auto top = createTopModule<ProtocolLayer>(fdi_params);
    auto &c = *top;

    // IOs
    bool io_fdi_lp_data_ready = false;
    bool io_fdi_pl_retimer_crd = false;
    bool io_fdi_pl_dllp_valid = false;
    BigUInt io_fdi_pl_dllp_bits = 0;
    bool io_fdi_pl_dllp_ofc = false;
    BigUInt io_fdi_pl_stream_proto_stack = 0;
    BigUInt io_fdi_pl_stream_proto_type = 0;
    bool io_fdi_pl_flit_cancel = false;
    BigUInt io_fdi_pl_state_status = 0; // 4
    bool io_fdi_pl_inband_pres = false;
    bool io_fdi_pl_error = false;
    bool io_fdi_pl_cerror = false;
    bool io_fdi_pl_nf_error = false;
    bool io_fdi_pl_train_error = false;
    bool io_fdi_pl_rx_active_req = false;
    BigUInt io_fdi_pl_protocol = 0; // 3
    BigUInt io_fdi_pl_protocol_flit_format = 0; // 4
    bool io_fdi_pl_protocol_valid = false;
    bool io_fdi_pl_stall_req = false;
    bool io_fdi_pl_phy_in_recenter = false;
    bool io_fdi_pl_phy_in_l1 = false;
    bool io_fdi_pl_phy_in_l2 = false;
    BigUInt io_fdi_pl_speed_mode = 0; // 3
    BigUInt io_fdi_pl_link_width = 0; // 3
    bool io_fdi_pl_clk_req = false;
    bool io_fdi_pl_wake_ack = false;
    bool io_fdi_pl_config_valid = false;
    BigUInt io_fdi_pl_config_bits = 0; // params.sbWidth
    bool io_fdi_lp_config_credit = false;

    bool io_tl_lp_data_valid = false;
    BigUInt io_tl_lp_data_bits = 0; // 8 * fdiParams.width
    bool io_tl_lp_data_irdy = false;
    bool io_tl_ready_to_rcv = false;
    bool io_fault = false;
    bool io_soft_reset = false;

    // connect
    c.io.fdi.lp_data.assignReady(io_fdi_lp_data_ready);
    c.io.fdi.pl_retimer_crd.capture(io_fdi_pl_retimer_crd);
    c.io.fdi.pl_dllp.assignValid(io_fdi_pl_dllp_valid);
    c.io.fdi.pl_dllp.assignBits(fdi_params.dllp_width, io_fdi_pl_dllp_bits);
    c.io.fdi.pl_dllp_ofc.capture(io_fdi_pl_dllp_ofc);
    c.io.fdi.pl_stream.proto_stack.capture(4, io_fdi_pl_stream_proto_stack);
    c.io.fdi.pl_stream.proto_type.capture(4, io_fdi_pl_stream_proto_type);
    c.io.fdi.pl_flit_cancel.capture(io_fdi_pl_flit_cancel);
    c.io.fdi.pl_state_status.capture(4, io_fdi_pl_state_status);
    c.io.fdi.pl_inband_pres.capture(io_fdi_pl_inband_pres);
    c.io.fdi.pl_error.capture(io_fdi_pl_error);
    c.io.fdi.pl_cerror.capture(io_fdi_pl_cerror);
    c.io.fdi.pl_nf_error.capture(io_fdi_pl_nf_error);
    c.io.fdi.pl_train_error.capture(io_fdi_pl_train_error);
    c.io.fdi.pl_rx_active_req.capture(io_fdi_pl_rx_active_req);
    c.io.fdi.pl_protocol.capture(3, io_fdi_pl_protocol);
    c.io.fdi.pl_protocol_flit_format.capture(4, io_fdi_pl_protocol_flit_format);
    c.io.fdi.pl_protocol_valid.capture(io_fdi_pl_protocol_valid);
    c.io.fdi.pl_stall_req.capture(io_fdi_pl_stall_req);
    c.io.fdi.pl_phy_in_recenter.capture(io_fdi_pl_phy_in_recenter);
    c.io.fdi.pl_phy_in_l1.capture(io_fdi_pl_phy_in_l1);
    c.io.fdi.pl_phy_in_l2.capture(io_fdi_pl_phy_in_l2);
    c.io.fdi.pl_speed_mode.capture(3, io_fdi_pl_speed_mode);
    c.io.fdi.pl_link_width.capture(3, io_fdi_pl_link_width);
    c.io.fdi.pl_clk_req.capture(io_fdi_pl_clk_req);
    c.io.fdi.pl_wake_ack.capture(io_fdi_pl_wake_ack);
    c.io.fdi.pl_config.assignValid(io_fdi_pl_config_valid);
    c.io.fdi.pl_config.assignBits(fdi_params.sb_width, io_fdi_pl_config_bits);
    c.io.fdi.lp_config_credit.capture(io_fdi_lp_config_credit);

    c.io.tl_lp_data_valid.capture(io_tl_lp_data_valid);
    c.io.tl_lp_data_bits.capture(8 * fdi_params.width, io_tl_lp_data_bits);
    c.io.tl_lp_data_irdy.capture(io_tl_lp_data_irdy);
    c.io.tl_ready_to_rcv.capture(io_tl_ready_to_rcv);
    c.io.fault.capture(io_fault);
    c.io.soft_reset.capture(io_soft_reset);

    // runs
    io_fault = true;
    EXPECT_EQ_BOOL(c.io.fdi.lp_link_error(), true);
    io_fault = false;
    EXPECT_EQ_BOOL(c.io.fdi.lp_link_error(), false);
}