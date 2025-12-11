
#ifndef __MB_INIT_FSM_TEST_HPP__
#define __MB_INIT_FSM_TEST_HPP__

#include "sideband/sb_msg_encoding.hpp"
#include "logphy/log_phy_types.hpp"

using namespace CCPS;

BigUInt formMsgReqData(
    int voltage_swing,
    int max_data_rate,
    ClockModeParam clock_mode,
    bool clock_phase,
    int module_id,
    bool CCPS_ax32
);

BigUInt formParamsReqMsgMsg(
    bool req,
    int voltage_swing,
    int max_data_rate,
    ClockModeParam clock_mode,
    bool clock_phase,
    int module_id,
    bool CCPS_ax32
);

int formParamsReqMsgTimeoutCycles(int sb_clock_freq);

#endif // __MB_INIT_FSM_TEST_HPP__