#include "logphy/mb_init_fsm.hpp"
#include "mb_init_fsm_test.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include "interfaces/rdi.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

//原先static const LinkTrainingParams link_training_params {.sb_clock_freq_analog = 8000};
// 替换原代码的一行
static const LinkTrainingParams link_training_params = {8000};

static const SidebandParams sb_params;
static const AfeParams afe_params;
static const RdiParams rdi_params;
static const int sb_w = sb_params.sb_node_msg_width;
static const int sb_clock_freq = link_training_params.sb_clock_freq_analog / afe_params.sb_serializer_ratio;

// IOs
static bool io_sb_train_io_msg_req_ready = false;
static bool io_sb_train_io_msg_req_status_valid = false;
static BigUInt io_sb_train_io_msg_req_status_data = 0;
static BigUInt io_sb_train_io_msg_req_status_status = 0;
static bool io_pattern_generator_io_transmit_req_ready = false;
static bool io_pattern_generator_io_transmit_pattern_status_valid = false;
static BigUInt io_pattern_generator_io_transmit_pattern_status_bits = 0;

static void connectIOs(MBInitFSM &c) {
    c.io.sb_train_io.msg_req.ready.capture(io_sb_train_io_msg_req_ready);
    c.io.sb_train_io.msg_req_status.valid.capture(io_sb_train_io_msg_req_status_valid);
    c.io.sb_train_io.msg_req_status.data.capture(64, io_sb_train_io_msg_req_status_data);
    c.io.sb_train_io.msg_req_status.status.capture(1, io_sb_train_io_msg_req_status_status);
    c.io.pattern_generator_io.transmit_req.ready.capture(io_pattern_generator_io_transmit_req_ready);
    c.io.pattern_generator_io.transmit_pattern_status.assignValid(io_pattern_generator_io_transmit_pattern_status_valid);
    c.io.pattern_generator_io.transmit_pattern_status.assignBits(1, io_pattern_generator_io_transmit_pattern_status_bits);
}

BigUInt formMsgReqData(
    int voltage_swing,
    int max_data_rate,
    ClockModeParam clock_mode,
    bool clock_phase,
    int module_id,
    bool CCPS_ax32
) {
    BigUInt data;
    data |= (CCPS_ax32 ? 1 : 0) << 13;
    data |= (module_id & 0x3) << 11;
    data |= (clock_phase ? 1 : 0) << 10;
    data |= (clock_mode == ClockModeParam::strobe ? 0 : 1) << 9;
    data |= (voltage_swing & 0x1f) << 4;
    data |= (max_data_rate & 0xf);
    return data;
}

BigUInt formParamsReqMsgMsg(
    bool req,
    int voltage_swing,
    int max_data_rate,
    ClockModeParam clock_mode,
    bool clock_phase,
    int module_id,
    bool CCPS_ax32
) {
    auto data = formMsgReqData(
        voltage_swing,
        max_data_rate,
        clock_mode,
        clock_phase,
        module_id,
        CCPS_ax32
    );

    auto base = SBM().MBINIT_PARAM_CONFIG_REQ;
    if (!req) {
        base = SBM().MBINIT_PARAM_CONFIG_RESP;
    }

    return SBMessage_factory(
        base,
        "PHY",
        false,
        "PHY",
        UInt(64, data)
    ).toBigUInt();
}

int formParamsReqMsgTimeoutCycles(int sb_clock_freq) {
    return int(0.008 * sb_clock_freq);
}

void enqueueParamRespResp(MBInitFSM &c) {
    io_sb_train_io_msg_req_status_valid = true;
    io_sb_train_io_msg_req_status_data = formMsgReqData(
        link_training_params.mb_training_params.voltage_swing,
        link_training_params.mb_training_params.maximum_data_rate,
        link_training_params.mb_training_params.clock_mode,
        link_training_params.mb_training_params.clock_phase,
        link_training_params.mb_training_params.module_id,
        link_training_params.mb_training_params.CCPS_ax32
    );
    io_sb_train_io_msg_req_status_status = MessageRequestStatusType::SUCCESS;
    while (!c.io.sb_train_io.msg_req_status.ready()) {
        c.step();
    }
    c.step();
    io_sb_train_io_msg_req_status_valid = false;
    c.step();
}

void dequeueParamResp(MBInitFSM &c) {
    io_sb_train_io_msg_req_ready = true;
    while (!c.io.sb_train_io.msg_req.valid()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sb_train_io.msg_req.ready(), true);
    EXPECT_EQ_BOOL(c.io.sb_train_io.msg_req.valid(), true);
    EXPECT_EQ_BigUInt(c.io.sb_train_io.msg_req.msg(),
                      formParamsReqMsgMsg(
                        false,
                        link_training_params.mb_training_params.voltage_swing,
                        link_training_params.mb_training_params.maximum_data_rate,
                        link_training_params.mb_training_params.clock_mode,
                        link_training_params.mb_training_params.clock_phase,
                        link_training_params.mb_training_params.module_id,
                        link_training_params.mb_training_params.CCPS_ax32
                    )
    );
    EXPECT_EQ_BigUInt(c.io.sb_train_io.msg_req.timeout_cycles(),
                      formParamsReqMsgTimeoutCycles(sb_clock_freq)
    );

    c.step();
    io_sb_train_io_msg_req_ready = false;
    EXPECT_EQ_BOOL(c.io.transition(), false);
    EXPECT_EQ_BOOL(c.io.error(), false);
}

