#include "protocol/protocol_layer.hpp"
#include <fstream>

namespace CCPS {
    ProtocolLayer::ProtocolLayer(const FdiParams &fdi_params) {
        // Instantiate
        _lp_rx_active_sts_reg = createReg<Bool>(Bool(false));
        _lp_state_req_reg = createReg<UInt>(UInt(4, BigUInt(PhyStateReq::nop)));
        _lp_stall_reg = createReg<Bool>(Bool(false));
        _pl_protocol_reg = createReg<UInt>(UInt(3, 0));
        _pl_protocol_flitfmt_reg = createReg<UInt>(UInt(4, 0));

        // connect
        io.tl_pl_state_status = io.fdi.pl_state_status;

        io.fdi.lp_data.assignBits(io.tl_lp_data_bits);
        io.fdi.lp_data.assignValid(io.tl_lp_data_valid);
        io.fdi.lp_data_irdy = io.tl_lp_data_irdy;
        io.tl_lp_data_ready = [this]() -> Bool {
            return io.fdi.lp_data.isReady();
        };

        io.tl_pl_data_valid = [this]() -> Bool {
            return io.fdi.pl_data.isValid();
        };
        io.tl_pl_data_bits = [this]() -> UInt {
            return io.fdi.pl_data.bits();
        };

        // Constants for the FDI signals not used in v1
        io.fdi.lp_retimer_crd.capture(false);
        io.fdi.lp_corrupt_crc.capture(false);
        io.fdi.lp_dllp.assignValid(false);
        io.fdi.lp_dllp.assignBits(fdi_params.dllp_width, BigUInt(0));
        io.fdi.lp_dllp_ofc.capture(false);
        // Dynamic clock gating feature not supported in v1
        io.fdi.lp_clk_ack.capture(true);
        io.fdi.lp_wake_req.capture(true);

        // Tie lpStream to streaming protocol on stack 0
        _streaming_proto_stack.capture(4, ProtoStack::stack0);
        _streaming_proto_type.capture(4, ProtoStreamType::Stream);
        io.fdi.lp_stream.proto_stack = _streaming_proto_stack;
        io.fdi.lp_stream.proto_type = _streaming_proto_type;

        _lp_rx_active_pl_state = [this]() -> Bool {
            return Bool(io.fdi.pl_state_status().toBigUInt() == PhyState::active);
        };

        io.fdi.lp_rx_active_status = _lp_rx_active_sts_reg;

        _req_active = [this]() -> Bool {
            return Bool((io.fdi.pl_state_status().toBigUInt() == PhyState::reset &&
                    _lp_state_req_reg->read().toBigUInt() == PhyStateReq::nop &&
                    static_cast<bool>(io.fdi.pl_inband_pres())) ||
                (io.fdi.pl_state_status().toBigUInt() == PhyState::linkReset));
        };

        io.fdi.lp_state_req = _lp_state_req_reg;

        // lpLinkError should be asserted when there is an error detected by protocol layer
        // should be done as a ECC check in CCPS flit
        io.fdi.lp_link_error = io.fault;

        // Refer to section 8.3.2
        // Whent he lpStallAck is asserted the TL A channel is stalled and the lp irdy and valid
        // signals are deasserted in the CCPSTLFront class.
        io.fdi.lp_stall_ack = _lp_stall_reg;

        io.fdi.lp_config.assignBits(fdi_params.sb_width, BigUInt(0));
        io.fdi.lp_config.assignValid(false);
        io.fdi.pl_config_credit.capture(false);
    }

