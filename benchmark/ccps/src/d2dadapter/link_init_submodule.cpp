#include "d2dadapter/link_init_submodule.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"
#include "interfaces/types.hpp"

namespace CCPS {
    LinkInitSubmodule::LinkInitSubmodule() {
        // Instantiate
        // State register for link initialization
        _linkinit_state_reg = createReg<UInt>(UInt(3, LinkInitState::INIT_START));
        // Parameter exchange on sideband message arbitration flags
        _param_exch_sbmsg_rcv_flag = createReg<Bool>(Bool(false));
        _param_exch_sbmsg_snt_flag = createReg<Bool>(Bool(false));
        // Active state sb message arbitration flags
        _active_sbmsg_req_rcv_flag = createReg<Bool>(Bool(false));
        _active_sbmsg_rsp_rcv_flag = createReg<Bool>(Bool(false));
        _active_sbmsg_ext_rsp_reg = createReg<Bool>(Bool(false));
        _active_sbmsg_ext_req_reg = createReg<Bool>(Bool(false));
        _transition_to_active_reg = createReg<Bool>(Bool(false));

        // connect
        _linkinit_fdi_pl_inband_pres = [this]() -> Bool {
            BigUInt io_link_state = io.link_state().toBigUInt();
            bool res = false;
            if (io_link_state == PhyState::reset) {
                BigUInt link_state_reg = _linkinit_state_reg->read().toBigUInt();
                if (link_state_reg == LinkInitState::FDI_BRINGUP) {
                    res = true;
                } else if (link_state_reg == LinkInitState::INIT_DONE) {
                    res = true;
                }
            } else {
                res = false;
            }
            return Bool(res);
        };
        io.linkinit_fdi_pl_inband_pres = _linkinit_fdi_pl_inband_pres;

        _linkinit_fdi_pl_rxactive_req = [this]() -> Bool {
            BigUInt io_link_state = io.link_state().toBigUInt();
            bool res = false;
            if (io_link_state == PhyState::reset) {
                res = false;
                BigUInt link_state_reg = _linkinit_state_reg->read().toBigUInt();
                if (link_state_reg == LinkInitState::INIT_START) {
                    res = false;
                } else if (link_state_reg == LinkInitState::FDI_BRINGUP) {
                    if (_active_sbmsg_req_rcv_flag->read()) {
                        res = true;
                    } else {
                        res = false;
                    }
                } else if (link_state_reg == LinkInitState::INIT_DONE) {
                    res = true;
                }
            } else {
                res = false;
            }
            return Bool(res);
        };
        io.linkinit_fdi_pl_rxactive_req = _linkinit_fdi_pl_rxactive_req;

        _linkinit_fdi_pl_state_sts = [this]() -> UInt {
            BigUInt io_link_state = io.link_state().toBigUInt();
            BigUInt res = PhyState::reset;
            if (io_link_state == PhyState::reset) {
                BigUInt link_state_reg = _linkinit_state_reg->read().toBigUInt();
                if (link_state_reg == LinkInitState::INIT_DONE) {
                    res = PhyState::active;
                }
            }
            return UInt(4, res);
        };
        io.linkinit_fdi_pl_state_sts = _linkinit_fdi_pl_state_sts;

        _linkinit_rdi_lp_state_req = [this]() -> UInt {
            BigUInt io_link_state = io.link_state().toBigUInt();
            BigUInt res = PhyStateReq::nop;
            if (io_link_state == PhyState::reset) {
                BigUInt link_state_reg = _linkinit_state_reg->read().toBigUInt();
                if (link_state_reg == LinkInitState::INIT_START) {
                    res = PhyStateReq::nop;
                } else if (link_state_reg == LinkInitState::RDI_BRINGUP) {
                    res = PhyStateReq::active;
                } else if (link_state_reg == LinkInitState::PARAM_EXCH) {
                    res = PhyStateReq::active;
                } else if (link_state_reg == LinkInitState::FDI_BRINGUP) {
                    res = PhyStateReq::active;
                } else if (link_state_reg == LinkInitState::INIT_DONE) {
                    res = PhyStateReq::active;
                }
            } else {
                res = PhyStateReq::nop;
            }
            return UInt(4, res);
        };
        io.linkinit_rdi_lp_state_req = _linkinit_rdi_lp_state_req;

        _active_entry = [this]() -> Bool {
            BigUInt io_link_state = io.link_state().toBigUInt();
            bool res = false;
            if (io_link_state == PhyState::reset) {
                res = false;
                BigUInt link_state_reg = _linkinit_state_reg->read().toBigUInt();
                if (link_state_reg == LinkInitState::INIT_START) {
                    res = false;
                } else if (link_state_reg == LinkInitState::INIT_DONE) {
                    res = true;
                }
            } else {
                res = false;
            }
            return Bool(res);
        };
        io.active_entry = _active_entry;

        _linkinit_sb_snd = [this]() -> UInt {
            BigUInt io_link_state = io.link_state().toBigUInt();
            BigUInt res = SideBandMessage::NOP;
            if (io_link_state == PhyState::reset) {
                res = SideBandMessage::NOP;
                BigUInt link_state_reg = _linkinit_state_reg->read().toBigUInt();
                if (link_state_reg == LinkInitState::INIT_START) {
                    res = SideBandMessage::NOP;
                } else if (link_state_reg == LinkInitState::PARAM_EXCH) {
                    if (!_param_exch_sbmsg_snt_flag->read()) {
                        res = SideBandMessage::ADV_CAP;
                    } else {
                        res = SideBandMessage::NOP;
                    }
                } else if (link_state_reg == LinkInitState::FDI_BRINGUP) {
                    if (io.fdi_lp_rxactive_sts() && io.linkinit_fdi_pl_rxactive_req() && !_active_sbmsg_ext_rsp_reg->read()) {
                        res = SideBandMessage::RSP_ACTIVE;
                    } else if (_transition_to_active_reg->read() && !_active_sbmsg_ext_req_reg->read()) {
                        res = SideBandMessage::REQ_ACTIVE;
                    } else {
                        res = SideBandMessage::NOP;
                    }
                } else if (link_state_reg == LinkInitState::INIT_DONE) {
                    res = SideBandMessage::NOP;
                }
            } else {
                res = SideBandMessage::NOP;
            }
            return UInt(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, res);
        };
        io.linkinit_sb_snd = _linkinit_sb_snd;
    }

