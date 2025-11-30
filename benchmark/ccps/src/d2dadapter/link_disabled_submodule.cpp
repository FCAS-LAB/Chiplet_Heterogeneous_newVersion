#include "d2dadapter/link_disabled_submodule.hpp"
#include "interfaces/types.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"

namespace CCPS {
    // ========================== LinkDisabledSubmodule ===============================
    LinkDisabledSubmodule::LinkDisabledSubmodule() {
        // create registers
        _disabled_fdi_req_reg = createReg<Bool>(Bool(false));
        _disabled_sbmsg_req_rcv_reg = createReg<Bool>(Bool(false));
        _disabled_sbmsg_rsp_rcv_reg = createReg<Bool>(Bool(false));
        _disabled_sbmsg_ext_rsp_reg = createReg<Bool>(Bool(false));
        _disabled_sbmsg_ext_req_reg = createReg<Bool>(Bool(false));

        // connect output wires
        _io_disabled_entry = [this]() -> Bool {
            if (io.link_state().toBigUInt() == PhyState::reset ||
                io.link_state().toBigUInt() == PhyState::active ||
                io.link_state().toBigUInt() == PhyState::retrain ||
                io.link_state().toBigUInt() == PhyState::linkReset
            ) {
                return _disabled_sbmsg_ext_rsp_reg->read() || _disabled_sbmsg_rsp_rcv_reg->read();
            } else {
                return Bool(false);
            }
        };
        io.disabled_entry = _io_disabled_entry;

        _io_disabled_sb_snd = [this]() -> UInt {
            const int sw = D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH;
            if (io.link_state().toBigUInt() == PhyState::reset ||
                io.link_state().toBigUInt() == PhyState::active ||
                io.link_state().toBigUInt() == PhyState::retrain ||
                io.link_state().toBigUInt() == PhyState::linkReset
            ) {
                if (_disabled_fdi_req_reg->read() && !_disabled_sbmsg_req_rcv_reg->read()
                    && !_disabled_sbmsg_ext_req_reg->read()
                ) {
                    return UInt(sw, SideBandMessage::REQ_DISABLED);
                }
                else if (_disabled_sbmsg_req_rcv_reg->read() && !_disabled_sbmsg_ext_rsp_reg->read()) {
                    return UInt(sw, SideBandMessage::RSP_DISABLED);
                }
                else {
                    return UInt(sw, SideBandMessage::NOP);
                }
            } else {
                return UInt(sw, SideBandMessage::NOP);
            }
        };
        io.disabled_sb_snd = _io_disabled_sb_snd;
    }

    void LinkDisabledSubmodule::calcNextState() {
        if (io.link_state().toBigUInt() == PhyState::reset ||
            io.link_state().toBigUInt() == PhyState::active ||
            io.link_state().toBigUInt() == PhyState::retrain ||
            io.link_state().toBigUInt() == PhyState::linkReset
        ) {
            // State change request by fdi
            if (io.link_state().toBigUInt() == PhyState::reset &&
                io.fdi_lp_state_req().toBigUInt() == PhyStateReq::disabled &&
                io.fdi_lp_state_req_prev().toBigUInt() == PhyStateReq::nop
                ) {
                    *_disabled_fdi_req_reg = Bool(true);
            }
            else if (io.fdi_lp_state_req().toBigUInt() == PhyStateReq::disabled &&
                io.link_state().toBigUInt() != PhyState::reset
            ) {
                *_disabled_fdi_req_reg = Bool(true);
            }

            if (io.disabled_sb_snd().toBigUInt() == SideBandMessage::REQ_DISABLED && static_cast<bool>(io.disabled_sb_rdy())) {
                *_disabled_sbmsg_ext_req_reg = Bool(true);
            }

            if (io.disabled_sb_snd().toBigUInt() == SideBandMessage::RSP_DISABLED && static_cast<bool>(io.disabled_sb_rdy())) {
                *_disabled_sbmsg_ext_rsp_reg = Bool(true);
            }

            // Check whether there is inflight disabled request sbmsg from partner link
            if (io.disabled_sb_rcv().toBigUInt() == SideBandMessage::REQ_DISABLED) {
                *_disabled_sbmsg_req_rcv_reg = Bool(true);
            }

            /* Check whether there is inflight disaabled response sbmsg from partner
             * link */
            if (io.disabled_sb_rcv().toBigUInt() == SideBandMessage::RSP_DISABLED) {
                *_disabled_sbmsg_rsp_rcv_reg = Bool(true);
            }
        } else {
            *_disabled_fdi_req_reg       = Bool(false);
            *_disabled_sbmsg_req_rcv_reg = Bool(false);
            *_disabled_sbmsg_rsp_rcv_reg = Bool(false);
            *_disabled_sbmsg_ext_req_reg = Bool(false);
            *_disabled_sbmsg_ext_rsp_reg = Bool(false);
        }
    }

} // namespace