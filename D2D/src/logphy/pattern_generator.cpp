#include "logphy/pattern_generator.hpp"
#include "logphy/log_phy_types.hpp"

namespace CCPS {

    PatternGenerator::PatternGenerator(const AfeParams &afe_params, const SidebandParams &sb_params):
        _sb_params(sb_params)
    {
        assert (sb_params.sb_node_msg_width == 128);

        // Instantiate
        _write_in_progress = createReg<Bool>(Bool(false));
        _read_in_progress = createReg<Bool>(Bool(false));
        _pattern = createReg<UInt>(UInt(1, TransmitPattern::CLOCK_64_LOW_32));
        _sideband = createReg<Bool>(Bool(true));
        _timeout_cycles = createReg<UInt>(UInt(32, 0));
        _status = createReg<UInt>(UInt(1, BigUInt(MessageRequestStatusType::SUCCESS)));
        _status_valid = createReg<Bool>(Bool(false));
        _pattern_detected_count = createReg<UInt>(UInt(log2Ceil(128*2+1), 0));
        _pattern_written_count = createReg<UInt>(UInt(log2Ceil(2+1), 0));

        // connect
        _in_progress = [this] () -> Bool {
            return _write_in_progress->read() || _read_in_progress->read();
        };

        io.pattern_generator_io.transmit_req.ready = [this]() -> Bool {
            return Bool(_in_progress() == Bool(false));
        };
        io.pattern_generator_io.transmit_pattern_status.assignValid(_status_valid);
        io.pattern_generator_io.transmit_pattern_status.assignBits(_status);

        _pattern_to_transmit = [this, sb_params]() -> UInt {
            BigUInt res = 0;
            if (_write_in_progress->read()) {
                if (_pattern->read().toBigUInt() == TransmitPattern::CLOCK_64_LOW_32) {
                    res = getMaskedData(_clock_pattern_shift_reg_biguint, sb_params.sb_node_msg_width);
                }
            }
            return UInt(sb_params.sb_node_msg_width, res);
        };

        io.sideband_lane_io.tx_data.assignValid(_write_in_progress);
        io.sideband_lane_io.tx_data.assignBits(_pattern_to_transmit);
        io.sideband_lane_io.rx_data.assignReady(_read_in_progress);
    }

