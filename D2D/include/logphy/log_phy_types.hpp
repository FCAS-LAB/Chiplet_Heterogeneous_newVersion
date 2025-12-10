#ifndef __LOGPHY_LOG_PHY_TYPES_HPP__
#define __LOGPHY_LOG_PHY_TYPES_HPP__
#include "utils/wire.hpp"
#include "utils/base_types.hpp"
#include "utils/decoupled.hpp"
#include "interfaces/afe.hpp"

namespace CCPS {
    // 3 bits
    enum class LinkTrainingState {
        reset = 0,
        sbInit,
        mbInit,
        linkInit,
        active,
        linkError,
        retrain
    };

    // 1 bits
    enum class MsgSource {
        PATTERN_GENERATOR,
        SB_MSG_WRAPPER
    };

    /** Sideband Types */

    struct SBExchangeMsg {
        Wire<UInt> exchange_msg; // 128 bits
    };

    // 1 bits
    enum class MessageRequestStatusType {
        SUCCESS = 0,
        ERR
    };

    struct SBReqMsg {
        Wire<UInt> msg; // 128 bits
    };

    struct MessageRequest  {
        Wire<UInt> msg; // 128 bits
        Wire<UInt> timeout_cycles; // 64 bits
      // val msgTypeHasData = Bool()
    };

    struct MessageRequestStatus  {
        Wire<UInt> status;  // 1bits
        Wire<UInt> data; // 64bits
    };

    // 1bits
    /** Param Enums */

    enum class ClockModeParam {
        strobe = 0,
        continuous
    };

    // 1bits
    enum class TransmitPattern {
        CLOCK_64_LOW_32 = 0
    };

    struct SBIO {

        FifoParams fifo_params; // I

        /** Data to transmit on the sideband.
          *
          * Output from the async FIFO.
          */
        Decoupled<UInt> tx_data; // params.sbSerializerRatio
        Decoupled<Bool> tx_valid;

        /** Data received on the sideband.
          *
          * Input to the async FIFO.
          */
        Decoupled<UInt> rx_data{true}; // params.sbSerializerRatio
    };

    struct MainbandIO {

        FifoParams fifo_params; // I

      /** Data to transmit on the mainband.
        *
        * Output from the async FIFO.
        *
        * @group data
        */
        std::vector<Decoupled<UInt>> tx_data; // afeParams.mbLanes, afeParams.mbSerializerRatio

      /** Data received on the mainband.
        *
        * Input to the async FIFO.
        *
        * @group data
        */
      std::vector<Decoupled<UInt>> rx_data; // afeParams.mbLanes, afeParams.mbSerializerRatio, all flipped
    };

    struct MainbandLaneIO {
        /** Data to transmit on the mainband.
          */
        Decoupled<UInt> tx_data{true}; // afeParams.mbLanes * afeParams.mbSerializerRatio

        Valid<UInt> rx_data; // afeParams.mbLanes * afeParams.mbSerializerRatio

        MainbandLaneIO(bool do_flip = false) {
          if (do_flip) {
            flip();
          }
        }

        void flip() {
          tx_data.flip();
          rx_data.flip();
        }
    };

    struct SidebandLaneIO {
        /** Data to transmit on the mainband.
          */
        Decoupled<UInt> tx_data{true}; // sbParams.sbNodeMsgWidth

        Decoupled<UInt> rx_data; // sbParams.sbNodeMsgWidth

        SidebandLaneIO(bool do_flip=false) {
            if (do_flip) {
              flip();
            }
        }

        void flip() {
            tx_data.flip();
            rx_data.flip();
        }
    };

    // moved from LinkTrainingFSM.scala
    struct MBTrainingParams {
        int voltage_swing = 0;
        int maximum_data_rate = 0;
        ClockModeParam clock_mode = ClockModeParam::strobe;
        bool clock_phase = false;
        int module_id = 0;
        bool CCPS_ax32 = false;
    };

    // moved from LinkTrainingFSM.scala
    /** Implementation TODOs:
      *   - investigate multiple message source issue
      *   - implement plStallReq
      *   - implement lpStateReq
      */
    struct LinkTrainingParams {
        int pll_wait_time = 100;
        int max_sb_message_size = 128;
        MBTrainingParams mb_training_params = MBTrainingParams();
        int sb_clock_freq_analog = 800'000'000; // 800MHz
    };

} // namespace CCPS

#endif // __LOGPHY_LOG_PHY_TYPES_HPP__