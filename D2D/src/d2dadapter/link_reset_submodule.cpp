#include "d2dadapter/link_reset_submodule.hpp"
#include "interfaces/types.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"

namespace CCPS {

    LinkResetSubmodule::LinkResetSubmodule() {
        // Instantiate
        _linkreset_fdi_req_reg          = createReg<Bool>(Bool(false));
        _linkreset_sbmsg_req_rcv_flag   = createReg<Bool>(Bool(false));
        _linkreset_sbmsg_rsp_rcv_flag   = createReg<Bool>(Bool(false));
        _linkreset_sbmsg_ext_rsp_reg    = createReg<Bool>(Bool(false));
        _linkreset_sbmsg_ext_req_reg    = createReg<Bool>(Bool(false));
        // Connect
        _io_linkreset_entry = [this] () -> Bool {
            BigUInt link_state = io.link_state().toBigUInt();
            if (link_state == PhyState::reset ||
                link_state == PhyState::active ||
                link_state == PhyState::retrain) {
                return _linkreset_sbmsg_ext_rsp_reg->read() || _linkreset_sbmsg_rsp_rcv_flag->read();
            } else {
                return Bool(false);
            }
        };
        io.linkreset_entry = _io_linkreset_entry;

        _linkreset_sb_snd = [this] () -> UInt {
            BigUInt res = 0;
            BigUInt link_state = io.link_state().toBigUInt();
            if (link_state == PhyState::reset ||
                link_state == PhyState::active ||
                link_state == PhyState::retrain) {
                if (_linkreset_fdi_req_reg->read() && !_linkreset_sbmsg_req_rcv_flag->read() &&
                    !_linkreset_sbmsg_ext_req_reg->read()) {
                    res = SideBandMessage::REQ_LINKRESET;   // 2
                } else if (_linkreset_sbmsg_req_rcv_flag->read() && !_linkreset_sbmsg_ext_rsp_reg->read()) {
                    res = SideBandMessage::RSP_LINKRESET;
                } else {
                    res = SideBandMessage::NOP;
                }
            } else {
                res = SideBandMessage::NOP;
            }
            return UInt(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, res);
        };
        io.linkreset_sb_snd = _linkreset_sb_snd;
    }

    void LinkResetSubmodule::calcNextState() {
        BigUInt link_state = io.link_state().toBigUInt();

        if (link_state == PhyState::reset ||
            link_state == PhyState::active ||
            link_state == PhyState::retrain)
        {
            // State change request by fdi
            if (link_state == PhyState::reset &&
                io.fdi_lp_state_req().toBigUInt() == PhyStateReq::linkReset &&
                io.fdi_lp_state_req_prev().toBigUInt() == PhyStateReq::nop) {
                *_linkreset_fdi_req_reg = Bool(true);
            } else if (io.fdi_lp_state_req().toBigUInt() == PhyStateReq::linkReset &&
                       link_state != PhyState::reset) {
                *_linkreset_fdi_req_reg = Bool(true);       // 1
            } else {
                *_linkreset_fdi_req_reg = _linkreset_fdi_req_reg->read();
            }

            if (io.linkreset_sb_snd().toBigUInt() == SideBandMessage::REQ_LINKRESET &&
                static_cast<bool>(io.linkreset_sb_rdy())) {
                *_linkreset_sbmsg_ext_req_reg = Bool(true);     // 3
            } else {
                *_linkreset_sbmsg_ext_req_reg = _linkreset_sbmsg_ext_req_reg->read();
            }

            if (io.linkreset_sb_snd().toBigUInt() == SideBandMessage::RSP_LINKRESET &&
                static_cast<bool>(io.linkreset_sb_rdy())) {
                *_linkreset_sbmsg_ext_rsp_reg = Bool(true);
            } else {
                *_linkreset_sbmsg_ext_rsp_reg = _linkreset_sbmsg_ext_rsp_reg->read();
            }

            // Check whether there is inflight linkreset request sbmsg from partner link
            if (io.linkreset_sb_rcv().toBigUInt() == SideBandMessage::REQ_LINKRESET) {
                *_linkreset_sbmsg_req_rcv_flag = Bool(true);
            } else {
                *_linkreset_sbmsg_req_rcv_flag = _linkreset_sbmsg_req_rcv_flag->read();
            }

            /* Check whether there is inflight linkreset response sbmsg from partner
             * link */
            if (io.linkreset_sb_rcv().toBigUInt() == SideBandMessage::RSP_LINKRESET) {
                *_linkreset_sbmsg_rsp_rcv_flag = Bool(true);        // 4
            } else {
                *_linkreset_sbmsg_rsp_rcv_flag = _linkreset_sbmsg_rsp_rcv_flag->read();
            }
        } else {
            *_linkreset_fdi_req_reg = Bool(false);
            *_linkreset_sbmsg_req_rcv_flag = Bool(false);
            *_linkreset_sbmsg_rsp_rcv_flag = Bool(false);
            *_linkreset_sbmsg_ext_req_reg = Bool(false);
            *_linkreset_sbmsg_ext_rsp_reg = Bool(false);
        }
    }
} // namespace CCPS