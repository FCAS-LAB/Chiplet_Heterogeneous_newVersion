#ifndef __MB_INIT_FSM_HPP__
#define __MB_INIT_FSM_HPP__

#include "utils/wire.hpp"
#include "utils/base_types.hpp"
#include "utils/decoupled.hpp"
#include "utils/module.hpp"
#include "sideband/sideband_io.hpp"
#include "logphy/log_phy_types.hpp"
#include "logphy/sb_msg_wrapper.hpp"
#include "logphy/pattern_generator.hpp"
#include "sideband/sb_msg_encoding.hpp"

namespace CCPS {
    class MBInitFSM: public RegModule {
    public:
        struct {
            SBMsgWrapperTrainIO sb_train_io;
            PatternGeneratorIO pattern_generator_io{true};
            Wire<Bool> transition; // O
            Wire<Bool> error;      // O
        } io;

        MBInitFSM(const LinkTrainingParams &link_training_params, const AfeParams &afe_params);
        void calcNextState() override;

    private:
        // ================= chisel signals ===============
        // 3bits
        enum class State {
            PARAM=0, REPAIR_CLK, REPAIR_VAL, IDLE, ERR
        };
        // 2bits
        enum class ParamSubState {
            SEND_REQ=0, WAIT_REQ, SEND_RESP, WAIT_RESP
        };

        RegPtr<UInt> _state;
        Wire<UInt> _next_state;
        RegPtr<UInt> _param_sub_state;

        RegPtr<UInt> _voltage_swing;
        RegPtr<UInt> _max_data_rate;
        RegPtr<UInt> _clock_mode;
        RegPtr<Bool> _clock_phase;
        RegPtr<UInt> _module_id;
        RegPtr<Bool> _CCPS_ax32;
        int _sb_clock_freq;

        RegPtr<UInt> _req_data;
        Wire<UInt> _exchange_max_data_rate;

        UInt formParamsReqMsg_Msg(
            const bool req,
            const UInt &voltage_swing,
            const UInt &max_data_rate,
            const ClockModeParam &clock_mode,
            const Bool &clock_phase,
            const UInt &module_id,
            const Bool &CCPS_ax32
        ) {
            UInt data(50, 0);
            data.append(CCPS_ax32);
            data.append(module_id(1, 0));
            data.append(clock_phase);
            data.append(UInt(1, clock_mode));
            data.append(voltage_swing(4, 0));
            data.append(max_data_rate(3, 0));

            auto base = SBM().MBINIT_PARAM_CONFIG_REQ;
            if (!req) {
                base = SBM().MBINIT_PARAM_CONFIG_RESP;
            }
            return SBMessage_factory(
                base,
                "PHY",
                false,
                "PHY",
                data
            );
        }
    };
} // namespace CCPS

#endif // __MB_INIT_FSM_HPP__