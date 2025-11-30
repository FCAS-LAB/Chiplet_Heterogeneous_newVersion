#include "logphy/mb_init_fsm.hpp"

namespace CCPS {

    MBInitFSM::MBInitFSM(const LinkTrainingParams &link_training_params, const AfeParams &afe_params):
        _sb_clock_freq(link_training_params.sb_clock_freq_analog / afe_params.sb_serializer_ratio) {

        // Instantiate
        _state = createReg<UInt>(UInt(3, State::PARAM));
        _param_sub_state = createReg<UInt>(UInt(2, ParamSubState::SEND_REQ));

        _voltage_swing = createReg<UInt>(UInt(5, link_training_params.mb_training_params.voltage_swing));
        _max_data_rate = createReg<UInt>(UInt(4, link_training_params.mb_training_params.maximum_data_rate));
        _clock_mode = createReg<UInt>(UInt(1, link_training_params.mb_training_params.clock_mode));
        _clock_phase = createReg<Bool>(Bool(link_training_params.mb_training_params.clock_phase));
        _module_id = createReg<UInt>(UInt(2, link_training_params.mb_training_params.module_id));
        _CCPS_ax32 = createReg<Bool>(Bool(link_training_params.mb_training_params.CCPS_ax32));

        _req_data = createReg<UInt>(UInt(64, 0));

        // connect
        io.transition = [this]() -> Bool {
            bool res = _next_state().toBigUInt() == State::IDLE ||
                       _next_state().toBigUInt() == State::ERR;
            return Bool(res);
        };
        io.error = [this]() -> Bool {
            return Bool(_state->read().toBigUInt() == State::ERR);
        };

        io.sb_train_io.msg_req.valid = [&]() -> Bool {
            bool res = false;
            State state = static_cast<State>(_state->read().toBigUInt());
            ParamSubState param_sub_state = static_cast<ParamSubState>(_param_sub_state->read().toBigUInt());
            if (state == State::PARAM) {
                if (param_sub_state == ParamSubState::SEND_REQ) {
                    res = true;
                } else if (param_sub_state == ParamSubState::SEND_RESP) {
                    res = true;
                }
            }
            return Bool(res);
        };
        io.sb_train_io.msg_req.msg = [&]() -> UInt {
            State state = static_cast<State>(_state->read().toBigUInt());
            ParamSubState param_sub_state = static_cast<ParamSubState>(_param_sub_state->read().toBigUInt());
            if (state == State::PARAM) {
                if (param_sub_state == ParamSubState::SEND_REQ) {
                    return formParamsReqMsg_Msg(
                        true,
                        _voltage_swing->read(),
                        _max_data_rate->read(),
                        ClockModeParam(_clock_mode->read().toBigUInt()),
                        _clock_phase->read(),
                        _module_id->read(),
                        _CCPS_ax32->read()
                    );
                } else if (param_sub_state == ParamSubState::SEND_RESP) {
                    UInt req_data = _req_data->read();
                    return formParamsReqMsg_Msg(
                        false,
                        UInt(5, 0),
                        _exchange_max_data_rate(),
                        static_cast<ClockModeParam>(req_data(9).toBigUInt()),
                        Bool(req_data(10).toBigUInt() == 1),
                        UInt(2, 0),
                        Bool(false)
                    );
                }
            }
            return UInt(128, 0);
        };

        io.sb_train_io.msg_req.timeout_cycles = [this]() -> UInt {
            return UInt(64, int(0.008 * _sb_clock_freq));
        };

        io.sb_train_io.msg_req_status.ready = [&]() -> Bool {
            bool res = false;
            State state = static_cast<State>(_state->read().toBigUInt());
            ParamSubState param_sub_state = static_cast<ParamSubState>(_param_sub_state->read().toBigUInt());
            if (state == State::PARAM) {
                if (param_sub_state == ParamSubState::WAIT_REQ) {
                    res = true;
                } else if (param_sub_state == ParamSubState::WAIT_RESP) {
                    res = true;
                }
            }
            return Bool(res);
        };
        io.pattern_generator_io.transmit_req.valid.capture(false);
        io.pattern_generator_io.transmit_req.pattern.capture(1, 0);
        io.pattern_generator_io.transmit_req.timeout_cycles.capture(32, 0);
        io.pattern_generator_io.transmit_req.sideband.capture(false);
        io.pattern_generator_io.transmit_pattern_status.assignReady(false);

        _next_state = [this]() -> UInt {
            State state = static_cast<State>(_state->read().toBigUInt());
            ParamSubState param_sub_state = static_cast<ParamSubState>(_param_sub_state->read().toBigUInt());
            State next_state = static_cast<State>(state);

            if (state == State::PARAM) {
                if (param_sub_state == ParamSubState::WAIT_REQ) {
                    if (io.sb_train_io.msg_req_status.valid() &&
                        io.sb_train_io.msg_req_status.ready()) {
                        if (io.sb_train_io.msg_req_status.status().toBigUInt() == MessageRequestStatusType::ERR) {
                            next_state = State::ERR;
                        }
                    }
                } else if (param_sub_state == ParamSubState::WAIT_RESP) {
                    if (io.sb_train_io.msg_req_status.valid() &&
                        io.sb_train_io.msg_req_status.ready()) {
                        if (io.sb_train_io.msg_req_status.status().toBigUInt() == MessageRequestStatusType::ERR) {
                            next_state = State::ERR;
                        } else {
                            next_state = State::IDLE;
                        }
                    }
                }
            }
            return UInt(3, next_state);
        };

        _exchange_max_data_rate = [this] () -> UInt {
            UInt req_data = _req_data->read()(3, 0);
            UInt max_data_rate = _max_data_rate->read();
            if (max_data_rate >= req_data) {
                return req_data;
            }
            return max_data_rate;
        };
    }

