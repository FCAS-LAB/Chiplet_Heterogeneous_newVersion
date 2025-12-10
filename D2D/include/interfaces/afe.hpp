#ifndef __AFE_HPP__
#define __AFE_HPP__

#include "interfaces/types.hpp"
#include "utils/clock.hpp"
#include "utils/wire.hpp"
#include "utils/base_types.hpp"
#include "utils/module.hpp"

namespace CCPS
{

    struct FifoParams {
      ClockPtr clk;
      Wire<Bool> reset;
    };

    /** The mainband pins exposed by a standard package CCPS module in one
      * direction.
      */
    struct MainbandIo {
      Wire<UInt> data; // default 16bits
      Wire<Bool> valid;
      Wire<Bool> track;
      ClockPtr clkp;
      ClockPtr clkn;
    };

    /** The sideband pins exposed by a standard package CCPS module in one
      * direction.
      */
    struct SidebandIo {
      Wire<Bool> data;
      ClockPtr clk;
    };

    /** The pins (mainband and sideband) exposed by a standard package CCPS module
      * in one direction.
      */
    struct UnidirectionalIo {
      MainbandIo mainband; // default 16bits
      SidebandIo sideband;
    };

    /** The pins (mainband and sideband) exposed by a standard package CCPS module
      * in both directions.
      */
    struct StandardPackageIo {
      UnidirectionalIo tx; // O  default 16bits
      UnidirectionalIo rx; // I  default 16bits
    };

    struct AfeParams {
        int sb_serializer_ratio{1};
        int sb_width{1};
        int mb_serializer_ratio{16};
        int mb_lanes{16};

        AfeParams(
            int sb_serializer_ratio = 1,
            int sb_width = 1,
            int mb_serializer_ratio = 16,
            int mb_lanes = 16
        ) {
          this->sb_serializer_ratio = sb_serializer_ratio;
          this->sb_width = sb_width;
          this->mb_serializer_ratio = mb_serializer_ratio;
          this->mb_lanes = mb_lanes;
        }
    };

    /** The sideband analog front-end (AFE) interface, from the perspective of the
      * logical PHY layer.
      *
      * All signals in this interface are synchronous to the sideband clock (fixed
      * at 800 MHz). As a result, the sideband's `serializerRatio` likely will be
      * different from the mainband's `serializerRatio`.
      */
    struct SidebandAfeIo {

        FifoParams fifo_params; // I

        /** Data to transmit on the sideband.
          *
          * Output from the async FIFO.
          */
        Wire<UInt> tx_data;// O afeParams.sbWidth
        ClockPtr tx_clock;// O

        /** Data received on the sideband.
          *
          * Input to the async FIFO.
          */
        Wire<UInt> rx_data;// I afeParams.sbWidth
        ClockPtr rx_clock;// I

        /** Enable sideband receivers. */
        Wire<Bool> rx_en;// O

        /** Sideband PLL Lock.
          *
          * Indicates whether the sideband clock is stable.
          */
        Wire<Bool> pll_lock;// I

        void connect(SidebandAfeIo &other) {
            // connect fifo_params.clk in propagateClock function
            // connect tx_clock in propagateClock function
            // connect rx_clock in propagateClock function

            other.fifo_params.reset = fifo_params.reset;
            tx_data = other.tx_data;
            other.rx_data = rx_data;
            rx_en = other.rx_en;
            other.pll_lock = pll_lock;
        }
    };

    /** The mainband analog front-end (AFE) interface, from the perspective of the
      * logical PHY layer.
      *
      * All signals in this interface are synchronous to the mainband AFE's digital
      * clock, which is produced by taking a high speed clock from a PLL and
      * dividing its frequency by `serializerRatio`.
      *
      * With half-rate clocking (1 data bit transmitted per UI; 1 UI = 0.5 clock
      * cycles), the PLL clock may be 2, 4, 6, 8, 12, or 16 GHz. With a serializer
      * ratio of 16, this results in a 0.125-1 GHz AFE digital clock.
      */
    struct MainbandAfeIo {

        FifoParams fifo_params; // I

        /** Data to transmit on the mainband. Output from the async FIFO.
          */
        std::vector<Decoupled<UInt>> tx_data; // afeParams.mbLanes, afeParams.mbSerializerRatio

        /** Data received on the mainband. Input to the async FIFO.
          */
        std::vector<Decoupled<UInt>> rx_data; // flipped. afeParams.mbLanes, afeParams.mbSerializerRatio

        Wire<UInt> tx_freq_sel; // O

        /** Mainband receiver enable.
          */
        Wire<Bool> rx_en;// O

        /** Mainband PLL Lock. Indicates whether the mainband clock is stable.
          */
        Wire<Bool> pll_lock;// I

        void connect(MainbandAfeIo &other) {
            // connect fifo_params.clk in propagateClock function

            other.fifo_params.reset = fifo_params.reset;

            tx_data.resize(other.tx_data.size());
            for (size_t i = 0; i < tx_data.size(); ++i) {
                tx_data[i].connect(other.tx_data[i]);
            }

            rx_data.resize(other.rx_data.size());
            for (size_t i = 0; i < rx_data.size(); ++i) {
                other.rx_data[i].connect(rx_data[i]);
            }

            tx_freq_sel = other.tx_freq_sel;
            rx_en = other.rx_en;
            other.pll_lock = pll_lock;
        }
    };

} // namespace CCPS

#endif // __AFE_HPP__
