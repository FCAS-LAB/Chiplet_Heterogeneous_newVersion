#ifndef __RDI_BRINGUP_HPP__
#define __RDI_BRINGUP_HPP__

#include "utils/wire.hpp"
#include "utils/decoupled.hpp"
#include "utils/module.hpp"
#include "logphy/log_phy_types.hpp"
#include "logphy/sb_msg_wrapper.hpp"

namespace CCPS {

    struct RdiBringupIO {
        Wire<Bool> pl_clk_req; // O
        Wire<Bool> lp_clk_ack; // I
        Wire<Bool> lp_wake_req; // I
        Wire<Bool> pl_wake_ack; // O
        Wire<UInt> lp_state_req; // I       // 4bits
        Wire<UInt> pl_state_status; // O    // 4bits
        Wire<Bool> pl_stall_req; // O
        Wire<Bool> lp_stall_ack; // I
        Wire<Bool> lp_link_error; // I
    };

    class RdiBringup: public RegModule {
    public:
        struct {
            RdiBringupIO rdi_io;                // non-flipped
            SBMsgWrapperTrainIO sb_train_io;    // flipped
            Wire<Bool> active;                  // O
            Wire<Bool> internal_error;          // I
            Wire<Bool> internal_retrain;        // I
        } io;

        RdiBringup();
        void calcNextState() override;

    private:
        // ============ chisel signals =============
        enum class ResetSubState { // 3bits
            CLK_HANDSHAKE=0, LP_WAKE_HANDSHAKE, WAIT_LP_STATE_REQ, REQ_ACTIVE_SEND,
            REQ_ACTIVE_WAIT, RESP_ACTIVE_SEND, RESP_ACTIVE_WAIT
        };
        enum class StallReqAckState { // 2bits
          ACTIVE=0, LP_STALLACK_WAIT, LP_STALLACK_DEASSERT
        };

        RegPtr<UInt> _state;
        Wire<UInt> _next_state;
        RegPtr<UInt> _reset_sub_state;
        RegPtr<UInt> _stall_req_ack_state;
        RegPtr<UInt> _prev_req;
        RegPtr<UInt> _next_state_req;
    };
} // namespace CCPS

#endif // __RDI_BRINGUP_HPP__