    void MBInitFSM::calcNextState() {
        State state = static_cast<State>(_state->read().toBigUInt());
        ParamSubState param_sub_state = static_cast<ParamSubState>(_param_sub_state->read().toBigUInt());
        State next_state = static_cast<State>(_next_state().toBigUInt());

        if (next_state == State::PARAM && state != State::PARAM) {
            *_param_sub_state = UInt(2, ParamSubState::SEND_REQ);
        }

        *_state = _next_state();

        if (state == State::PARAM) {
            if (param_sub_state == ParamSubState::SEND_REQ) {
                if (io.sb_train_io.msg_req.valid() &&
                    io.sb_train_io.msg_req.ready()) {
                    *_param_sub_state = UInt(2, ParamSubState::WAIT_REQ);
                }
            } else if (param_sub_state == ParamSubState::WAIT_REQ) {
                if (io.sb_train_io.msg_req_status.valid() &&
                    io.sb_train_io.msg_req_status.ready()) {
                    *_req_data = io.sb_train_io.msg_req_status.data();
                    if (io.sb_train_io.msg_req_status.status().toBigUInt() != MessageRequestStatusType::ERR) {
                        *_param_sub_state = UInt(2, ParamSubState::SEND_RESP);
                    }
                }
            } else if (param_sub_state == ParamSubState::SEND_RESP) {
                *_max_data_rate = _exchange_max_data_rate();
                if (io.sb_train_io.msg_req.valid() &&
                    io.sb_train_io.msg_req.ready()) {
                    *_param_sub_state = UInt(2, ParamSubState::WAIT_RESP);
                }
            }
        }

        // This module needs reset
        RegModule::calcNextState();

        //std::cout << getPathName() << ": state " << _state->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": next_state " << _next_state().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": param_sub_state " << _param_sub_state->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": voltage_swing " << _voltage_swing->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": max_data_rate " << _max_data_rate->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": clock_mode " << _clock_mode->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": clock_phase " << _clock_phase->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": module_id " << _module_id->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": CCPS_ax32 " << _CCPS_ax32->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.transition " << io.transition().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.error " << io.error().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.sb_train_io.msg_req.valid " << io.sb_train_io.msg_req.valid().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.sb_train_io.msg_req.ready " << io.sb_train_io.msg_req.ready().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.sb_train_io.msg_req.msg " << std::hex << io.sb_train_io.msg_req.msg().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.sb_train_io.msg_req.timeout_cycles " << std::hex << io.sb_train_io.msg_req.timeout_cycles().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.sb_train_io.msg_req_status.valid " << io.sb_train_io.msg_req_status.valid().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.sb_train_io.msg_req_status.ready " << io.sb_train_io.msg_req_status.ready().toBigUInt() << std::endl;
        //std::cout << "===================================================" << std::endl;
    }
} // namespace CCPS