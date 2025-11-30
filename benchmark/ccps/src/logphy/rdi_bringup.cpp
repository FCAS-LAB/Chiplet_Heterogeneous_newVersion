#include "logphy/rdi_bringup.hpp"
#include "sideband/sb_msg_encoding.hpp"

namespace CCPS {

    RdiBringup::RdiBringup() {
        // Instantiate
        _state = createReg<UInt>(UInt(4, PhyState::reset));
        _reset_sub_state = createReg<UInt>(UInt(3, ResetSubState::WAIT_LP_STATE_REQ));
        _stall_req_ack_state = createReg<UInt>(UInt(2, StallReqAckState::ACTIVE));
        _prev_req = createReg<UInt>(UInt(4, PhyStateReq::nop));
        _next_state_req = createReg<UInt>(UInt(4, PhyState::reset));

        // connect
        io.rdi_io.pl_clk_req.capture(Bool(true));
        io.rdi_io.pl_wake_ack.capture(Bool(true));

        io.rdi_io.pl_state_status = _state;

        _next_state = [this]() -> UInt {
            PhyState state = static_cast<PhyState>(_state->read().toBigUInt());
            PhyState next_state = state;
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            StallReqAckState stall_req_ack_state = static_cast<StallReqAckState>(_stall_req_ack_state->read().toBigUInt());

            if (io.internal_error() || io.rdi_io.lp_link_error()) {
                next_state = PhyState::linkError;
            } else if (io.internal_retrain()) {
                next_state = PhyState::retrain;
            }

            if (io.rdi_io.lp_state_req().toBigUInt() != PhyStateReq::nop) {
                if (state != PhyState::reset || _prev_req->read().toBigUInt() == PhyStateReq::nop) {
                    next_state = static_cast<PhyState>(io.rdi_io.lp_state_req().toBigUInt());
                }
            }

            if (state == PhyState::reset) {
                if (reset_sub_state == ResetSubState::RESP_ACTIVE_WAIT) {
                    if (io.sb_train_io.msg_req_status.valid() && io.sb_train_io.msg_req_status.ready()) {
                        next_state = PhyState::active;
                    }
                }
            }

            return UInt(4, next_state);
        };

        io.active = [this] () -> Bool {
            return Bool(_state->read().toBigUInt() == PhyState::active);
        };

        io.sb_train_io.msg_req.valid = [this] () -> Bool {
            bool res = false;
            PhyState state = static_cast<PhyState>(_state->read().toBigUInt());
            PhyState next_state = state;
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            StallReqAckState stall_req_ack_state = static_cast<StallReqAckState>(_stall_req_ack_state->read().toBigUInt());

            if (state == PhyState::reset) {
                if (reset_sub_state == ResetSubState::REQ_ACTIVE_SEND) {
                    res = true;
                } else if (reset_sub_state == ResetSubState::RESP_ACTIVE_SEND) {
                    res = true;
                }
            }
            return Bool(res);
        };

        io.sb_train_io.msg_req.msg = [this] () -> UInt {
            BigUInt res = 0;
            PhyState state = static_cast<PhyState>(_state->read().toBigUInt());
            PhyState next_state = state;
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            StallReqAckState stall_req_ack_state = static_cast<StallReqAckState>(_stall_req_ack_state->read().toBigUInt());

            if (state == PhyState::reset) {
                if (reset_sub_state == ResetSubState::REQ_ACTIVE_SEND) {
                    return SBMessage_factory(
                        SBM().LINK_MGMT_RDI_REQ_ACTIVE,
                        "PHY",
                        false,
                        "PHY"
                    );
                } else if (reset_sub_state == ResetSubState::RESP_ACTIVE_SEND) {
                    return SBMessage_factory(
                        SBM().LINK_MGMT_RDI_RSP_ACTIVE,
                        "PHY",
                        false,
                        "PHY"
                    );
                }
            }
            return UInt(64, 0);
        };

        io.sb_train_io.msg_req.timeout_cycles = [this]() -> UInt {
            return UInt(64, 1'000'000);
        };

        io.sb_train_io.msg_req_status.ready = [this]() -> Bool {
            bool res = false;
            PhyState state = static_cast<PhyState>(_state->read().toBigUInt());
            PhyState next_state = state;
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            StallReqAckState stall_req_ack_state = static_cast<StallReqAckState>(_stall_req_ack_state->read().toBigUInt());

            if (state == PhyState::reset) {
                if (reset_sub_state == ResetSubState::REQ_ACTIVE_WAIT) {
                    res = true;
                } else if (reset_sub_state == ResetSubState::RESP_ACTIVE_WAIT) {
                    res = true;
                }
            }
            return Bool(res);
        };

        io.rdi_io.pl_stall_req = [this]() -> Bool {
            return Bool(_stall_req_ack_state->read().toBigUInt() == StallReqAckState::LP_STALLACK_WAIT);
        };
    }

