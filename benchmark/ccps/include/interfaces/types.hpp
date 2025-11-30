#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include "utils/wire.hpp"

namespace CCPS {

    /** The state of the logical PHY. */
     // 4bits
     enum class PhyState { // 4bits
        reset = 0,
        active = 1,
        activePmNak = 3,
        l1 = 4,
        l2 = 8,
        linkReset = 9,
        linkError = 0xA,
        retrain = 0xB,
        disabled = 0xC
    };

    /** A request for the PHY to change state. */
     // 4bits
     enum class PhyStateReq {
        nop = 0x0,
        active = 0x1,
        l1 = 0x4,
        l2 = 0x8,
        linkReset = 0x9,
        retrain = 0xb,
        disabled = 0xc
    };

    /** The speed of the physical layer of the link, in GT/s. */
     // 3bits
     enum class SpeedMode {
        speed4 = 0x0,
        speed8 = 0x1,
        speed12 = 0x2,
        speed16 = 0x3,
        speed24 = 0x4,
        speed32 = 0x5
    };

    /** The number of physical lanes in the PHY, after link degradation. */
     // 3bits
     enum class PhyWidth {
        width8 = 0x1,
        width16 = 0x2,
        width32 = 0x3,
        width64 = 0x4,
        width128 = 0x5,
        width256 = 0x6
    };

    /** The protocol stack. Defaults to stack 0.
      *
      * Some CCPS links can support running multiple protocols over the same
      * physical link. In this case, `ProtoStack` indicates which protocol stack a
      * message is associated with.
      */
     // 4bits
     enum class ProtoStack {
        stack0 = 0x0,
        stack1 = 0x1
    };

    /** The protocol type running on the CCPS link. */
     // 4bits
     enum class ProtoStreamType {

      /** PCIe */
        PCIe = 0x1,

      /** CXL.io */
        CXLI = 0x2,

      /** CXL.cache */
        CXLC = 0x3,

      /** Streaming */
        Stream = 0x4
    };

    struct ProtoStream {
        Wire<UInt> proto_stack; // 4bits
        Wire<UInt> proto_type;  // 4bits

        void connect(ProtoStream &other) {
            proto_stack = other.proto_stack;
            proto_type = other.proto_type;
        }
    };

     // 3bits
     enum class Protocol {
        pcie = 0x0,
        cxl1 = 0x3,
        cxl2 = 0x4,
        cxl3 = 0x5,
        cxl4 = 0x6,
        streaming = 0x7
    };

     // 4bits
     enum class FlitFormat {
        raw = 0x1,
        flit68 = 0x2,
        standard256EndHeader = 0x3,
        standard256StartHeader = 0x4,
        latencyOpt256NoOptional = 0x5,
        latencyOpt256Optional = 0x6
    };

    struct AsyncQueueParams {
        int depth;

        AsyncQueueParams(int depth=8): depth(depth) {}
    };


} // namespace CCPS

#endif // __TYPES_HPP__
