#include "d2dadapter/parity_negotiation_submodule.hpp"
#include "interfaces/types.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"

namespace CCPS {

    ParityNegotiationSubmodule::ParityNegotiationSubmodule() {
        // Instantiate
        _parity_req_snt_flag_reg = createReg<Bool>(Bool(false));
        _parity_rsp_snt_flag_reg = createReg<Bool>(Bool(false));
        _parity_req_rcv_flag_reg = createReg<Bool>(Bool(false));
        _parity_rsp_rcv_flag_reg = createReg<Bool>(Bool(false));
        _parity_rx_enable_reg = createReg<Bool>(Bool(false));
        _parity_tx_enable_reg = createReg<Bool>(Bool(false));
        _parity_req_timeout_counter_reg = createReg<UInt>(UInt(32, 0));

        // Connect
        _io_negoriation_complete = [this] () -> Bool {
            return _reqcomplete() && _rspcomplete() && io.start_negotiation();
        };
        io.negotiation_complete = _io_negoriation_complete;

        _io_parity_sb_snd = [this] () -> UInt {
            BigUInt res = 0;
            if (io.start_negotiation()) {
                if (io.parity_tx_sw_en() && !_parity_req_snt_flag_reg->read()) {
                    res = SideBandMessage::PARITY_FEATURE_REQ;
                } else if (io.parity_rx_sw_en() && _parity_req_rcv_flag_reg->read() && !_parity_rsp_snt_flag_reg->read()) {
                    res = SideBandMessage::PARITY_FEATURE_ACK;
                } else if (!io.parity_rx_sw_en() && _parity_req_rcv_flag_reg->read() && !_parity_rsp_snt_flag_reg->read()) {
                    res = SideBandMessage::PARITY_FEATURE_NAK;
                } else {
                    res = SideBandMessage::NOP;
                }
            } else {
                res = SideBandMessage::NOP;
            }
            return UInt(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, res);
        };
        io.parity_sb_snd = _io_parity_sb_snd;

        _io_parity_rx_enable = [this] () -> Bool {
            return _parity_rx_enable_reg->read();
        };
        io.parity_rx_enable = _io_parity_rx_enable;

        _io_parity_tx_enable = [this] () -> Bool {
            return _parity_tx_enable_reg->read();
        };
        io.parity_tx_enable = _io_parity_tx_enable;

        _reqcomplete = [this] () -> Bool {
            return !io.parity_tx_sw_en() || _parity_rsp_rcv_flag_reg->read();
        };
        _rspcomplete = [this] () -> Bool {
            return Bool(_parity_req_timeout_counter_reg->read() == _timeout()) || _parity_rsp_snt_flag_reg->read();
        };
        _timeout = [this] () -> UInt {
            return UInt(32, io.cycles_1us().toBigUInt() << 2); // 8ms
        };
    }