    void PatternGenerator::calcNextState() {
        if (io.pattern_generator_io.transmit_req.valid() && io.pattern_generator_io.transmit_req.ready()) {
            *_write_in_progress = Bool(true);
            *_read_in_progress = Bool(true);
            *_pattern = io.pattern_generator_io.transmit_req.pattern();
            *_sideband = io.pattern_generator_io.transmit_req.sideband();
            *_timeout_cycles = io.pattern_generator_io.transmit_req.timeout_cycles();
            *_status_valid = Bool(false);
        }

        if (io.pattern_generator_io.transmit_pattern_status.fire()) {
            *_status_valid = Bool(false);
        }

        TransmitPattern pattern = static_cast<TransmitPattern>(_pattern->read().toBigUInt());

        if (_in_progress()) {
            BigUInt timeout_cycles = _timeout_cycles->read().toBigUInt();
            *_timeout_cycles = UInt(32, timeout_cycles - 1);
            if (timeout_cycles == 0) {
                *_status = UInt(1, MessageRequestStatusType::ERR);
                *_status_valid = Bool(true);
                *_write_in_progress = Bool(false);
                *_read_in_progress = Bool(false);
                *_pattern_written_count = UInt(UInt(log2Ceil(2+1), 0));
                *_pattern_detected_count = UInt(log2Ceil(128*2+1), 0);
            } else if (_pattern_written_count->read().toBigUInt() >= _pattern_written_count_max[pattern]
                       && _pattern_detected_count->read().toBigUInt() >= _pattern_detected_count_max[pattern]) {
                *_status_valid = Bool(true);
                *_status = UInt(1, MessageRequestStatusType::SUCCESS);
                *_write_in_progress = Bool(false);
                *_read_in_progress = Bool(false);
                *_pattern_written_count = UInt(log2Ceil(2+1), 0);
                *_pattern_detected_count = UInt(log2Ceil(128*2+1), 0);
            }
        }

        if (_write_in_progress->read()) {
            if (pattern == TransmitPattern::CLOCK_64_LOW_32) {
                if (io.sideband_lane_io.tx_data.fire()) {
                    *_pattern_written_count = UInt(log2Ceil(2+1), _pattern_written_count->read().toBigUInt() + 1);
                }
            }
        }

        if (_read_in_progress->read()) {
            if (pattern == TransmitPattern::CLOCK_64_LOW_32) {
                if (io.sideband_lane_io.rx_data.fire()) {
                    if (io.sideband_lane_io.rx_data.bits().toBigUInt() == _pattern_to_detect) {
                        *_pattern_detected_count = UInt(log2Ceil(128*2+1), _pattern_detected_count->read().toBigUInt() + _sb_params.sb_node_msg_width);
                    }
                }
            }
        }

        //std::cout << getPathName() << ": io.pattern_generator_io.transmit_req.valid " << static_cast<bool>(io.pattern_generator_io.transmit_req.valid()) << std::endl;
        //std::cout << getPathName() << ": io.pattern_generator_io.transmit_req.ready " << static_cast<bool>(io.pattern_generator_io.transmit_req.ready()) << std::endl;
        //std::cout << getPathName() << ": io.pattern_generator_io.transmit_req.pattern " << io.pattern_generator_io.transmit_req.pattern().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.pattern_generator_io.transmit_req.timeout_cycles " << io.pattern_generator_io.transmit_req.timeout_cycles().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.pattern_generator_io.transmit_req.sideband " << static_cast<bool>(io.pattern_generator_io.transmit_req.sideband()) << std::endl;
        //std::cout << getPathName() << ": io.pattern_generator_io.transmit_pattern_status.valid " << static_cast<bool>(io.pattern_generator_io.transmit_pattern_status.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.pattern_generator_io.transmit_pattern_status.bits " << io.pattern_generator_io.transmit_pattern_status.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.pattern_generator_io.transmit_pattern_status.ready " << static_cast<bool>(io.pattern_generator_io.transmit_pattern_status.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.sideband_lane_io.tx_data.valid " << static_cast<bool>(io.sideband_lane_io.tx_data.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.sideband_lane_io.tx_data.ready " << static_cast<bool>(io.sideband_lane_io.tx_data.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.sideband_lane_io.tx_data.bits " << std::hex << io.sideband_lane_io.tx_data.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.sideband_lane_io.rx_data.valid " << static_cast<bool>(io.sideband_lane_io.rx_data.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.sideband_lane_io.rx_data.ready " << static_cast<bool>(io.sideband_lane_io.rx_data.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.sideband_lane_io.rx_data.bits " << std::hex << io.sideband_lane_io.rx_data.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _write_in_progress " << static_cast<bool>(_write_in_progress->read()) << std::endl;
        //std::cout << getPathName() << ": _read_in_progress " << static_cast<bool>(_read_in_progress->read()) << std::endl;
        //std::cout << getPathName() << ": _pattern " << _pattern->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _sideband " << static_cast<bool>(_sideband->read()) << std::endl;
        //std::cout << getPathName() << ": _timeout_cycles " << _timeout_cycles->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _status " << _status->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _status_valid " << static_cast<bool>(_status_valid->read()) << std::endl;
        //std::cout << getPathName() << ": _pattern_detected_count " << _pattern_detected_count->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _pattern_written_count " << _pattern_written_count->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _in_progress " << static_cast<bool>(_in_progress()) << std::endl;
        //std::cout << "=============================================" << std::endl;
    }
} // CCPS