    void LinkInitSubmodule::calcNextState() {
        BigUInt io_link_state = io.link_state().toBigUInt();

        int sw = 3; // state width
        if (io_link_state == PhyState::reset) {
            *_param_exch_sbmsg_rcv_flag = Bool(false);
            *_param_exch_sbmsg_snt_flag = Bool(false);
            *_active_sbmsg_req_rcv_flag = Bool(false);
            *_active_sbmsg_rsp_rcv_flag = Bool(false);
            *_active_sbmsg_ext_rsp_reg = Bool(false);
            *_active_sbmsg_ext_req_reg = Bool(false);
            *_transition_to_active_reg = Bool(false);

            int linkinit_state_reg = static_cast<int>(_linkinit_state_reg->read().toBigUInt());
            switch (linkinit_state_reg)
            {
            case LinkInitState::INIT_START:
                if (io.rdi_pl_inband_pres()) {
                    *_linkinit_state_reg = UInt(sw, LinkInitState::RDI_BRINGUP);
                } else {
                    *_linkinit_state_reg = _linkinit_state_reg->read();
                }
                break;
            case LinkInitState::RDI_BRINGUP:
                if (io.rdi_pl_state_sts().toBigUInt() == PhyState::active) {
                    *_linkinit_state_reg = UInt(sw, LinkInitState::PARAM_EXCH);
                } else {
                    *_linkinit_state_reg = _linkinit_state_reg->read();
                }
                break;
            case LinkInitState::PARAM_EXCH:
                // Check whether there is inflight AdvCap sbmsg from partner link adapter
                if (io.linkinit_sb_rcv().toBigUInt() == SideBandMessage::ADV_CAP) {
                    *_param_exch_sbmsg_rcv_flag = Bool(true);
                } else {
                    *_param_exch_sbmsg_rcv_flag = _param_exch_sbmsg_rcv_flag->read();
                }
                if (io.linkinit_sb_rdy() && io.linkinit_sb_snd().toBigUInt() == SideBandMessage::ADV_CAP) {
                    *_param_exch_sbmsg_snt_flag = Bool(true);
                } else {
                    *_param_exch_sbmsg_snt_flag = _param_exch_sbmsg_snt_flag->read();
                }
                // Check if AdvCap was sent and received and vice versa
                if (_param_exch_sbmsg_snt_flag->read() && _param_exch_sbmsg_rcv_flag->read()) {
                    *_linkinit_state_reg = UInt(sw, LinkInitState::FDI_BRINGUP);
                } else {
                    *_linkinit_state_reg = _linkinit_state_reg->read();
                }
                break;
            case LinkInitState::FDI_BRINGUP:
                if (io.linkinit_sb_rcv().toBigUInt() == SideBandMessage::RSP_ACTIVE) {
                    *_active_sbmsg_rsp_rcv_flag = Bool(true);
                } else {
                    *_active_sbmsg_rsp_rcv_flag = _active_sbmsg_rsp_rcv_flag->read();
                }
                if (io.linkinit_sb_rcv().toBigUInt() == SideBandMessage::REQ_ACTIVE) {
                    *_active_sbmsg_req_rcv_flag = Bool(true);
                } else {
                    *_active_sbmsg_req_rcv_flag = _active_sbmsg_req_rcv_flag->read();
                }
                if (io.linkinit_sb_snd().toBigUInt() == SideBandMessage::RSP_ACTIVE && io.linkinit_sb_rdy()) {
                    *_active_sbmsg_ext_rsp_reg = Bool(true);
                } else {
                    *_active_sbmsg_ext_rsp_reg = _active_sbmsg_ext_rsp_reg->read();
                }
                if (io.linkinit_sb_snd().toBigUInt() == SideBandMessage::REQ_ACTIVE && io.linkinit_sb_rdy()) {
                    *_active_sbmsg_ext_req_reg = Bool(true);
                } else {
                    *_active_sbmsg_ext_req_reg = _active_sbmsg_ext_req_reg->read();
                }
                if (io.fdi_lp_state_req().toBigUInt() == PhyStateReq::active &&
                    io.fdi_lp_state_req_prev().toBigUInt() == PhyStateReq::nop) {
                    *_transition_to_active_reg = Bool(true);
                } else {
                    *_transition_to_active_reg = _transition_to_active_reg->read();
                }
                if (_active_sbmsg_ext_rsp_reg->read() && _active_sbmsg_rsp_rcv_flag->read()) {
                    *_linkinit_state_reg = UInt(sw, LinkInitState::INIT_DONE);
                } else {
                    *_linkinit_state_reg = _linkinit_state_reg->read();
                }
                break;
            case LinkInitState::INIT_DONE:
                *_linkinit_state_reg = UInt(sw, LinkInitState::INIT_DONE);
                break;

            default:
                break;
            }
        } else {
            *_linkinit_state_reg = UInt(sw, LinkInitState::INIT_START);
            *_param_exch_sbmsg_rcv_flag = Bool(false);
            *_param_exch_sbmsg_snt_flag = Bool(false);
        }

        //std::cout << getPathName() << ": fdi_lp_state_req               " << io.fdi_lp_state_req().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": fdi_lp_state_req_prev          " << io.fdi_lp_state_req_prev().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": fdi_lp_rxactive_sts            " << static_cast<bool>(io.fdi_lp_rxactive_sts()) << std::endl;
        //std::cout << getPathName() << ": linkinit_fdi_pl_inband_pres    " << static_cast<bool>(io.linkinit_fdi_pl_inband_pres()) << std::endl;
        //std::cout << getPathName() << ": linkinit_fdi_pl_rxactive_req   " << static_cast<bool>(io.linkinit_fdi_pl_rxactive_req()) << std::endl;
        //std::cout << getPathName() << ": linkinit_fdi_pl_state_sts      " << io.linkinit_fdi_pl_state_sts().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": rdi_pl_state_sts               " << io.rdi_pl_state_sts().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": rdi_pl_inband_pres             " << static_cast<bool>(io.rdi_pl_inband_pres()) << std::endl;
        //std::cout << getPathName() << ": linkinit_rdi_lp_state_req      " << io.linkinit_rdi_lp_state_req().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": link_state                     " << io.link_state().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": active_entry                   " << static_cast<bool>(io.active_entry()) << std::endl;
        //std::cout << getPathName() << ": linkinit_sb_snd                " << io.linkinit_sb_snd().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": linkinit_sb_rcv                " << io.linkinit_sb_rcv().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": linkinit_sb_rdy                " << static_cast<bool>(io.linkinit_sb_rdy()) << std::endl;
        //
        //std::cout << getPathName() << ": linkinit_state_reg             " << _linkinit_state_reg->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": param_exch_sbmsg_rcv_flag      " << static_cast<bool>(_param_exch_sbmsg_rcv_flag->read()) << std::endl;
        //std::cout << getPathName() << ": param_exch_sbmsg_snt_flag      " << static_cast<bool>(_param_exch_sbmsg_snt_flag->read()) << std::endl;
        //std::cout << getPathName() << ": active_sbmsg_req_rcv_flag      " << static_cast<bool>(_active_sbmsg_req_rcv_flag->read()) << std::endl;
        //std::cout << getPathName() << ": active_sbmsg_rsp_rcv_flag      " << static_cast<bool>(_active_sbmsg_rsp_rcv_flag->read()) << std::endl;
        //std::cout << getPathName() << ": active_sbmsg_ext_rsp_reg       " << static_cast<bool>(_active_sbmsg_ext_rsp_reg ->read()) << std::endl;
        //std::cout << getPathName() << ": active_sbmsg_ext_req_reg       " << static_cast<bool>(_active_sbmsg_ext_req_reg ->read()) << std::endl;
        //std::cout << getPathName() << ": transition_to_active_reg       " << static_cast<bool>(_transition_to_active_reg ->read()) << std::endl;

    }
} // namespace CCPS