void enqueueParamReqResp(MBInitFSM &c) {
    io_sb_train_io_msg_req_status_valid = true;
    io_sb_train_io_msg_req_status_data = formMsgReqData(
        link_training_params.mb_training_params.voltage_swing,
        link_training_params.mb_training_params.maximum_data_rate,
        link_training_params.mb_training_params.clock_mode,
        link_training_params.mb_training_params.clock_phase,
        link_training_params.mb_training_params.module_id,
        link_training_params.mb_training_params.CCPS_ax32
    );
    io_sb_train_io_msg_req_status_status = MessageRequestStatusType::SUCCESS;
    while (!c.io.sb_train_io.msg_req_status.ready()) {
        c.step();
    }
    c.step();
    io_sb_train_io_msg_req_status_valid = false;
    c.step();
    EXPECT_EQ_BOOL(c.io.transition(), false);
    EXPECT_EQ_BOOL(c.io.error(), false);
}

void dequeueParamReq(MBInitFSM &c) {
    io_sb_train_io_msg_req_ready = true;
    while (!c.io.sb_train_io.msg_req.valid()) {
        c.step();
    }
    EXPECT_EQ_BOOL(c.io.sb_train_io.msg_req.ready(), true);
    EXPECT_EQ_BOOL(c.io.sb_train_io.msg_req.valid(), true);
    EXPECT_EQ_BigUInt(c.io.sb_train_io.msg_req.msg(),
                      formParamsReqMsgMsg(
                          true,
                          link_training_params.mb_training_params.voltage_swing,
                          link_training_params.mb_training_params.maximum_data_rate,
                          link_training_params.mb_training_params.clock_mode,
                          link_training_params.mb_training_params.clock_phase,
                          link_training_params.mb_training_params.module_id,
                          link_training_params.mb_training_params.CCPS_ax32
                      )
    );
    EXPECT_EQ_BigUInt(c.io.sb_train_io.msg_req.timeout_cycles(),
                      formParamsReqMsgTimeoutCycles(sb_clock_freq)
    );

    c.step();
    io_sb_train_io_msg_req_ready = false;
    c.step();
    EXPECT_EQ_BOOL(c.io.transition(), false);
    EXPECT_EQ_BOOL(c.io.error(), false);
}

void initialCheck(MBInitFSM &c) {
    EXPECT_EQ_BOOL(c.io.transition(), false);
    EXPECT_EQ_BOOL(c.io.error(), false);
    c.step();
}

void initializePorts(MBInitFSM &c) {
    return ;
}

TEST (MBInitFSMTest, PerformParameterExchangeBasicSim) {
    auto top = createTopModule<MBInitFSM>(link_training_params, afe_params);
    auto &c = *top;

    // connect
    connectIOs(c);

    // run
    initializePorts(c);
    initialCheck(c);
    dequeueParamReq(c);
    enqueueParamReqResp(c);
    dequeueParamResp(c);
    enqueueParamRespResp(c);

    EXPECT_EQ_BOOL(c.io.transition(), true);
    EXPECT_EQ_BOOL(c.io.error(), false);
}

TEST (MBInitFSMTest, PerformParameterExchangeWithDelays) {
    auto top = createTopModule<MBInitFSM>(link_training_params, afe_params);
    auto &c = *top;

    // connect
    connectIOs(c);

    // run
    initializePorts(c);
    initialCheck(c);
    dequeueParamReq(c);

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.transition(), false);
    }
    enqueueParamReqResp(c);
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.transition(), false);
    }
    dequeueParamResp(c);
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.transition(), false);
    }
    enqueueParamRespResp(c);

    EXPECT_EQ_BOOL(c.io.transition(), true);
    EXPECT_EQ_BOOL(c.io.error(), false);
}

TEST (MBInitFSMTest, Timeout) {
    auto top = createTopModule<MBInitFSM>(link_training_params, afe_params);
    auto &c = *top;

    // connect
    connectIOs(c);

    const int timeout = (0.008 * sb_clock_freq) + 20;
    initializePorts(c);
    initialCheck(c);
    dequeueParamReq(c);
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.transition(), false);
        c.step();
    }
    enqueueParamReqResp(c);
    dequeueParamResp(c);

    io_sb_train_io_msg_req_status_valid = true;
    io_sb_train_io_msg_req_status_data = formMsgReqData(
        link_training_params.mb_training_params.voltage_swing,
        link_training_params.mb_training_params.maximum_data_rate,
        link_training_params.mb_training_params.clock_mode,
        link_training_params.mb_training_params.clock_phase,
        link_training_params.mb_training_params.module_id,
        link_training_params.mb_training_params.CCPS_ax32
    );
    io_sb_train_io_msg_req_status_status = MessageRequestStatusType::ERR;
    while (!c.io.sb_train_io.msg_req_status.ready()) {
        c.step();
    }
    c.step();
    io_sb_train_io_msg_req_status_valid = false;
    c.step();

    EXPECT_EQ_BOOL(c.io.transition(), true);
    EXPECT_EQ_BOOL(c.io.error(), true);
}
