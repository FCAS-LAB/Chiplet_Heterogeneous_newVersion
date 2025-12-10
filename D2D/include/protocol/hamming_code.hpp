#ifndef __HAMMING_CODE_HPP__
#define __HAMMING_CODE_HPP__

#include "utils/wire.hpp"
#include "utils/decoupled.hpp"
#include "utils/reg.hpp"
#include "utils/module.hpp"
#include "protocol/common.hpp"

namespace CCPS {

    // ============= HammingEncode ================
    class HammingEncode: public WireModule {
    public:
        struct {
            Wire<UInt> data;    // I protoParams.ccpsNonEccWidth
            Wire<UInt> checksum;    // O protoParams.ccpsEccWidth
        } io;

        HammingEncode(const ProtocolLayerParams& proto_params);

    private:
        // ------- chisel signals -------
        ProtocolLayerParams _proto_params;
        BigUInt _hammingEncode(BigUInt data);
        BigUInt _genSingleEncode(BigUInt data, int index);
    };

    // ============= HammingDecode ================
    class HammingDecode: public WireModule {
    public:
        struct {
            Wire<UInt> data;    // I protoParams.ccpsNonEccWidth
            Wire<UInt> checksum;    // I protoParams.ccpsEccWidth
            Wire<Bool> matches; // O
        } io;

        HammingDecode(const ProtocolLayerParams& proto_params);

    private:
        ModulePtr<HammingEncode> _hamming_encode;
    };
} // namespace CCPS

#endif // __HAMMING_CODE_HPP__