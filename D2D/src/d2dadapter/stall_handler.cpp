#include "d2dadapter/stall_handler.hpp"
#include "interfaces/types.hpp"

namespace CCPS {
    // ========================== FDIStallHandler ===============================
    FDIStallHandler::FDIStallHandler() {
        // Instantiate
        _fdi_lp_stallreq_reg = createReg<Bool>(Bool(false));
        _linkmgmt_stalldone_reg = createReg<Bool>(Bool(false));
        _stall_handshake_state_reg = createReg<UInt>(UInt(2, StallHandshakeState::IDLE));

        // Connect
        _io_fdi_pl_stallreq = [this] () -> Bool {
            return _fdi_lp_stallreq_reg->read();
        };
        io.fdi_pl_stallreq = _io_fdi_pl_stallreq;

        _io_linkmgmt_stalldone = [this] () -> Bool {
            return _linkmgmt_stalldone_reg->read();
        };
        io.linkmgmt_stalldone = _io_linkmgmt_stalldone;
    }

    // ========================== FDIStallHandler ===============================
    void FDIStallHandler::calcNextState() {
        unsigned stall_handshake_state_reg = static_cast<unsigned>(_stall_handshake_state_reg->read().toBigUInt());
        switch (stall_handshake_state_reg) {
            case StallHandshakeState::IDLE:
                if (io.linkmgmt_stallreq() && !io.fdi_lp_stallack()) {
                    *_fdi_lp_stallreq_reg = Bool(true);
                    *_linkmgmt_stalldone_reg = Bool(false);
                    *_stall_handshake_state_reg = UInt(2, StallHandshakeState::REQSNT);
                } else {
                    *_fdi_lp_stallreq_reg = Bool(false);
                    *_linkmgmt_stalldone_reg = Bool(false);
                    *_stall_handshake_state_reg = _stall_handshake_state_reg->read();
                }
                break;
            case StallHandshakeState::REQSNT:
                if (io.fdi_lp_stallack()) {
                    *_fdi_lp_stallreq_reg = Bool(false);
                    *_linkmgmt_stalldone_reg = Bool(false);
                    *_stall_handshake_state_reg = UInt(2, StallHandshakeState::REQFALL);
                } else {
                    *_fdi_lp_stallreq_reg = Bool(true);
                    *_linkmgmt_stalldone_reg = Bool(false);
                    *_stall_handshake_state_reg = _stall_handshake_state_reg->read();
                }
                break;
            case StallHandshakeState::REQFALL:
                if (!io.fdi_lp_stallack()) {
                    *_fdi_lp_stallreq_reg = Bool(false);
                    *_linkmgmt_stalldone_reg = Bool(true);
                    *_stall_handshake_state_reg = UInt(2, StallHandshakeState::COMPLETE);
                } else {
                    *_fdi_lp_stallreq_reg = Bool(false);
                    *_linkmgmt_stalldone_reg = Bool(false);
                    *_stall_handshake_state_reg = _stall_handshake_state_reg->read();
                }
                break;
            case StallHandshakeState::COMPLETE:
                if (!io.linkmgmt_stallreq()) {
                    *_fdi_lp_stallreq_reg = Bool(false);
                    *_linkmgmt_stalldone_reg = Bool(false);
                    *_stall_handshake_state_reg = UInt(2, StallHandshakeState::IDLE);
                } else {
                    *_fdi_lp_stallreq_reg = Bool(false);
                    *_linkmgmt_stalldone_reg = Bool(true);
                    *_stall_handshake_state_reg = _stall_handshake_state_reg->read();
                }
                break;
            default:
                break;
        }
#if 0
        std::cout << getPathName() << "io.linkmgmt_stallreq " << static_cast<bool>(io.linkmgmt_stallreq()) << std::endl;
        std::cout << getPathName() << "io.linkmgmt_stalldone " << static_cast<bool>(io.linkmgmt_stalldone()) << std::endl;
        std::cout << getPathName() << "io.fdi_lp_stallack " << static_cast<bool>(io.fdi_lp_stallack()) << std::endl;
        std::cout << getPathName() << "io.fdi_pl_stallreq " << static_cast<bool>(io.fdi_pl_stallreq()) << std::endl;
        std::cout << std::endl;
#endif
    }

