#include "d2dadapter/link_management_controller.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"
#include "interfaces/types.hpp"
#include "utils/wire.hpp"
#include "utils/print.hpp"


namespace CCPS {
    LinkManagementController::LinkManagementController(
        const FdiParams &fdi_params,
        const RdiParams &rdi_params,
        const SidebandParams &sb_params
    ) {
        // Instantiate
        _disabled_submodule = createSubmodule<LinkDisabledSubmodule>("disabled_submodule");
        _linkreset_submodule = createSubmodule<LinkResetSubmodule>("linkreset_submodule");
        _linkinit_submodule = createSubmodule<LinkInitSubmodule>("linkinit_submodule");
        _parity_negotiation_submodule = createSubmodule<ParityNegotiationSubmodule>("parity_negotiation_submodule");

        _rdi_lp_linkerror_reg = createReg<Bool>(Bool(false));
        _rdi_lp_state_req_reg = createReg<UInt>(UInt(4, PhyStateReq::nop));
        _fdi_pl_rxactive_req_reg = createReg<Bool>(Bool(false));
        _fdi_pl_inband_pres_reg = createReg<Bool>(Bool(false));
        _linkmgmt_stallreq_reg = createReg<Bool>(Bool(false));
        _fdi_lp_state_req_prev_reg = createReg<UInt>(UInt(4, PhyStateReq::nop));
        _link_state_reg = createReg<UInt>(UInt(4, PhyState::reset));

        // connect
        // Top level IO signal assignments
        // RDI
        io.rdi_lp_linkerror = _rdi_lp_linkerror_reg;
        io.rdi_lp_state_req = _rdi_lp_state_req_reg;
        // FDI
        io.fdi_pl_state_sts = _link_state_reg;
        io.fdi_pl_rx_active_req = _fdi_pl_rxactive_req_reg;
        io.fdi_pl_inband_pres = _fdi_pl_inband_pres_reg;

        io.linkmgmt_stallreq = _linkmgmt_stallreq_reg;

        // Submodule IO signal assignments
        // Disabled submodule
        _disabled_submodule->io.fdi_lp_state_req = io.fdi_lp_state_req;
        _disabled_submodule->io.fdi_lp_state_req_prev = _fdi_lp_state_req_prev_reg;
        _disabled_submodule->io.link_state = _link_state_reg;
        #define _disabled_entry _disabled_submodule->io.disabled_entry
        // Intermediate sideband messgaes which gets assigned to top IO when required
        #define _disabled_sb_snd _disabled_submodule->io.disabled_sb_snd

        _disabled_submodule->io.disabled_sb_rcv = io.sb_rcv;
        _disabled_sb_rdy = [this]() -> Bool {
            bool res = false;
            PhyState link_state_reg = static_cast<PhyState>(_link_state_reg->read().toBigUInt());
            SideBandMessage temp_disabled_sb_snd = static_cast<SideBandMessage>(_disabled_sb_snd().toBigUInt());
            switch (link_state_reg){
                case PhyState::reset:
                    if (temp_disabled_sb_snd != SideBandMessage::NOP) {
                        res = static_cast<bool>(io.sb_rdy());
                    }
                    break;
                case PhyState::active:
                    if (temp_disabled_sb_snd != SideBandMessage::NOP) {
                        res = static_cast<bool>(io.sb_rdy());
                    }
                    break;
                case PhyState::retrain:
                    if (temp_disabled_sb_snd != SideBandMessage::NOP) {
                        res = static_cast<bool>(io.sb_rdy());
                    }
                    break;
                case PhyState::linkReset:
                    if (temp_disabled_sb_snd != SideBandMessage::NOP) {
                        res = static_cast<bool>(io.sb_rdy());
                    }
                    break;
                default:
                    break;
            }
            return Bool(res);
        };
        _disabled_submodule->io.disabled_sb_rdy = _disabled_sb_rdy;

        // LinkReset submodule
        _linkreset_submodule->io.fdi_lp_state_req = io.fdi_lp_state_req;
        _linkreset_submodule->io.fdi_lp_state_req_prev = _fdi_lp_state_req_prev_reg;
        _linkreset_submodule->io.link_state = _link_state_reg;
        #define _linkreset_entry _linkreset_submodule->io.linkreset_entry
        // Intermediate sideband messgaes which gets assigned to top IO when required
        #define _linkreset_sb_snd _linkreset_submodule->io.linkreset_sb_snd
        _linkreset_submodule->io.linkreset_sb_rcv = io.sb_rcv;
        _linkreset_sb_rdy = [this]() -> Bool {
            bool res = false;
            if (_disabled_sb_snd().toBigUInt() != SideBandMessage::NOP) {
                return Bool(res);
            }

            PhyState link_state_reg = static_cast<PhyState>(_link_state_reg->read().toBigUInt());
            SideBandMessage temp_linkreset_sb_snd = static_cast<SideBandMessage>(_linkreset_sb_snd().toBigUInt());
            switch (link_state_reg){
                case PhyState::reset:
                    if (temp_linkreset_sb_snd != SideBandMessage::NOP) {
                        res = static_cast<bool>(io.sb_rdy());
                    }
                    break;
                case PhyState::active:
                    if (temp_linkreset_sb_snd != SideBandMessage::NOP) {
                        res = static_cast<bool>(io.sb_rdy());
                    }
                    break;
                case PhyState::retrain:
                    if (temp_linkreset_sb_snd != SideBandMessage::NOP) {
                        res = static_cast<bool>(io.sb_rdy());
                    }
                    break;
                case PhyState::linkReset:
                    if (temp_linkreset_sb_snd != SideBandMessage::NOP) {
                        res = static_cast<bool>(io.sb_rdy());
                    }
                    break;
                default:
                    break;
            }
            return Bool(res);
        };
        _linkreset_submodule->io.linkreset_sb_rdy = _linkreset_sb_rdy;

        // LinkInit submodule
        _linkinit_submodule->io.fdi_lp_state_req = io.fdi_lp_state_req;
        _linkinit_submodule->io.fdi_lp_state_req_prev = _fdi_lp_state_req_prev_reg;
        _linkinit_submodule->io.fdi_lp_rxactive_sts = io.fdi_lp_rx_active_sts;
        _linkinit_submodule->io.rdi_pl_state_sts = io.rdi_pl_state_sts;
        _linkinit_submodule->io.rdi_pl_inband_pres = io.rdi_pl_inband_pres;
        _linkinit_submodule->io.link_state = _link_state_reg;
        #define _linkinit_fdi_pl_rxactive_req _linkinit_submodule->io.linkinit_fdi_pl_rxactive_req
        #define _linkinit_fdi_pl_inband_pres _linkinit_submodule->io.linkinit_fdi_pl_inband_pres
        #define _linkinit_rdi_lp_state_req _linkinit_submodule->io.linkinit_rdi_lp_state_req
        #define _active_entry _linkinit_submodule->io.active_entry
        // Intermediate sideband messgaes which gets assigned to top IO when required
        #define _linkinit_sb_snd _linkinit_submodule->io.linkinit_sb_snd
        _linkinit_submodule->io.linkinit_sb_rcv = io.sb_rcv;
        _linkinit_sb_rdy = [this]() -> Bool {
            bool res = false;
            PhyState link_state_reg = static_cast<PhyState>(_link_state_reg->read().toBigUInt());
            if (link_state_reg == PhyState::reset) {
                if (_disabled_sb_snd().toBigUInt() == SideBandMessage::NOP
                    && _linkreset_sb_snd().toBigUInt() == SideBandMessage::NOP
                    && _linkinit_sb_snd().toBigUInt() != SideBandMessage::NOP) {
                    res = static_cast<bool>(io.sb_rdy());
                }
            }
            return Bool(res);
        };
        _linkinit_submodule->io.linkinit_sb_rdy = _linkinit_sb_rdy;

        // Parity negotiation submodule
        io.parity_tx_enable = _parity_negotiation_submodule->io.parity_tx_enable;
        io.parity_rx_enable = _parity_negotiation_submodule->io.parity_rx_enable;

        _parity_negotiation_submodule->io.start_negotiation = [this]() -> Bool {
            return Bool(_link_state_reg->read().toBigUInt() == PhyState::retrain);
        };
        #define _negotiation_complete _parity_negotiation_submodule->io.negotiation_complete
        _parity_negotiation_submodule->io.parity_sb_rcv = io.sb_rcv;
        #define _parity_negotiation_sb_snd _parity_negotiation_submodule->io.parity_sb_snd
        _parity_negotiation_sb_rdy = [this]() -> Bool {
            bool res = false;
            PhyState link_state_reg = static_cast<PhyState>(_link_state_reg->read().toBigUInt());
            if (link_state_reg == PhyState::retrain) {
                if (_disabled_sb_snd().toBigUInt() == SideBandMessage::NOP
                    && _linkreset_sb_snd().toBigUInt() == SideBandMessage::NOP
                    && _parity_negotiation_sb_snd().toBigUInt() != SideBandMessage::NOP) {
                    res = static_cast<bool>(io.sb_rdy());
                }
            }
            return Bool(res);
        };
        _parity_negotiation_submodule->io.parity_sb_rdy = _parity_negotiation_sb_rdy;
        _parity_negotiation_submodule->io.parity_rx_sw_en = io.parity_rx_sw_en;
        _parity_negotiation_submodule->io.parity_tx_sw_en = io.parity_tx_sw_en;
        _parity_negotiation_submodule->io.cycles_1us = io.cycles_1us;

        // FDI/RDI common state change triggers
        // LinkError logic
        // PHY informs the adapter over RDI that it is in linkError state
        _linkerror_phy_sts = [this]() -> Bool {
            return Bool(io.rdi_pl_state_sts().toBigUInt() == PhyState::linkError);
        };
        // Protocol initiates linkError through lp_linkerror assertion
        //val linkerror_fdi_req = io.fdi_lp_linkerror
        // Placeholder for any other internal request logic which can trigger linkError

        _stallhandler_handshake_done = [this]() -> Bool {
            return _linkmgmt_stallreq_reg->read() && io.linkmgmt_stalldone();
        };

        // rx_deactive and rx_active signals for checking if rx on mainband is disabled
        _rx_deactive = [this]() -> Bool {
            return !(io.fdi_lp_rx_active_sts()) & !(io.fdi_pl_rx_active_req());
        };
        _rx_active = [this]() -> Bool {
            return io.fdi_lp_rx_active_sts() & io.fdi_pl_rx_active_req();
        };

        // PHY informs the adapter over RDI that it should go into retrain
        _retrain_phy_sts = [this]() -> Bool {
            return Bool(io.rdi_pl_state_sts().toBigUInt() == PhyState::retrain);
        };

        _io_sb_snd = [this]() -> UInt {
            SideBandMessage res = SideBandMessage::NOP;
            PhyState link_state_reg = static_cast<PhyState>(_link_state_reg->read().toBigUInt());
            SideBandMessage temp_disabled_sb_snd = static_cast<SideBandMessage>(_disabled_sb_snd().toBigUInt());
            SideBandMessage temp_linkreset_sb_snd = static_cast<SideBandMessage>(_linkreset_sb_snd().toBigUInt());
            SideBandMessage temp_linkinit_sb_snd = static_cast<SideBandMessage>(_linkinit_sb_snd().toBigUInt());
            SideBandMessage temp_parity_negotiation_sb_snd = static_cast<SideBandMessage>(_parity_negotiation_sb_snd().toBigUInt());
            if (link_state_reg == PhyState::reset) {
                if (temp_disabled_sb_snd != SideBandMessage::NOP) {
                    res = temp_disabled_sb_snd;
                } else if (temp_linkreset_sb_snd != SideBandMessage::NOP) {
                    res = temp_linkreset_sb_snd;
                } else if (temp_linkinit_sb_snd != SideBandMessage::NOP) {
                    res = temp_linkinit_sb_snd;
                } else {
                    res = SideBandMessage::NOP;
                }
            } else if (link_state_reg == PhyState::active) {
                if (temp_disabled_sb_snd != SideBandMessage::NOP) {
                    res = temp_disabled_sb_snd;
                } else if (temp_linkreset_sb_snd != SideBandMessage::NOP) {
                    res = temp_linkreset_sb_snd;
                } else {
                    res = SideBandMessage::NOP;
                }
            } else if (link_state_reg == PhyState::retrain) {
                if (temp_disabled_sb_snd != SideBandMessage::NOP) {
                    res = temp_disabled_sb_snd;
                } else if (temp_linkreset_sb_snd != SideBandMessage::NOP) {
                    res = temp_linkreset_sb_snd;
                } else if (temp_parity_negotiation_sb_snd != SideBandMessage::NOP) {
                    res = temp_parity_negotiation_sb_snd;
                } else {
                    res = SideBandMessage::NOP;
                }
            } else if (link_state_reg == PhyState::linkReset) {
                if (temp_disabled_sb_snd != SideBandMessage::NOP) {
                    res = temp_disabled_sb_snd;
                } else {
                    res = SideBandMessage::NOP;
                }
            } else if (link_state_reg == PhyState::disabled ||
                       link_state_reg == PhyState::linkError) {
                res = SideBandMessage::NOP;
            } else {
                res = SideBandMessage::NOP;
            }
            return UInt(6, res);
        };
        io.sb_snd = _io_sb_snd;

        // ========= debug ============
        _debug_count_en = [this]() -> Bool {
            return Bool(true);
        };
        _debug_count = createSubmodule<Counter>("debug_count", _debug_count_en, 1024*1024);
    }

