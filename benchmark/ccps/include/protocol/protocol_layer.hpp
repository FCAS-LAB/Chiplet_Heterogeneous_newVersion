#ifndef __PROTOCOL_LAYER_HPP__
#define __PROTOCOL_LAYER_HPP__

#include "utils/wire.hpp"
#include "utils/decoupled.hpp"
#include "utils/reg.hpp"
#include "utils/module.hpp"
#include "protocol/common.hpp"
#include "interfaces/fdi.hpp"

namespace CCPS {

    class ProtocolLayer : public RegModule {
    public:
        struct {
            Fdi fdi;  // non-flipped
            Wire<UInt> tl_pl_state_status; // O 4bits
            Wire<Bool> tl_lp_data_valid;    // I
            Wire<UInt> tl_lp_data_bits;     // I 8 * fdiParams.width
            Wire<Bool> tl_lp_data_irdy;     // I
            Wire<Bool> tl_lp_data_ready;    // O
            Wire<UInt> tl_pl_data_bits;     // O 8 * fdiParams.width
            Wire<Bool> tl_pl_data_valid;    // O
            Wire<Bool> tl_ready_to_rcv;     // I
            Wire<Bool> fault;               // I
            Wire<Bool> soft_reset;          // I
        } io;

        ProtocolLayer(const FdiParams &fdi_params);

        void calcNextState() override;

    private:
        // ----------- chisel signals -----------
        RegPtr<Bool> _lp_rx_active_sts_reg;
        RegPtr<UInt> _lp_state_req_reg;     // 4bits
        RegPtr<Bool> _lp_stall_reg;
        RegPtr<UInt> _pl_protocol_reg;      // 3bits
        RegPtr<UInt> _pl_protocol_flitfmt_reg;  // 4bits
        Wire<UInt> _streaming_proto_stack;  // 4bits
        Wire<UInt> _streaming_proto_type;   // 4bits
        Wire<Bool> _lp_rx_active_pl_state;
        Wire<Bool> _req_active;

        // debug
        RegPtr<Bool> _lp_data_valid_d1_reg;
        RegPtr<Bool> _pl_data_valid_d1_reg;
    };
} // namespace CCPS

#endif // __PROTOCOL_LAYER_HPP__