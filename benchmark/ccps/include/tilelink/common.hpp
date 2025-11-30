#ifndef __TILELINK_COMMON_HPP__
#define __TILELINK_COMMON_HPP__

#include "utils/wire.hpp"

namespace CCPS {
    struct TileLinkParams {
        int CONFIG_ADDRESS;
        int ADDRESS;
        int ADDR_RANGE;
        int BEAT_BYTES{32};
        int CONFIG_BEAT_BYTES{8};
        int opcode_width{3};
        int param_width{3};
        int source_id_width{4};
        int sink_id_width{8};
        int address_width{64};
        int data_width{256};
        int mask_width{data_width/8};
        int size_width{static_cast<int>(log2Ceil(data_width/8)) + 4}; // 9 bits if 256
        int denied_width{1};
        int corrupt_width{1};
        int reserved_h2_width{5}; // width of the reserved bits in header 2

        TileLinkParams(
            int address,
            int address_range,
            int config_address,
            int inward_queue_depth,
            int outward_queue_depth
        ):
            CONFIG_ADDRESS(config_address), ADDRESS(address), ADDR_RANGE(address_range) {}
    };

    struct TLBundleAUnionD {
        Wire<UInt> opcode;  // I tiParams.opcodeWidth
        Wire<UInt> param;   // I tiParams.paramWidth
        Wire<UInt> size;    // I tiParams.sizeWidth
        Wire<UInt> source;  // I tiParams.sourceIDWidth
        Wire<UInt> sink;    // I tiParams.sinkIDWidth
        Wire<UInt> address; // I tiParams.addressWidth
        Wire<UInt> mask;    // I tiParams.maskWidth
        Wire<UInt> data;    // I tiParams.dataWidth
        Wire<UInt> msg_type;// I 4bits
    };
} // namespace CCPS

#endif // __TILELINK_COMMON_HPP__