    void ParityNegotiationSubmodule::calcNextState() {

        if (io.start_negotiation()) {
            if (!_parity_req_rcv_flag_reg->read()) {
                *_parity_req_timeout_counter_reg = _parity_req_timeout_counter_reg->read() + UInt(1, 1);
            } else if (_parity_req_timeout_counter_reg->read() == _timeout()) { // 8us
                *_parity_req_timeout_counter_reg = _parity_req_timeout_counter_reg->read();
            } else {
                *_parity_req_timeout_counter_reg = UInt(32, 0);
            }

            if (io.parity_sb_snd().toBigUInt() == SideBandMessage::PARITY_FEATURE_REQ && static_cast<bool>(io.parity_sb_rdy())) {
                *_parity_req_snt_flag_reg = Bool(true);
            } else {
                *_parity_req_snt_flag_reg = _parity_req_snt_flag_reg->read();
            }

            if ((io.parity_sb_snd().toBigUInt() == SideBandMessage::PARITY_FEATURE_ACK || io.parity_sb_snd().toBigUInt() == SideBandMessage::PARITY_FEATURE_NAK) && static_cast<bool>(io.parity_sb_rdy())) {
                *_parity_rsp_snt_flag_reg = Bool(true);
            } else {
                *_parity_rsp_snt_flag_reg = _parity_rsp_snt_flag_reg->read();
            }

            if (io.parity_sb_rcv().toBigUInt() == SideBandMessage::PARITY_FEATURE_REQ) {
                *_parity_req_rcv_flag_reg = Bool(true);
            } else {
                *_parity_req_rcv_flag_reg = _parity_req_rcv_flag_reg->read();
            }

            if (io.parity_sb_rcv().toBigUInt() == SideBandMessage::PARITY_FEATURE_ACK || io.parity_sb_rcv().toBigUInt() == SideBandMessage::PARITY_FEATURE_NAK) {
                *_parity_rsp_rcv_flag_reg = Bool(true);
            } else {
                *_parity_rsp_rcv_flag_reg = _parity_rsp_rcv_flag_reg->read();
            }

            if (io.parity_sb_snd().toBigUInt() == SideBandMessage::PARITY_FEATURE_ACK && static_cast<bool>(io.parity_sb_rdy())) {
                *_parity_rx_enable_reg = Bool(true);
            } else if (io.parity_sb_snd().toBigUInt() == SideBandMessage::PARITY_FEATURE_NAK && static_cast<bool>(io.parity_sb_rdy())) {
                *_parity_rx_enable_reg = Bool(false);
            } else {
                *_parity_rsp_snt_flag_reg = _parity_rsp_snt_flag_reg->read();
            }

            if (io.parity_sb_rcv().toBigUInt() == SideBandMessage::PARITY_FEATURE_ACK) {
                *_parity_tx_enable_reg = Bool(true);
            } else if (io.parity_sb_rcv().toBigUInt() == SideBandMessage::PARITY_FEATURE_NAK || !static_cast<bool>(io.parity_tx_sw_en())) {
                *_parity_tx_enable_reg = Bool(false);
            } else {
                *_parity_tx_enable_reg = _parity_tx_enable_reg->read();
            }
        } else {
            *_parity_req_timeout_counter_reg = UInt(32, 0);
            *_parity_rsp_snt_flag_reg = Bool(false);
            *_parity_req_snt_flag_reg = Bool(false);
            *_parity_req_rcv_flag_reg = Bool(false);
            *_parity_rsp_rcv_flag_reg = Bool(false);
            *_parity_rx_enable_reg = _parity_rx_enable_reg->read();
            *_parity_tx_enable_reg = _parity_tx_enable_reg->read();
        }

        //std::cout << getPathName() << ": " << " io.start_negotiation:           " << static_cast<bool>(io.start_negotiation()) << std::endl;
        //std::cout << getPathName() << ": " << " io.negotiation_complete:        " << static_cast<bool>(io.negotiation_complete()) << std::endl;
        //std::cout << getPathName() << ": " << " io.cycles_1us:                  " << io.cycles_1us().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": " << " io.parity_tx_sw_en:             " << static_cast<bool>(io.parity_tx_sw_en()) << std::endl;
        //std::cout << getPathName() << ": " << " io.parity_rx_sw_en:             " << static_cast<bool>(io.parity_rx_sw_en()) << std::endl;
        //std::cout << getPathName() << ": " << " io.parity_rx_enable:            " << static_cast<bool>(io.parity_rx_enable()) << std::endl;
        //std::cout << getPathName() << ": " << " io.parity_tx_enable:            " << static_cast<bool>(io.parity_tx_enable()) << std::endl;
        //std::cout << getPathName() << ": " << " io.parity_sb_rcv:               " << io.parity_sb_rcv().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": " << " io.parity_sb_rdy:               " << static_cast<bool>(io.parity_sb_rdy()) << std::endl;
        //std::cout << getPathName() << ": " << " io.parity_sb_snd:               " << io.parity_sb_snd().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": " << " parity_req_snt_flag_reg:        " << static_cast<bool>(_parity_req_snt_flag_reg->read()) << std::endl;
        //std::cout << getPathName() << ": " << " parity_req_rcv_flag_reg:        " << static_cast<bool>(_parity_req_rcv_flag_reg->read()) << std::endl;
        //std::cout << getPathName() << ": " << " parity_rsp_snt_flag_reg:        " << static_cast<bool>(_parity_rsp_snt_flag_reg->read()) << std::endl;
        //std::cout << getPathName() << ": " << " parity_req_timeout_counter_reg: " << _parity_req_timeout_counter_reg->read().toBigUInt() << std::endl;
    }
} // namespace CCPS