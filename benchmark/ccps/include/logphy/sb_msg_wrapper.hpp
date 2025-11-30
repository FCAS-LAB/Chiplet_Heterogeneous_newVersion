#ifndef __SB_MSG_WRAPPER_HPP__
#define __SB_MSG_WRAPPER_HPP__

#include "utils/wire.hpp"
#include "utils/base_types.hpp"
#include "utils/decoupled.hpp"
#include "utils/module.hpp"
#include "sideband/sideband_io.hpp"
#include "logphy/log_phy_types.hpp"

namespace CCPS {

    struct SBMsgWrapperTrainIO {
        struct {
            Wire<Bool> valid;               // I
            Wire<Bool> ready;               // O
            Wire<UInt> msg;                 // I // 128 bits
            Wire<UInt> timeout_cycles;      // I // 64 bits
        } msg_req;
        struct {
            Wire<Bool> valid;               // O
            Wire<Bool> ready;               // I
            Wire<UInt> status;              // O // 1 bits
            Wire<UInt> data;                // O // 64 bits
        } msg_req_status;
    };

    class SBMsgWrapper: public RegModule {
    public:
        struct {
            SBMsgWrapperTrainIO train_io;
            SidebandLaneIO lane_io{true}; // sideband params
        } io;

        SBMsgWrapper(const SidebandParams &sb_params);
        void calcNextState() override;

    private:
        // =========== chisel signals ==============
        enum class State {
            IDLE = 0,
            EXCHANGE,
            WAIT_ACK
        };
        RegPtr<UInt> _current_state;
        RegPtr<UInt> _timeout_counter;
        Wire<UInt> _next_state;
        RegPtr<Bool> _sent_msg;
        RegPtr<Bool> _received_msg;
        RegPtr<UInt> _current_req;
        RegPtr<UInt> _current_req_timeout_max;
        RegPtr<UInt> _current_status;
        RegPtr<UInt> _data_out;
        Wire<Bool> _has_sent_msg;
        Wire<Bool> _just_received_msg;
        Wire<Bool> _has_received_msg;

        Bool messageIsEqual(UInt m1, UInt m2) {
            bool res = m1(4, 0) == m2(4, 0) && // opcode
                       m1(21, 14) == m2(21, 14) && // subcode
                       m1(39, 32) == m2(39, 32); // code
            return Bool(res);
        }
    };
} // namespace CCPS

#endif // __SB_MSG_WRAPPER_HPP__