    void RdiBringup::calcNextState() {
        PhyState state = static_cast<PhyState>(_state->read().toBigUInt());
        PhyState next_state = static_cast<PhyState>(_next_state().toBigUInt());
        ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
        StallReqAckState stall_req_ack_state = static_cast<StallReqAckState>(_stall_req_ack_state->read().toBigUInt());

        *_state = UInt(4, next_state);

        if (state != PhyState::reset && next_state == PhyState::reset) {
            *_reset_sub_state = UInt(3, ResetSubState::WAIT_LP_STATE_REQ);
        }

        if (state != PhyState::active && next_state == PhyState::active) {
            *_stall_req_ack_state = UInt(2, StallReqAckState::ACTIVE);
        }

        *_prev_req = io.rdi_io.lp_state_req();

        if (state == PhyState::reset) {
            if (reset_sub_state == ResetSubState::WAIT_LP_STATE_REQ) {
                if (next_state == PhyState::active) {
                    *_state = UInt(4, PhyState::reset);
                    *_reset_sub_state = UInt(3, ResetSubState::REQ_ACTIVE_SEND);
                }
            } else if (reset_sub_state == ResetSubState::REQ_ACTIVE_SEND) {
                if (io.sb_train_io.msg_req.valid() && io.sb_train_io.msg_req.ready()) {
                    *_reset_sub_state = UInt(3, ResetSubState::REQ_ACTIVE_WAIT);
                }
            } else if (reset_sub_state == ResetSubState::REQ_ACTIVE_WAIT) {
                if (io.sb_train_io.msg_req_status.valid() && io.sb_train_io.msg_req_status.ready()) {
                    *_reset_sub_state = UInt(3, ResetSubState::RESP_ACTIVE_SEND);
                }
            } else if (reset_sub_state == ResetSubState::RESP_ACTIVE_SEND) {
                if (io.sb_train_io.msg_req.valid() && io.sb_train_io.msg_req.ready()) {
                    *_reset_sub_state = UInt(3, ResetSubState::RESP_ACTIVE_WAIT);
                }
            }
        } else if (state == PhyState::active) {
            if (stall_req_ack_state == StallReqAckState::ACTIVE) {
                if (next_state == PhyState::retrain || next_state == PhyState::linkReset || next_state == PhyState::disabled) {
                    *_stall_req_ack_state = UInt(2, StallReqAckState::LP_STALLACK_WAIT);
                    *_state = UInt(4, PhyState::active);
                    *_next_state_req = _next_state();
                }
            } else if (stall_req_ack_state == StallReqAckState::LP_STALLACK_WAIT) {
                if (io.rdi_io.lp_stall_ack()) {
                    *_stall_req_ack_state = UInt(2, StallReqAckState::LP_STALLACK_DEASSERT);
                    *_state = UInt(4, PhyState::active);
                }
            } else if (stall_req_ack_state == StallReqAckState::LP_STALLACK_DEASSERT) {
                if (!io.rdi_io.lp_stall_ack()) {
                    *_state = _next_state_req->read();
                }
            }
        }
    }
} // namespace CCPS