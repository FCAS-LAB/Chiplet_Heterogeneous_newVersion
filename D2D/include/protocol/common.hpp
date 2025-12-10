#ifndef __PROTOCOL_COMMON_HPP__
#define __PROTOCOL_COMMON_HPP__

#include "tilelink/common.hpp"
using namespace CCPS;
namespace CCPS {

    struct ProtocolLayerParams {
        int ccps_filt_width{64};
        int ccps_filt_size{4};
        int ccps_non_ecc_width{448};
        int ccps_ecc_width{64};
        int host_id_width{8};
        int partner_id_width{8};
        int credit_width{4};
        int reserved_cmd_width{24};
    };

    struct CCPSCmdFormat {
        Wire<UInt> msg_type; // O 4bits
        Wire<UInt> host_id;  // O proto.hostIDWIDTH
        Wire<UInt> partner_id; // O proto.partnerIDWIDTH
        Wire<UInt> tl_a_credit; // O proto.creditWIDTH
        Wire<UInt> tl_b_credit; // O proto.creditWIDTH
        Wire<UInt> tl_c_credit; // O proto.creditWIDTH
        Wire<UInt> tl_d_credit; // O proto.creditWIDTH
        Wire<UInt> tl_e_credit; // O proto.creditWIDTH
        Wire<UInt> reserved_cmd; // O proto.reservedCmdWIDTH

        UInt toUInt() const;

        friend std::ostream& operator<<(std::ostream& os, const CCPSCmdFormat& obj);
    };

    struct CCPSHeader1Format {
        Wire<UInt> address; // O tl.addressWidth

        UInt toUInt() const;

        friend std::ostream& operator<<(std::ostream& os, const CCPSHeader1Format& obj);
    };

    struct CCPSHeader2Format {
        Wire<UInt> opcode; // O tl.opcodeWidth
        Wire<UInt> param; // O tl.paramWidth
        Wire<UInt> size; // O tl.sizeWidth
        Wire<UInt> source; // O tl.sourceIDWidth
        Wire<UInt> sink; // O tl.sinkIDWidth
        Wire<UInt> mask; // O tl.maskWidth
        Wire<UInt> reservedh2; // O tl.reservedH2Width

        UInt toUInt() const;

        friend std::ostream& operator<<(std::ostream& os, const CCPSHeader2Format& obj);
    };

    struct CCPSRawPayloadFormat {
        CCPSCmdFormat cmd;  // non-flipped
        CCPSHeader1Format header1; // non-flipped
        CCPSHeader2Format header2; // non-flipped
        std::vector<Wire<UInt>> data; // non-flipped, O proto.ccpsFlitSize * proto.ccpsFlitWidth
        Wire<UInt> ecc; // O proto.ccpsEccWidth

        UInt toUInt() const;
        UInt toUIntWithOutEcc() const;

        void dumpData(const std::string& file_path) const;

        friend std::ostream& operator<<(std::ostream& os, const CCPSRawPayloadFormat& obj);
    };

    struct d2dConfig {
        Wire<UInt> d2d_cycles_1us; // O 32bits
        Wire<UInt> d2d_uncorrectable_error_csr; // O 32bits
        Wire<UInt> d2d_uncorrectable_error_mask_csr; // O 32bits
        Wire<UInt> d2d_uncorrectable_error_severity_csr; // O 32bits
        Wire<UInt> d2d_correctable_error_csr; // O 32bits
        Wire<UInt> d2d_correctable_error_mask_csr; // O 32bits
        Wire<UInt> d2d_header_log_1_csr; // O 64bits
        Wire<UInt> d2d_header_log_2_csr; // O 64bits
        Wire<UInt> d2d_error_and_link_testing_parity_log0; // O 64bits
        Wire<UInt> d2d_error_and_link_testing_parity_log1; // O 64bits
        Wire<UInt> d2d_error_and_link_testing_parity_log2; // O 64bits
        Wire<UInt> d2d_error_and_link_testing_parity_log3; // O 64bits
        Wire<UInt> advertised_adapter_capability; // O 64bits
        Wire<UInt> d2d_stack_num; // O 1bit
        Wire<UInt> d2d_state_can_reset; // O 1bit
        Wire<UInt> d2d_flush_and_reset; // O 1bit
    };

    struct sbConfig {
        Wire<UInt> sideband_mailbox_index_low; // I 32bits
        Wire<UInt> sideband_mailbox_index_high; // I 32bits
        Wire<UInt> sideband_mailbox_data_low; // I 32bits
        Wire<UInt> sideband_mailbox_data_high; // I 32bits
        Wire<UInt> sideband_mailbox_ready; // O 1bit
        Wire<UInt> sideband_mailbox_valid; // I 1bit
        Wire<UInt> sideband_mailbox_sw_to_node_index_low; // O 32bits
        Wire<UInt> sidebank_mailbox_sw_to_node_index_high; // O 32bits
        Wire<UInt> sideband_mailbox_sw_to_node_data_low; // O 32bits
        Wire<UInt> sideband_mailbox_sw_to_node_data_high; // O 32bits
        Wire<UInt> sideband_mailbox_sw_ready; // O 1bit
        Wire<UInt> sideband_mailbox_sw_valid; // O 1bit
    };
}  // namespace CCPS

#endif // __PROTOCOL_COMMON_HPP__