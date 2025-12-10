#include "logphy/link_training_fsm.hpp"
#include "sideband/sb_msg_encoding.hpp"

namespace CCPS {

    LinkTrainingFSM::LinkTrainingFSM(
        const LinkTrainingParams &link_training_params,
        const SidebandParams &sb_params,
        const AfeParams &afe_params
    ):
        _sb_clock_freq(link_training_params.sb_clock_freq_analog / afe_params.sb_serializer_ratio),
        _link_training_params(link_training_params)
    {
        // Instantiate
        _pattern_generator = createSubmodule<PatternGenerator>("pattern_generator", afe_params, sb_params);
        _sb_msg_wrapper = createSubmodule<SBMsgWrapper>("sb_msg_wrapper", sb_params);
        _mb_init = createSubmodule<MBInitFSM>("mb_init_fsm", link_training_params, afe_params);
        _rdi_bringup = createSubmodule<RdiBringup>("rdi_bringup");
        _counter = createSubmodule<Counter>("counter", _counter_inc, link_training_params.pll_wait_time - 1);

        _current_state = createReg<UInt>(UInt(3, LinkTrainingState::reset));
        _reset_sub_state = createReg<UInt>(UInt(2, ResetSubState::INIT));
        _sb_init_sub_state = createReg<UInt>(UInt(3, SBInitSubState::SEND_CLOCK));
        _active_sub_state = createReg<UInt>(UInt(1, ActiveSubState::IDLE));

        _debug_count = createReg<UInt>(UInt(32, 0));

        // connect
        _msg_source = [this]() -> UInt {
            BigUInt val = 0;
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::SEND_CLOCK) {
                    val = MsgSource::PATTERN_GENERATOR;
                } else if (sb_init_sub_state == SBInitSubState::WAIT_CLOCK) {
                    val = MsgSource::PATTERN_GENERATOR;
                } else if (sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_EXCH) {
                    val = MsgSource::SB_MSG_WRAPPER;
                } else if (sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_WAIT) {
                    val = MsgSource::SB_MSG_WRAPPER;
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_REQ) {
                    val = MsgSource::SB_MSG_WRAPPER;
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_REQ_WAIT) {
                    val = MsgSource::SB_MSG_WRAPPER;
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_RESP) {
                    val = MsgSource::SB_MSG_WRAPPER;
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_RESP_WAIT) {
                    val = MsgSource::SB_MSG_WRAPPER;
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                val = MsgSource::SB_MSG_WRAPPER;
            } else if (current_state == LinkTrainingState::linkInit) {
                val = MsgSource::SB_MSG_WRAPPER;
            }
            return UInt(3, val);
        };

        _pattern_generator->io.pattern_generator_io.transmit_req.valid = [this]() -> Bool {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());

