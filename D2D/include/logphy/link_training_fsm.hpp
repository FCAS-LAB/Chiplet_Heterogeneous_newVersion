#ifndef __LINK_TRAINING_FSM_HPP__
#define __LINK_TRAINING_FSM_HPP__

#include "utils/wire.hpp"
#include "utils/decoupled.hpp"
#include "utils/module.hpp"
#include "logphy/log_phy_types.hpp"
#include "logphy/sb_msg_wrapper.hpp"
#include "logphy/rdi_bringup.hpp"
#include "logphy/mb_init_fsm.hpp"
#include "logphy/pattern_generator.hpp"
#include "utils/counter.hpp"

namespace CCPS {

    struct SidebandFSMIO {
        Decoupled<UInt> rx_data;  // sbParams.sbNodeMsgWidth
        Decoupled<UInt> pattern_tx_data{true};// sbParams.sbNodeMsgWidth
        Decoupled<UInt> packet_tx_data{true};// sbParams.sbNodeMsgWidth
        Wire<UInt> rx_mode; // I    // 1bit
        Wire<UInt> tx_mode; // I    // 1bit
        Wire<Bool> rx_en;   // I
        Wire<Bool> pll_lock;// O
    };

    struct MainbandFSMIO {
        Wire<Bool> rx_en;   // I
        Wire<Bool> pll_lock;// O
        Wire<UInt> tx_freq_sel; // I // 3bits
    };

    class LinkTrainingFSM: public RegModule {
    public:
        struct {
            MainbandFSMIO mainband_fsm_io;   // flipped
            SidebandFSMIO sideband_fsm_io;   // flipped
            struct {
                RdiBringupIO rdi_bringup_io; // non-flipped
            } rdi;
            Wire<UInt> current_state; // O // 3bits
        } io;

        LinkTrainingFSM(
            const LinkTrainingParams &link_training_params,
            const SidebandParams &sb_params,
            const AfeParams &afe_params
        );

        void calcNextState() override;

    private:
        // ========== chisel signals ==============
        enum class ResetSubState { // 2bits
            INIT, FREQ_SEL_CYC_WAIT, FREQ_SEL_LOCK_WAIT
        };
        enum class SBInitSubState { // 3bits
            SEND_CLOCK=0, WAIT_CLOCK, SB_OUT_OF_RESET_EXCH, SB_OUT_OF_RESET_WAIT,
            SB_DONE_REQ, SB_DONE_REQ_WAIT, SB_DONE_RESP, SB_DONE_RESP_WAIT
        };
        enum class ActiveSubState { // 1bit
            IDLE=0
        };

        int _sb_clock_freq;

        ModulePtr<PatternGenerator> _pattern_generator;
        ModulePtr<SBMsgWrapper> _sb_msg_wrapper;
        ModulePtr<MBInitFSM> _mb_init;
        ModulePtr<RdiBringup> _rdi_bringup;
        ModulePtr<Counter> _counter;

        Wire<UInt> _msg_source; // 1bit
        RegPtr<UInt> _current_state; // 3bits
        Wire<UInt> _next_state;     // 3bits
        RegPtr<UInt> _reset_sub_state;  // 3bits
        RegPtr<UInt> _sb_init_sub_state;  // 3bits
        //Wire<UInt> _pl_state_status;    // 4bits
        Wire<Bool> _reset_freq_ctr_value;
        RegPtr<UInt> _active_sub_state; // 1bit

        // ============ helper signals ============
        Wire<Bool> _reset_mb_init;
        Wire<Bool> _counter_inc;
        LinkTrainingParams _link_training_params;
        RegPtr<UInt> _debug_count;
    };
} // namespace CCPS

#endif // __LINK_TRAINING_FSM_HPP__