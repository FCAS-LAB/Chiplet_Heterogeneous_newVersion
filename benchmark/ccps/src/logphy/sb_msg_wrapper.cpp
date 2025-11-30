#include "logphy/sb_msg_wrapper.hpp"

namespace CCPS {

    SBMsgWrapper::SBMsgWrapper(const SidebandParams &sb_params) {
        // Instantiate
        _current_state = createReg<UInt>(UInt(2, State::IDLE));
        _timeout_counter = createReg<UInt>(UInt(64, 0));
        _sent_msg = createReg<Bool>(Bool(false));
        _received_msg = createReg<Bool>(Bool(false));
        _current_req = createReg<UInt>(UInt(128, 0));
        _current_req_timeout_max = createReg<UInt>(UInt(64, 0));
        _current_status = createReg<UInt>(UInt(1, BigUInt(MessageRequestStatusType::ERR)));
        _data_out = createReg<UInt>(UInt(64, 0));

        // Connect
        _next_state = [this] () -> UInt {
            State state = static_cast<State>(_current_state->read().toBigUInt());
            State res = state;
            if (state == State::IDLE) {
                if (io.train_io.msg_req.valid() && io.train_io.msg_req.ready()) {
                    res = State::EXCHANGE;
                }
            } else if (state == State::EXCHANGE) {
                if (_has_received_msg() && _has_sent_msg()) {
                    res = State::WAIT_ACK;
                }
                if (_timeout_counter->read().toBigUInt() == _current_req_timeout_max->read().toBigUInt()) {
                    res = State::WAIT_ACK;
                }
            } else if (state == State::WAIT_ACK) {
                if (io.train_io.msg_req_status.valid() && io.train_io.msg_req_status.ready()) {
                    res = State::IDLE;
                }
            }
            return UInt(2, res);
        };

        io.train_io.msg_req_status.data = _data_out;
        io.train_io.msg_req_status.status = _current_status;
        io.lane_io.rx_data.assignReady(
            [this] () -> Bool {
                bool res = false;
                if (_current_state->read().toBigUInt() == State::EXCHANGE) {
                    res = true;
                }
                return Bool(res);
            }
        );
        io.lane_io.tx_data.assignValid(
            [this] () -> Bool {
                bool res = false;
                if (_current_state->read().toBigUInt() == State::EXCHANGE) {
                    res = true;
                }
                return Bool(res);
            }
        );
        io.lane_io.tx_data.assignBits(_current_req);
        io.train_io.msg_req_status.valid = [this]() -> Bool {
            return Bool(_current_state->read().toBigUInt() == State::WAIT_ACK);
        };
        io.train_io.msg_req.ready = [&]() -> Bool {
            return Bool(_current_state->read().toBigUInt() == State::IDLE);
        };

        _has_sent_msg = [this]() -> Bool {
            return Bool(io.lane_io.tx_data.fire() || _sent_msg->read());
        };
        _just_received_msg = [this]() -> Bool {
            return Bool(io.lane_io.rx_data.fire() &&
                        messageIsEqual(io.lane_io.rx_data.bits()(64, 0), _current_req->read()(64, 0)));
        };
        _has_received_msg = [this]() -> Bool {
            return Bool(_just_received_msg() || _received_msg->read());
        };
    }

    void SBMsgWrapper::calcNextState() {
        *_current_state = _next_state();

        State current_state = static_cast<State>(_current_state->read().toBigUInt());
        State next_state = static_cast<State>(_next_state().toBigUInt());

        if (current_state != next_state) {
            *_timeout_counter = UInt(64, 0);
            *_sent_msg = Bool(false);
            *_received_msg = Bool(false);
        }

        if (current_state == State::IDLE) {
            if (io.train_io.msg_req.valid() && io.train_io.msg_req.ready()) {
                *_current_req = io.train_io.msg_req.msg();
                *_current_req_timeout_max = io.train_io.msg_req.timeout_cycles();
            }
        } else if (current_state == State::EXCHANGE) {
            *_sent_msg = _has_sent_msg();
            *_received_msg = _has_received_msg();

            if (_has_received_msg() && _has_sent_msg()) {
                _data_out->write(io.lane_io.rx_data.bits()(127, 64));
                _current_status->write(UInt(1, BigUInt(MessageRequestStatusType::SUCCESS)));
            }

            BigUInt timeout_counter = _timeout_counter->read().toBigUInt();
            *_timeout_counter = UInt(64, timeout_counter + 1);
            if (timeout_counter == _current_req_timeout_max->read().toBigUInt()) {
                _current_status->write(UInt(1, BigUInt(MessageRequestStatusType::ERR)));
            }
        }

        //std::cout << getPathName() << ": io.train_io.msg_req.valid " << static_cast<bool>(io.train_io.msg_req.valid()) << std::endl;
        //std::cout << getPathName() << ": io.train_io.msg_req.ready " << static_cast<bool>(io.train_io.msg_req.ready()) << std::endl;
        //std::cout << getPathName() << ": io.train_io.msg_req.msg " << std::hex << io.train_io.msg_req.msg().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.train_io.msg_req.timeout_cycles " << io.train_io.msg_req.timeout_cycles().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.train_io.msg_req_status.valid " << static_cast<bool>(io.train_io.msg_req_status.valid()) << std::endl;
        //std::cout << getPathName() << ": io.train_io.msg_req_status.ready " << static_cast<bool>(io.train_io.msg_req_status.ready()) << std::endl;
        //std::cout << getPathName() << ": io.train_io.msg_req_status.status " << io.train_io.msg_req_status.status().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.train_io.msg_req_status.data " << io.train_io.msg_req_status.data().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.lane_io.rx_data.isReady " << static_cast<bool>(io.lane_io.rx_data.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.lane_io.rx_data.isValid " << static_cast<bool>(io.lane_io.rx_data.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.lane_io.rx_data.bits " << std::hex << io.lane_io.rx_data.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.lane_io.tx_data.isReady " << static_cast<bool>(io.lane_io.tx_data.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.lane_io.tx_data.isValid " << static_cast<bool>(io.lane_io.tx_data.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.lane_io.tx_data.bits " << std::hex << io.lane_io.tx_data.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _current_state " << static_cast<int>(_current_state->read().toBigUInt()) << std::endl;
        //std::cout << getPathName() << ": _next_state " << static_cast<int>(_next_state().toBigUInt()) << std::endl;
        //std::cout << getPathName() << ": _timeout_counter " << _timeout_counter->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _sent_msg " << static_cast<bool>(_sent_msg->read()) << std::endl;
        //std::cout << getPathName() << ": _received_msg " << static_cast<bool>(_received_msg->read()) << std::endl;
        //std::cout << getPathName() << ": _current_req " << _current_req->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _current_req_timeout_max " << _current_req_timeout_max->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _current_status " << _current_status->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _data_out " << _data_out->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _has_sent_msg " << static_cast<bool>(_has_sent_msg()) << std::endl;
        //std::cout << getPathName() << ": _just_received_msg " << static_cast<bool>(_just_received_msg()) << std::endl;
        //std::cout << getPathName() << ": _has_received_msg " << static_cast<bool>(_has_received_msg()) << std::endl;
        //std::cout << "========================================" << std::endl;
    }
} // namespace CCPS