    // ========================== RDIStallHandler ===============================
    RDIStallHandler::RDIStallHandler() {
        // Instantiate
        _rdi_lp_stallack_reg = createReg<Bool>(Bool(false));
        _mainband_stallreq_reg = createReg<Bool>(Bool(false));
        _stall_handshake_state_reg = createReg<UInt>(UInt(2, StallHandshakeState::IDLE));

        // Connect
        _io_mainband_stallreq = [this] () -> Bool {
            return _mainband_stallreq_reg->read();
        };
        io.mainband_stallreq = _io_mainband_stallreq;

        _io_rdi_lp_stallack = [this] () -> Bool {
            return _rdi_lp_stallack_reg->read();
        };
        io.rdi_lp_stallack = _io_rdi_lp_stallack;
    }

    // ========================== RDIStallHandler ===============================
    void RDIStallHandler::calcNextState() {
        unsigned stall_handshake_state_reg = static_cast<unsigned>(_stall_handshake_state_reg->read().toBigUInt());
        switch (stall_handshake_state_reg) {
            case StallHandshakeState::IDLE:
                if (io.rdi_pl_stallreq()) {
                    *_mainband_stallreq_reg = Bool(true);
                    *_rdi_lp_stallack_reg = Bool(false);
                    *_stall_handshake_state_reg = UInt(2, StallHandshakeState::REQSNT);
                } else {
                    *_mainband_stallreq_reg = Bool(false);
                    *_rdi_lp_stallack_reg = Bool(false);
                    *_stall_handshake_state_reg = _stall_handshake_state_reg->read();
                }
                break;
            case StallHandshakeState::REQSNT:
                if (io.mainband_stalldone()) {
                    *_mainband_stallreq_reg = Bool(true);
                    *_rdi_lp_stallack_reg = Bool(true);
                    *_stall_handshake_state_reg = UInt(2, StallHandshakeState::REQFALL);
                } else {
                    *_mainband_stallreq_reg = Bool(true);
                    *_rdi_lp_stallack_reg = Bool(false);
                    *_stall_handshake_state_reg = _stall_handshake_state_reg->read();
                }
                break;
            case StallHandshakeState::REQFALL:
                if (!io.rdi_pl_stallreq()) {
                    *_mainband_stallreq_reg = Bool(false);
                    *_rdi_lp_stallack_reg = Bool(false);
                    *_stall_handshake_state_reg = UInt(2, StallHandshakeState::IDLE);
                } else {
                    *_mainband_stallreq_reg = Bool(true);
                    *_rdi_lp_stallack_reg = Bool(true);
                    *_stall_handshake_state_reg = _stall_handshake_state_reg->read();
                }
                break;
            default:
                break;
        }
#if 0
        std::cout << getPathName() << ": " << ", io.mainband_stallreq           " << static_cast<bool>(io.mainband_stallreq()) << std::endl;
        std::cout << getPathName() << ": " << ", io.mainband_stalldone          " << static_cast<bool>(io.mainband_stalldone()) << std::endl;
        std::cout << getPathName() << ": " << ", io.rdi_pl_stallreq             " << static_cast<bool>(io.rdi_pl_stallreq()) << std::endl;
        std::cout << getPathName() << ": " << ", io.rdi_lp_stallack             " << static_cast<bool>(io.rdi_lp_stallack()) << std::endl;
        std::cout << getPathName() << ": " << ", mainband_stallreq_reg          " << static_cast<bool>(_mainband_stallreq_reg->read()) << std::endl;
        std::cout << getPathName() << ": " << ", rdi_lp_stallack_reg            " << static_cast<bool>(_rdi_lp_stallack_reg->read()) << std::endl;
        std::cout << getPathName() << ": " << ", stall_handshake_state_reg      " << static_cast<unsigned>(_stall_handshake_state_reg->read()) << std::endl;
        std::cout << std::endl;
#endif
    }
} // namespace CCPS