    void LinkManagementController::calcNextState() {
        ENTER_MODULE_FUNC();
        PhyState link_state_reg = static_cast<PhyState>(_link_state_reg->read().toBigUInt());

        *_fdi_lp_state_req_prev_reg = io.fdi_lp_state_req();

        // LinkError propagation from Protocol layer to PHY
        *_rdi_lp_linkerror_reg = io.fdi_lp_linkerror();

        // stall arbitration
        if (link_state_reg == PhyState::active) {
            *_linkmgmt_stallreq_reg = _linkreset_entry() || _disabled_entry() || _retrain_phy_sts();
        } else {
            *_linkmgmt_stallreq_reg = Bool(false);
        }

        // rxActive arbitration
        if (link_state_reg == PhyState::active) {
            if (_linkreset_entry() || _disabled_entry() || _retrain_phy_sts() || _linkerror_phy_sts()) {
                *_fdi_pl_rxactive_req_reg = Bool(false);
            } else {
                *_fdi_pl_rxactive_req_reg = Bool(true);
            }
        } else {
            if (_linkreset_entry() || _disabled_entry() || _linkerror_phy_sts()) {
                *_fdi_pl_rxactive_req_reg = Bool(false);
            } else {
                *_fdi_pl_rxactive_req_reg = _linkinit_fdi_pl_rxactive_req();
            }
        }

        // inband presence arbitration
        if (link_state_reg == PhyState::reset) {
            if (_linkerror_phy_sts()) {
                *_fdi_pl_inband_pres_reg = Bool(false);
            } else {
                *_fdi_pl_inband_pres_reg = _linkinit_fdi_pl_inband_pres();
            }
        } else if (link_state_reg == PhyState::linkError ||
                   link_state_reg == PhyState::disabled ||
                   link_state_reg == PhyState::linkReset) {
            *_fdi_pl_inband_pres_reg = Bool(false);
        } else {
            if (_linkerror_phy_sts()) {
                *_fdi_pl_inband_pres_reg = Bool(false);
            } else {
                *_fdi_pl_inband_pres_reg = Bool(true);
            }
        }

        // RDI lp state request generation logic
        if (link_state_reg == PhyState::reset) {
            *_rdi_lp_state_req_reg = _linkinit_rdi_lp_state_req();
        } else if (link_state_reg == PhyState::active) {
            if (_retrain_phy_sts()) {
                *_rdi_lp_state_req_reg = UInt(4, PhyStateReq::retrain);
            }
        } else if (link_state_reg == PhyState::retrain) {
            *_rdi_lp_state_req_reg = UInt(4, PhyStateReq::nop);
        } else if (link_state_reg == PhyState::linkError) {
            // Section 8.3.4.2 for link error exit
            if (io.fdi_lp_state_req().toBigUInt() == PhyStateReq::active
                && io.rdi_pl_state_sts().toBigUInt() == PhyState::linkError) {
                *_rdi_lp_state_req_reg = UInt(4, PhyStateReq::active);
            } else {
                *_rdi_lp_state_req_reg = UInt(4, PhyStateReq::nop);
            }
        } else if (link_state_reg == PhyState::disabled) {
            if (io.fdi_lp_state_req().toBigUInt() == PhyStateReq::active) {
                *_rdi_lp_state_req_reg = UInt(4, PhyStateReq::active);
            } else {
                *_rdi_lp_state_req_reg = UInt(4, PhyStateReq::disabled);
            }
        } else if (link_state_reg == PhyState::linkReset) {
            if (io.fdi_lp_state_req().toBigUInt() == PhyStateReq::active) {
                *_rdi_lp_state_req_reg = UInt(4, PhyStateReq::active);
            } else {
                *_rdi_lp_state_req_reg = UInt(4, PhyStateReq::linkReset);
            }
        }

        // FDI/RDI state machine. We use the same SM for optimized code as the spec
        // seems to trigger the state machines in tandem with no intermediate signalling
        switch (link_state_reg) {
            // RESET
            case PhyState::reset:
                if (_linkerror_phy_sts()) {
                    // TODO: any internal condition to trigger linkError? + SB msgs
                    *_link_state_reg = UInt(4, PhyState::linkError);
                } else if (_disabled_entry() && _rx_deactive()) {
                    *_link_state_reg = UInt(4, PhyState::disabled);
                } else if (_linkreset_entry() && _rx_deactive()) {
                    *_link_state_reg = UInt(4, PhyState::linkReset);
                } else if (_active_entry()) {
                    *_link_state_reg = UInt(4, PhyState::active);
                } else {
                    *_link_state_reg = _link_state_reg->read();
                }
                break;
            // ACTIVE
            case PhyState::active:
                if (_linkerror_phy_sts()) {
                    *_link_state_reg = UInt(4, PhyState::linkError);
                } else if (_disabled_entry() && _rx_deactive() && _stallhandler_handshake_done()) {
                    *_link_state_reg = UInt(4, PhyState::disabled);
                } else if (_linkreset_entry() && _rx_deactive() && _stallhandler_handshake_done()) {
                    *_link_state_reg = UInt(4, PhyState::linkReset);
                } else if (_retrain_phy_sts() && _rx_deactive() && _stallhandler_handshake_done()) {
                    *_link_state_reg = UInt(4, PhyState::retrain);
                } else {
                    *_link_state_reg = _link_state_reg->read();
                }
                break;
            // RETRAIN
            case PhyState::retrain:
                if (_linkerror_phy_sts()) {
                    *_link_state_reg = UInt(4, PhyState::linkError);
                } else if (_disabled_entry()) {
                    *_link_state_reg = UInt(4, PhyState::disabled);
                } else if (_linkreset_entry()) {
                    *_link_state_reg = UInt(4, PhyState::linkReset);
                } else {
                    *_link_state_reg = _link_state_reg->read();
                }
                break;
            // LINKERROR
            case PhyState::linkError:
                if ((io.fdi_lp_state_req().toBigUInt() == PhyStateReq::active ||
                    io.rdi_pl_state_sts().toBigUInt() == PhyState::linkError) &&
                    _rx_deactive()) {
                    *_link_state_reg = UInt(4, PhyState::reset);
                } else {
                    *_link_state_reg = _link_state_reg->read();
                }
                break;
            // DISABLED
            case PhyState::disabled:
                if (_linkerror_phy_sts()) {
                    *_link_state_reg = UInt(4, PhyState::linkError);
                } else if (io.fdi_lp_state_req().toBigUInt() == PhyStateReq::active ||
                           io.rdi_pl_state_sts().toBigUInt() == PhyState::reset) {
                    *_link_state_reg = UInt(4, PhyState::reset);
                } else {
                    *_link_state_reg = _link_state_reg->read();
                }
                break;
            // LINKRESET
            case PhyState::linkReset:
                if (_linkerror_phy_sts()) {
                    *_link_state_reg = UInt(4, PhyState::linkError);
                } else if (_disabled_entry() && _rx_deactive()) {
                    *_link_state_reg = UInt(4, PhyState::disabled);
                } else if (io.fdi_lp_state_req().toBigUInt() == PhyStateReq::active ||
                           io.rdi_pl_state_sts().toBigUInt() == PhyState::reset) {
                    *_link_state_reg = UInt(4, PhyState::reset);
                } else {
                    *_link_state_reg = _link_state_reg->read();
                }
                break;
            default:
                break;
        }

#if 0
        std::cout << getPathName() << ": link_state_reg " << _link_state_reg->read().toBigUInt() << std::endl;
        std::cout << getPathName() << ": io.sb_snd " << io.sb_snd().toBigUInt() << std::endl;
        std::cout << getPathName() << ": linkinit_rdi_lp_state_req " << _linkinit_rdi_lp_state_req().toBigUInt() << std::endl;
        std::cout << getPathName() << ": rdi_lp_state_req_reg " << _rdi_lp_state_req_reg->read().toBigUInt() << std::endl;
        std::cout << getPathName() << ": disabled_sb_snd " << _disabled_sb_snd().toBigUInt() << std::endl;
        std::cout << getPathName() << ": linkreset_sb_snd " << _linkreset_sb_snd().toBigUInt() << std::endl;
        std::cout << getPathName() << ": linkinit_sb_snd " << _linkinit_sb_snd().toBigUInt() << std::endl;
        std::cout << getPathName() << ": parity_negotiation_sb_snd " << _parity_negotiation_sb_snd().toBigUInt() << std::endl;
        std::cout << getPathName() << ": debug_count " << _debug_count->getCount().toBigUInt() << std::endl;
        std::cout << getPathName() << ": io.fdi_lp_state_req " << io.fdi_lp_state_req().toBigUInt() << std::endl;
        std::cout << getPathName() << ": io.fdi_lp_linkerror " << static_cast<bool>(io.fdi_lp_linkerror()) << std::endl;
        std::cout << getPathName() << ": io.fdi_lp_rx_active_sts " << static_cast<bool>(io.fdi_lp_rx_active_sts()) << std::endl;
        std::cout << getPathName() << ": io.fdi_pl_state_sts " << io.fdi_pl_state_sts().toBigUInt() << std::endl;
        std::cout << getPathName() << ": io.fdi_pl_rx_active_req " << static_cast<bool>(io.fdi_pl_rx_active_req()) << std::endl;
        std::cout << getPathName() << ": io.rdi_pl_inband_pres " << static_cast<bool>(io.rdi_pl_inband_pres()) << std::endl;
        std::cout << getPathName() << ": io.rdi_lp_linkerror " << static_cast<bool>(io.rdi_lp_linkerror()) << std::endl;
        std::cout << getPathName() << ": io.rdi_lp_state_req " << io.rdi_lp_state_req().toBigUInt() << std::endl;
        std::cout << getPathName() << ": io.rdi_pl_state_sts " << io.rdi_pl_state_sts().toBigUInt() << std::endl;
        std::cout << getPathName() << ": io.rdi_pl_inband_pres " << static_cast<bool>(io.rdi_pl_inband_pres()) << std::endl;
        std::cout << getPathName() << ": io.sb_snd " << io.sb_snd().toBigUInt() << std::endl;
        std::cout << getPathName() << ": io.sb_rcv " << io.sb_rcv().toBigUInt() << std::endl;
        std::cout << getPathName() << ": io.sb_rdy " << static_cast<bool>(io.sb_rdy()) << std::endl;
        std::cout << getPathName() << ": io.linkmgmt_stallreq " << static_cast<bool>(io.linkmgmt_stallreq()) << std::endl;
        std::cout << getPathName() << ": io.linkmgmt_stalldone " << static_cast<bool>(io.linkmgmt_stalldone()) << std::endl;
        std::cout << getPathName() << ": io.cycles_1us " << io.cycles_1us().toBigUInt() << std::endl;
        std::cout << getPathName() << ": io.parity_tx_sw_en " << static_cast<bool>(io.parity_tx_sw_en()) << std::endl;
        std::cout << getPathName() << ": io.parity_rx_sw_en " << static_cast<bool>(io.parity_rx_sw_en()) << std::endl;
        std::cout << getPathName() << ": io.parity_rx_enable " << static_cast<bool>(io.parity_rx_enable()) << std::endl;
        std::cout << getPathName() << ": io.parity_tx_enable " << static_cast<bool>(io.parity_tx_enable()) << std::endl;
        std::cout << std::endl;
#endif

    }
} // namespace CCPS