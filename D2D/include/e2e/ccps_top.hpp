#ifndef __CCPS_TOP_HPP__
#define __CCPS_TOP_HPP__

#include "protocol/protocol_layer.hpp"
#include "d2dadapter/d2d_adapter.hpp"
#include "logphy/logical_phy.hpp"
#include "interfaces/afe.hpp"

namespace CCPS {

    class CCPSTop: public RegModule {
    public:
        struct {
            Valid<UInt> fdi_lp_config;  // fdi_params.sb_width          // useless
            Wire<Bool> fdi_lp_config_credit; // I                       // useless
            Valid<UInt> fdi_pl_config{true};  // fdi_params.sb_width    // useless
            Wire<Bool> fdi_pl_config_credit; // O                       // useless
            Wire<Bool> fdi_lp_stall_ack; // O
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

            MainbandAfeIo mb_afe;
            SidebandAfeIo sb_afe;
        } io;

        CCPSTop(const FdiParams &fdi_params, const RdiParams &rdi_params,
            const SidebandParams &sb_params, BigUInt my_id,
            const LinkTrainingParams &link_training_params, const AfeParams &afe_params,
            const AsyncQueueParams &lane_async_queue_params);

        bool propagateClock() override;

        void calcNextState() override;

    private:
        ModulePtr<ProtocolLayer> _protocol;
        ModulePtr<D2DAdapter> _d2dadapter;
        ModulePtr<LogicalPhy> _logphy;
    };
} // namespace CCPS

#endif // __CCPS_TOP_HPP__