    void ProtocolLayer::calcNextState() {
        if (io.fdi.pl_rx_active_req() && io.tl_ready_to_rcv() && _lp_rx_active_pl_state()) {
            *_lp_rx_active_sts_reg = Bool(true);
        }

        if (_req_active()) {
            *_lp_state_req_reg = UInt(4, PhyStateReq::active);
        } else if (!_req_active() && io.soft_reset()) {
            *_lp_state_req_reg = UInt(4, PhyStateReq::linkReset);
        } else {
            *_lp_state_req_reg = UInt(4, PhyStateReq::nop);
        }

        *_lp_stall_reg = io.fdi.pl_stall_req();

        if (io.fdi.pl_protocol_valid()) {
            *_pl_protocol_reg = io.fdi.pl_protocol();
            *_pl_protocol_flitfmt_reg = io.fdi.pl_protocol_flit_format();
        }
#ifdef DEBUG_LEVEL_1
        static std::ofstream log_file("protocol_layer.txt");
        static int step_counter{0};

        // set hex for the following
        log_file << std::dec;
        log_file << "===================== step " << step_counter << " =====================" << std::endl;
        log_file << std::hex;

        // fdi
        log_file << getPathName() << ": io.fdi.lp_data.isValid " << static_cast<bool>(io.fdi.lp_data.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_data.isReady " << static_cast<bool>(io.fdi.lp_data.isReady()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_data.bits " << io.fdi.lp_data.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.lp_data_irdy " << static_cast<bool>(io.fdi.lp_data_irdy()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_data.isValid " << static_cast<bool>(io.fdi.pl_data.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_data.bits " << io.fdi.pl_data.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.lp_retimer_crd " << static_cast<bool>(io.fdi.lp_retimer_crd()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_corrupt_crc " << static_cast<bool>(io.fdi.lp_corrupt_crc()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_dllp.isValid " << static_cast<bool>(io.fdi.lp_dllp.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_dllp.bits " << io.fdi.lp_dllp.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.lp_dllp_ofc " << static_cast<bool>(io.fdi.lp_dllp_ofc()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_retimer_crd " << static_cast<bool>(io.fdi.pl_retimer_crd()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_dllp.isValid " << static_cast<bool>(io.fdi.pl_dllp.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_dllp.bits " << io.fdi.pl_dllp.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_dllp_ofc " << static_cast<bool>(io.fdi.pl_dllp_ofc()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_stream.proto_stack " << io.fdi.pl_stream.proto_stack().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_stream.proto_type " << io.fdi.pl_stream.proto_type().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_flit_cancel " << static_cast<bool>(io.fdi.pl_flit_cancel()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_state_req " << io.fdi.lp_state_req().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.lp_link_error " << static_cast<bool>(io.fdi.lp_link_error()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_state_status " << io.fdi.pl_state_status().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_inband_pres " << static_cast<bool>(io.fdi.pl_inband_pres()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_error " << static_cast<bool>(io.fdi.pl_error()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_cerror " << static_cast<bool>(io.fdi.pl_cerror()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_nf_error " << static_cast<bool>(io.fdi.pl_nf_error()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_train_error " << static_cast<bool>(io.fdi.pl_train_error()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_rx_active_req " << static_cast<bool>(io.fdi.pl_rx_active_req()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_rx_active_status " << static_cast<bool>(io.fdi.lp_rx_active_status()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_protocol " << io.fdi.pl_protocol().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_protocol_flit_format " << io.fdi.pl_protocol_flit_format().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_protocol_valid " << static_cast<bool>(io.fdi.pl_protocol_valid()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_stall_req " << static_cast<bool>(io.fdi.pl_stall_req()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_stall_ack " << static_cast<bool>(io.fdi.lp_stall_ack()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_phy_in_recenter " << static_cast<bool>(io.fdi.pl_phy_in_recenter()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_phy_in_l1 " << static_cast<bool>(io.fdi.pl_phy_in_l1()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_phy_in_l2 " << static_cast<bool>(io.fdi.pl_phy_in_l2()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_speed_mode " << io.fdi.pl_speed_mode().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_link_width " << io.fdi.pl_link_width().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_clk_req " << static_cast<bool>(io.fdi.pl_clk_req()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_clk_ack " << static_cast<bool>(io.fdi.lp_clk_ack()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_wake_req " << static_cast<bool>(io.fdi.lp_wake_req()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_wake_ack " << static_cast<bool>(io.fdi.pl_wake_ack()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_config.isValid " << static_cast<bool>(io.fdi.pl_config.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_config.bits " << io.fdi.pl_config.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_config_credit " << static_cast<bool>(io.fdi.pl_config_credit()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_config.isValid " << static_cast<bool>(io.fdi.lp_config.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_config.bits " << io.fdi.lp_config.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.lp_config_credit " << static_cast<bool>(io.fdi.lp_config_credit()) << std::endl;

        log_file << getPathName() << ": io.tl_lp_data_valid " << static_cast<bool>(io.tl_lp_data_valid()) << std::endl;
        log_file << getPathName() << ": io.tl_lp_data_bits 0x" << io.tl_lp_data_bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase) << std::endl;
        log_file << getPathName() << ": io.tl_lp_data_irdy " << static_cast<bool>(io.tl_lp_data_irdy()) << std::endl;
        log_file << getPathName() << ": io.tl_lp_data_ready " << static_cast<bool>(io.tl_lp_data_ready()) << std::endl;
        log_file << getPathName() << ": io.tl_pl_data_bits 0x" << io.tl_pl_data_bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase) << std::endl;
        log_file << getPathName() << ": io.tl_pl_data_valid " << static_cast<bool>(io.tl_pl_data_valid()) << std::endl;
        log_file << getPathName() << ": io.tl_ready_to_rcv " << static_cast<bool>(io.tl_ready_to_rcv()) << std::endl;
        log_file << getPathName() << ": io.fault " << static_cast<bool>(io.fault()) << std::endl;
        log_file << getPathName() << ": io.soft_reset " << static_cast<bool>(io.soft_reset()) << std::endl;

        // internal
        log_file << getPathName() << ": _lp_rx_active_sts_reg" << static_cast<bool>(_lp_rx_active_sts_reg->read()) << std::endl;
        log_file << getPathName() << ": _lp_state_req_reg" << _lp_state_req_reg->read().toBigUInt() << std::endl;
        log_file << getPathName() << ": _lp_stall_reg" << static_cast<bool>(_lp_stall_reg->read()) << std::endl;
        log_file << getPathName() << ": _pl_protocol_reg" << _pl_protocol_reg->read().toBigUInt() << std::endl;
        log_file << getPathName() << ": _pl_protocol_flitfmt_reg" << _pl_protocol_flitfmt_reg->read().toBigUInt() << std::endl;

        log_file << std::endl;

        // debug
        const auto packet_info = PacketInfoManager::GetInstance().peek();
        static std::ofstream keystone_log_file("protocol_layer_keystone.txt");

        // lp
        static int lp_packet_count = 0;
        static int lp_tag_count = 0;
        if (io.fdi.lp_data.isValid() && io.fdi.lp_data.isReady()) {
            keystone_log_file << getPathName() << ": lp_data, clock_step " << step_counter
                << " packet_idx " << std::dec << lp_packet_count
                << " tag_idx " << std::dec << lp_tag_count
                << " bits 0x" << io.fdi.lp_data.bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase)
                << std::endl;
            if (packet_info.finished && lp_tag_count == packet_info.tag_index) {
                lp_packet_count++;
                lp_tag_count = 0;
            } else {
                lp_tag_count++;
            }
        }

        // pl
        static int pl_packet_count = 0;
        static int pl_tag_count = 0;
        if (io.fdi.pl_data.isValid()) {
            keystone_log_file << getPathName() << ": pl_data, clock_step " << step_counter
                << " packet_idx " << std::dec << pl_packet_count
                << " tag_idx " << std::dec << pl_tag_count
                << " bits 0x" << io.fdi.pl_data.bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase)
                << std::endl;
            if (packet_info.finished && pl_tag_count == packet_info.tag_index) {
                pl_packet_count++;
                pl_tag_count = 0;
            } else {
                pl_tag_count++;
            }
        }

        step_counter++;

#endif // DEBUG_LEVEL_1

    }
} // namespace CCPS