            if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::SEND_CLOCK) {
                    return Bool(true);
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                return _mb_init->io.pattern_generator_io.transmit_req.valid();
            }
            return Bool(false);
        };

        _pattern_generator->io.pattern_generator_io.transmit_req.pattern = [this]() -> UInt {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::SEND_CLOCK) {
                    return UInt(1, TransmitPattern::CLOCK_64_LOW_32);
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                return _mb_init->io.pattern_generator_io.transmit_req.pattern();
            }
            return UInt(1, TransmitPattern::CLOCK_64_LOW_32);
        };
        _pattern_generator->io.pattern_generator_io.transmit_req.sideband = [this]() -> Bool {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::SEND_CLOCK) {
                    return Bool(true);
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                return _mb_init->io.pattern_generator_io.transmit_req.sideband();
            }
            return Bool(false);
        };
        _pattern_generator->io.pattern_generator_io.transmit_req.timeout_cycles = [this]() -> UInt {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::SEND_CLOCK) {
                    return UInt(32, int(0.008 * _sb_clock_freq));
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                return _mb_init->io.pattern_generator_io.transmit_req.timeout_cycles();
            }
            return UInt(32, 0);
        };

        _pattern_generator->io.pattern_generator_io.transmit_pattern_status.assignReady(
            [this]() -> Bool {
                LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
                SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
                ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
                ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
                if (current_state == LinkTrainingState::sbInit) {
                    if (sb_init_sub_state == SBInitSubState::WAIT_CLOCK) {
                        return Bool(true);
                    }
                } else if (current_state == LinkTrainingState::mbInit) {
                    return _mb_init->io.pattern_generator_io.transmit_pattern_status.isReady();
                }
                return Bool(false);
            }
        );

        _sb_msg_wrapper->io.train_io.msg_req.valid = [this]() -> Bool {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_EXCH) {
                    return Bool(true);
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_REQ) {
                    return Bool(true);
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_RESP) {
                    return Bool(true);
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                return _mb_init->io.sb_train_io.msg_req.valid();
            } else if (current_state == LinkTrainingState::linkInit) {
                return _rdi_bringup->io.sb_train_io.msg_req.valid();
            }
            return Bool(false);
        };

        _sb_msg_wrapper->io.train_io.msg_req.msg = [this]() -> UInt {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_EXCH) {
                    return SBMessage_factory(
                        SBM().SBINIT_OUT_OF_RESET,
                        "PHY",
                        true,
                        "PHY"
                    );
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_REQ) {
                    return SBMessage_factory(
                        SBM().SBINIT_DONE_REQ,
                        "PHY",
                        true,
                        "PHY"
                    );
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_RESP) {
                    return SBMessage_factory(
                        SBM().SBINIT_DONE_RESP,
                        "PHY",
                        true,
                        "PHY"
                    );
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                return _mb_init->io.sb_train_io.msg_req.msg();
            } else if (current_state == LinkTrainingState::linkInit) {
                return _rdi_bringup->io.sb_train_io.msg_req.msg();
            }
            return UInt(128, 0);
        };

        _sb_msg_wrapper->io.train_io.msg_req.timeout_cycles = [this]() -> UInt {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_EXCH) {
                    return UInt(128, int(0.008*_sb_clock_freq));
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_REQ) {
                    return UInt(128, int(0.008*_sb_clock_freq));
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_RESP) {
                    return UInt(128, int(0.008*_sb_clock_freq));
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                return _mb_init->io.sb_train_io.msg_req.timeout_cycles();
            } else if (current_state == LinkTrainingState::linkInit) {
                return _rdi_bringup->io.sb_train_io.msg_req.timeout_cycles();
            }
            return UInt(128, 0);
        };

        _sb_msg_wrapper->io.train_io.msg_req_status.ready = [this]() -> Bool {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_WAIT) {
                    return Bool(true);
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_REQ_WAIT) {
                    return Bool(true);
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_RESP_WAIT) {
                    return Bool(true);
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                return _mb_init->io.sb_train_io.msg_req_status.ready();
            } else if (current_state == LinkTrainingState::linkInit) {
                return _rdi_bringup->io.sb_train_io.msg_req_status.ready();
            }
            return Bool(false);
        };

        io.sideband_fsm_io.pattern_tx_data.connect(_pattern_generator->io.sideband_lane_io.tx_data);
        io.sideband_fsm_io.packet_tx_data.connect(_sb_msg_wrapper->io.lane_io.tx_data);

        io.sideband_fsm_io.rx_data.assignReady(
            [this]() -> Bool {
                bool val = false;
                if (_msg_source().toBigUInt() == MsgSource::PATTERN_GENERATOR) {
                    return _pattern_generator->io.sideband_lane_io.rx_data.isReady();
                } else {
                    return _sb_msg_wrapper->io.lane_io.rx_data.isReady();
                }
                return Bool(val);
            }
        );
        _pattern_generator->io.sideband_lane_io.rx_data.assignValid(
            [this]() -> Bool {
                bool val = false;
                if (_msg_source().toBigUInt() == MsgSource::PATTERN_GENERATOR) {
                    return io.sideband_fsm_io.rx_data.isValid();
                } else {
                    return Bool(false);
                }
            }
        );
        _pattern_generator->io.sideband_lane_io.rx_data.assignBits(
            [this]() -> UInt {
                return io.sideband_fsm_io.rx_data.bits();
            }
        );

        _sb_msg_wrapper->io.lane_io.rx_data.assignValid(
            [this]() -> Bool {
                bool val = false;
                if (_msg_source().toBigUInt() == MsgSource::PATTERN_GENERATOR) {
                    return Bool(false);
                } else {
                    return io.sideband_fsm_io.rx_data.isValid();
                }
            }
        );
        _sb_msg_wrapper->io.lane_io.rx_data.assignBits(
            [this]() -> UInt {
                return io.sideband_fsm_io.rx_data.bits();
            }
        );

        _next_state = [this]() -> UInt {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            LinkTrainingState next_state = current_state;

            if (current_state == LinkTrainingState::reset) {
                if (reset_sub_state == ResetSubState::FREQ_SEL_LOCK_WAIT) {
                    if (io.mainband_fsm_io.pll_lock() && io.sideband_fsm_io.pll_lock()) {
                        next_state = LinkTrainingState::sbInit;
                    }
                }
            } else if (current_state == LinkTrainingState::sbInit) {
                if (sb_init_sub_state == SBInitSubState::WAIT_CLOCK) {
                    if (_pattern_generator->io.pattern_generator_io.transmit_pattern_status.fire()) {
                        if (_pattern_generator->io.pattern_generator_io.transmit_pattern_status.bits().toBigUInt() == MessageRequestStatusType::ERR) {
                            next_state = LinkTrainingState::linkError;
                        }
                    }
                } else if (sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_WAIT) {
                    if (_sb_msg_wrapper->io.train_io.msg_req_status.valid() &&
                        _sb_msg_wrapper->io.train_io.msg_req_status.ready()) {
                        if (_sb_msg_wrapper->io.train_io.msg_req_status.status().toBigUInt() == MessageRequestStatusType::ERR) {
                            next_state = LinkTrainingState::linkError;
                        }
                    }
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_REQ_WAIT) {
                    if (_sb_msg_wrapper->io.train_io.msg_req_status.valid() &&
                        _sb_msg_wrapper->io.train_io.msg_req_status.ready()) {
                        if (_sb_msg_wrapper->io.train_io.msg_req_status.status().toBigUInt() == MessageRequestStatusType::ERR) {
                            next_state = LinkTrainingState::linkError;
                        }
                    }
                } else if (sb_init_sub_state == SBInitSubState::SB_DONE_RESP_WAIT) {
                    if (_sb_msg_wrapper->io.train_io.msg_req_status.valid() &&
                        _sb_msg_wrapper->io.train_io.msg_req_status.ready()) {
                        if (_sb_msg_wrapper->io.train_io.msg_req_status.status().toBigUInt() == MessageRequestStatusType::SUCCESS) {
                            next_state = LinkTrainingState::mbInit;
                        }
                        if (_sb_msg_wrapper->io.train_io.msg_req_status.status().toBigUInt() == MessageRequestStatusType::ERR) {
                            next_state = LinkTrainingState::linkError;
                        }
                    }
                }
            } else if (current_state == LinkTrainingState::mbInit) {
                if (_mb_init->io.transition()) {
                    if (_mb_init->io.error()) {
                        next_state = LinkTrainingState::linkError;
                    } else {
                        next_state = LinkTrainingState::linkInit;
                    }
                }
            } else if (current_state == LinkTrainingState::linkInit) {
                if (_rdi_bringup->io.active()) {
                    next_state = LinkTrainingState::active;
                }
            }
            return UInt(4, next_state);
        };

        _reset_mb_init = [this]() -> Bool {
            auto res = _next_state().toBigUInt() == LinkTrainingState::mbInit;
            res = res && (!(_current_state->read().toBigUInt() == LinkTrainingState::mbInit));
            res = res || isReset();
            return Bool(res);
        };
        _mb_init->setReset(_reset_mb_init);

        io.rdi.rdi_bringup_io.pl_clk_req = _rdi_bringup->io.rdi_io.pl_clk_req;
        _rdi_bringup->io.rdi_io.lp_clk_ack = io.rdi.rdi_bringup_io.lp_clk_ack;
        _rdi_bringup->io.rdi_io.lp_wake_req = io.rdi.rdi_bringup_io.lp_wake_req;
        io.rdi.rdi_bringup_io.pl_wake_ack = _rdi_bringup->io.rdi_io.pl_wake_ack;
        _rdi_bringup->io.rdi_io.lp_state_req = io.rdi.rdi_bringup_io.lp_state_req;
        io.rdi.rdi_bringup_io.pl_state_status = _rdi_bringup->io.rdi_io.pl_state_status;
        io.rdi.rdi_bringup_io.pl_stall_req = _rdi_bringup->io.rdi_io.pl_stall_req;
        _rdi_bringup->io.rdi_io.lp_stall_ack = io.rdi.rdi_bringup_io.lp_stall_ack;
        _rdi_bringup->io.rdi_io.lp_link_error = io.rdi.rdi_bringup_io.lp_link_error;

        _rdi_bringup->io.sb_train_io.msg_req.ready = [this]() -> Bool {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            LinkTrainingState next_state = current_state;

            if (current_state == LinkTrainingState::linkInit) {
                return _sb_msg_wrapper->io.train_io.msg_req.ready();
            }
            return Bool(false);
        };
        _rdi_bringup->io.sb_train_io.msg_req_status.valid = [this]() -> Bool {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
            LinkTrainingState next_state = current_state;

            if (current_state == LinkTrainingState::linkInit) {
                return _sb_msg_wrapper->io.train_io.msg_req_status.valid();
            }
            return Bool(false);
        };
        _rdi_bringup->io.sb_train_io.msg_req_status.data = _sb_msg_wrapper->io.train_io.msg_req_status.data;
        _rdi_bringup->io.sb_train_io.msg_req_status.status = _sb_msg_wrapper->io.train_io.msg_req_status.status;

        io.sideband_fsm_io.rx_mode = [this]() -> UInt {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());

            if (current_state == LinkTrainingState::sbInit &&
                (sb_init_sub_state == SBInitSubState::SEND_CLOCK ||
                 sb_init_sub_state == SBInitSubState::WAIT_CLOCK ||
                 sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_EXCH ||
                 sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_WAIT)) {
                return UInt(1, RXTXMode::RAW);
            } else {
                return UInt(1, RXTXMode::PACKET);
            }
        };
        io.sideband_fsm_io.tx_mode = io.sideband_fsm_io.rx_mode;

        _mb_init->io.sb_train_io.msg_req.ready = [this]() -> Bool {
            if (_current_state->read().toBigUInt() == LinkTrainingState::mbInit) {
                return _sb_msg_wrapper->io.train_io.msg_req.ready();
            }
            return Bool(false);
        };
        _mb_init->io.sb_train_io.msg_req_status.valid = [this]() -> Bool {
            if (_current_state->read().toBigUInt() == LinkTrainingState::mbInit) {
                return _sb_msg_wrapper->io.train_io.msg_req_status.valid();
            }
            return Bool(false);
        };
        _mb_init->io.sb_train_io.msg_req_status.data = _sb_msg_wrapper->io.train_io.msg_req_status.data;
        _mb_init->io.sb_train_io.msg_req_status.status = _sb_msg_wrapper->io.train_io.msg_req_status.status;
        _mb_init->io.pattern_generator_io.transmit_req.ready = [this]() -> Bool {
            if (_current_state->read().toBigUInt() == LinkTrainingState::mbInit) {
                return _pattern_generator ->io.pattern_generator_io.transmit_req.ready();
            }
            return Bool(false);
        };
        _mb_init->io.pattern_generator_io.transmit_pattern_status.assignValid(
            [this]() -> Bool {
                if (_current_state->read().toBigUInt() == LinkTrainingState::mbInit) {
                    return _pattern_generator ->io.pattern_generator_io.transmit_pattern_status.isValid();
                }
                return Bool(false);
            }
        );
        _mb_init->io.pattern_generator_io.transmit_pattern_status.assignBits(
            [this]() -> UInt {
                return _pattern_generator->io.pattern_generator_io.transmit_pattern_status.bits();
            }
        );


        io.sideband_fsm_io.rx_en.capture(true);
        io.mainband_fsm_io.rx_en = [this]() -> Bool {
            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            bool val = current_state != LinkTrainingState::reset;
            return Bool(val);
        };

        io.mainband_fsm_io.tx_freq_sel.capture(3, SpeedMode::speed4);

        io.current_state = _current_state;

        _reset_freq_ctr_value = [this]() -> Bool {
            bool val = false;

            LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
            SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
            ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
            ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());

            if (current_state == LinkTrainingState::reset &&
                reset_sub_state == ResetSubState::INIT) {
                val = true;
            }
            return Bool(val);
        };

        _rdi_bringup->io.internal_error = [this]() -> Bool {
            return Bool(_current_state->read().toBigUInt() == LinkTrainingState::linkError);
        };

        _rdi_bringup->io.internal_retrain.capture(false);

        _counter_inc.capture(true);
        _counter->setReset(_reset_freq_ctr_value);
    }

    void LinkTrainingFSM::calcNextState() {
        LinkTrainingState current_state = static_cast<LinkTrainingState>(_current_state->read().toBigUInt());
        SBInitSubState sb_init_sub_state = static_cast<SBInitSubState>(_sb_init_sub_state->read().toBigUInt());
        ResetSubState reset_sub_state = static_cast<ResetSubState>(_reset_sub_state->read().toBigUInt());
        ActiveSubState active_sub_state = static_cast<ActiveSubState>(_active_sub_state->read().toBigUInt());
        LinkTrainingState next_state = static_cast<LinkTrainingState>(_next_state().toBigUInt());

        if (next_state == LinkTrainingState::reset && current_state != LinkTrainingState::reset) {
            *_reset_sub_state = UInt(2, ResetSubState::INIT);
        }

        if (next_state == LinkTrainingState::sbInit && current_state != LinkTrainingState::sbInit) {
            *_sb_init_sub_state = UInt(3, SBInitSubState::SEND_CLOCK);
        }

        PhyState pl_state_status = static_cast<PhyState>(_rdi_bringup->io.rdi_io.pl_state_status().toBigUInt());
        if (pl_state_status == PhyState::reset) {
            *_current_state = _next_state();
        } else if (pl_state_status == PhyState::active) {
            *_current_state = UInt(3, LinkTrainingState::active);
        } else if (pl_state_status == PhyState::retrain) {
            *_current_state = UInt(3, LinkTrainingState::retrain);
        } else if (pl_state_status == PhyState::linkError) {
            *_current_state = UInt(3, LinkTrainingState::linkError);
        }

        if (current_state != LinkTrainingState::active && next_state == LinkTrainingState::active) {
            *_active_sub_state = UInt(1, ActiveSubState::IDLE);
        }

        if (current_state == LinkTrainingState::reset) {
            if (reset_sub_state == ResetSubState::INIT) {
                if (io.mainband_fsm_io.pll_lock() && io.sideband_fsm_io.pll_lock()) {
                    *_reset_sub_state = UInt(2, ResetSubState::FREQ_SEL_CYC_WAIT);
                }
            } else if (reset_sub_state == ResetSubState::FREQ_SEL_CYC_WAIT) {
                // Plus 1 because the counter starts at 0
                if (_counter->getCount().toBigUInt() + 1 == _link_training_params.pll_wait_time - 1) {
                    *_reset_sub_state = UInt(2, ResetSubState::FREQ_SEL_LOCK_WAIT);
                }
            }
        } else if (current_state == LinkTrainingState::sbInit) {
            if (sb_init_sub_state == SBInitSubState::SEND_CLOCK) { // 0
                if (_pattern_generator->io.pattern_generator_io.transmit_req.valid() &&
                    _pattern_generator->io.pattern_generator_io.transmit_req.ready()) {
                    *_sb_init_sub_state = UInt(3, SBInitSubState::WAIT_CLOCK);
                }
            } else if (sb_init_sub_state == SBInitSubState::WAIT_CLOCK) {  // 1
                if (_pattern_generator->io.pattern_generator_io.transmit_pattern_status.fire()) {
                    if (_pattern_generator->io.pattern_generator_io.transmit_pattern_status.bits().toBigUInt() == MessageRequestStatusType::SUCCESS) {
                        *_sb_init_sub_state = UInt(3, SBInitSubState::SB_OUT_OF_RESET_EXCH);
                    }
                }
            } else if (sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_EXCH) {  // 2
                if (_sb_msg_wrapper->io.train_io.msg_req.valid() && _sb_msg_wrapper->io.train_io.msg_req.ready()) {
                    *_sb_init_sub_state = UInt(3, SBInitSubState::SB_OUT_OF_RESET_WAIT);
                }
            } else if (sb_init_sub_state == SBInitSubState::SB_OUT_OF_RESET_WAIT) { // 3
                if (_sb_msg_wrapper->io.train_io.msg_req_status.valid() && _sb_msg_wrapper->io.train_io.msg_req_status.ready()) {
                    if (_sb_msg_wrapper->io.train_io.msg_req_status.status().toBigUInt() == MessageRequestStatusType::SUCCESS) {
                        *_sb_init_sub_state = UInt(3, SBInitSubState::SB_DONE_REQ);
                    }
                }
            }
            else if (sb_init_sub_state == SBInitSubState::SB_DONE_REQ) { // 4
                if (_sb_msg_wrapper->io.train_io.msg_req.valid() && _sb_msg_wrapper->io.train_io.msg_req.ready()) {
                    *_sb_init_sub_state = UInt(3, SBInitSubState::SB_DONE_REQ_WAIT);
                }
            } else if (sb_init_sub_state == SBInitSubState::SB_DONE_REQ_WAIT) { // 5
                if (_sb_msg_wrapper->io.train_io.msg_req_status.valid() && _sb_msg_wrapper->io.train_io.msg_req_status.ready()) {
                    if (_sb_msg_wrapper->io.train_io.msg_req_status.status().toBigUInt() == MessageRequestStatusType::SUCCESS) {
                        *_sb_init_sub_state = UInt(3, SBInitSubState::SB_DONE_RESP);
                    }
                }
            } else if (sb_init_sub_state == SBInitSubState::SB_DONE_RESP) { // 6
                if (_sb_msg_wrapper->io.train_io.msg_req.valid() && _sb_msg_wrapper->io.train_io.msg_req.ready()) {
                    *_sb_init_sub_state = UInt(3, SBInitSubState::SB_DONE_RESP_WAIT);
                }
            }
        }

        RegModule::calcNextState();

        //// debug
        //std::cout << getPathName() << ": current_state " << static_cast<int>(current_state) << std::endl;
        //std::cout << getPathName() << ": next_state " << static_cast<int>(next_state) << std::endl;
        //std::cout << getPathName() << ": reset_sub_state " << static_cast<int>(reset_sub_state) << std::endl;
        //std::cout << getPathName() << ": sb_init_sub_state " << static_cast<int>(sb_init_sub_state) << std::endl;
        //std::cout << getPathName() << ": active_sub_state " << static_cast<int>(active_sub_state) << std::endl;
        //std::cout << getPathName() << ": io.mainband_fsm_io.pll_lock " << static_cast<bool>(io.mainband_fsm_io.pll_lock()) << std::endl;
        //std::cout << getPathName() << ": io.sideband_fsm_io.pll_lock " << static_cast<bool>(io.sideband_fsm_io.pll_lock()) << std::endl;
        //std::cout << getPathName() << ": io.rdi.rdi_bringup_io.pl_state_status " << static_cast<int>(pl_state_status) << std::endl;

        //*_debug_count = _debug_count->read() + UInt(32, 1);
        //std::cout << "=========================" << _debug_count->read().toBigUInt() << "===============================" << std::endl;
    }
} // namespace CCPS