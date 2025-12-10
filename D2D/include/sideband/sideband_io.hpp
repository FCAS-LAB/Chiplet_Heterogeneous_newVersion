#ifndef __SIDEBAND_IO_HPP__
#define __SIDEBAND_IO_HPP__

#include "utils/base_types.hpp"
#include "utils/wire.hpp"
#include "utils/decoupled.hpp"
#include "interfaces/fdi.hpp"
#include "utils/module.hpp"

namespace CCPS {
    enum class RXTXMode { // 1bit
        RAW = 0,
        PACKET
    };

    // ===================== put SidebandParams here to avoid circular reference =======================
    class SidebandParams {
        public:
            SidebandParams(): sb_node_msg_width(128), max_crd(32) {}
            ~SidebandParams() = default;

            int sb_node_msg_width;
            int max_crd;
        };

    // ===================== end SidebandParams =======================
    struct SidebandNodeOuterIOSingle {
        Wire<UInt> bits;        // out
        Wire<Bool> valid;       // out
        Wire<Bool> credit;      // in

        SidebandNodeOuterIOSingle(bool do_flip=false);
        void connect(SidebandNodeOuterIOSingle &other);

    private:
        bool _is_flipped;
    };

    struct SidebandNodeOuterIO {
        SidebandNodeOuterIOSingle tx;
        SidebandNodeOuterIOSingle rx{true};

        SidebandNodeOuterIO(bool do_flip=false);
        void connect(SidebandNodeOuterIO &other);

    private:
        bool _is_flipped;
    };

    struct SidebandLinkNodeOuterIOSingle{
        Wire<UInt> bits;
        ClockPtr clock;

        SidebandLinkNodeOuterIOSingle(bool do_flip=false);
        // Need to connect clocks in module propagateClock function.
        void connect(SidebandLinkNodeOuterIOSingle &other);

    private:
        bool _is_flipped;
    };

    struct SidebandLinkNodeOuterIO {
        SidebandLinkNodeOuterIOSingle tx;
        SidebandLinkNodeOuterIOSingle rx{true};

        SidebandLinkNodeOuterIO(bool do_flip=false);
        // Need to connect clocks in module propagateClock function.
        void connect(SidebandLinkNodeOuterIO &other);

    private:
        bool _is_flipped;
    };

    class SidebandSwitcherbundle {
    public:
        Decoupled<UInt> node_to_layer_above{true};
        Decoupled<UInt> layer_to_node_above;
        Decoupled<UInt> node_to_layer_below{true};
        Decoupled<UInt> layer_to_node_below;

        SidebandSwitcherbundle(bool do_flip);
        SidebandSwitcherbundle();

        void flip();
        void connect(SidebandSwitcherbundle& other);

    private:
        bool _is_flipped;
    };

    class D2DSidebandChannelIO {
    public:
        // connect to another sideband node in the layer above (protocol layer)
        SidebandNodeOuterIO to_upper_layer;

        // connect to another sideband node in the layer below (physical layer)
        SidebandNodeOuterIO to_lower_layer;

        // d2d layer drive these
        SidebandSwitcherbundle inner{true};
    };

    class PHYSidebandChannelIO {
    public:
        // connect to another sideband node in the layer above (d2d layer)
        SidebandNodeOuterIO to_upper_layer;

        // connect to another sideband node in the layer below (link layer)
        SidebandLinkNodeOuterIO to_lower_layer;

        // phy layer drive these
        struct {
            Wire<UInt> input_mode;
            Wire<UInt> rx_mode;
            Decoupled<UInt> raw_input{true};
            SidebandSwitcherbundle switcher_bundle{true};
        } inner;
    };

    class SidebandNodeIO {
    public:
        // layers drive these
        struct {
            Decoupled<UInt> layer_to_node{true};
            /* This signal overrides the tx.ready and takes up the priority reserved
             * queue slot */
            // Should only be asserted high for access completion packets
            Decoupled<UInt> node_to_layer;
        } inner;

        SidebandNodeOuterIO outer;
    };

    // IO for the remote sideband
    class SidebandLinkIO {
    public:
        // layers drive these
        Wire<UInt> rx_mode;
        struct {
            Decoupled<UInt> layer_to_node{true};
            /* This signal overrides the tx.ready and takes up the priority reserved
             * queue slot */
            // Should only be asserted high for access completion packets
            Decoupled<UInt> node_to_layer;
        } inner;

        SidebandLinkNodeOuterIO outer;

    };

    class SidebandSwitcherIO {
    public:
        SidebandSwitcherbundle inner{true};
        SidebandSwitcherbundle outer;

        SidebandSwitcherIO() = default;
    };


}

#endif // __SIDEBAND_IO_HPP__