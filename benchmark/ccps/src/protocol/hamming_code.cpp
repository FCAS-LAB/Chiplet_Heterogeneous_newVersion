#include "protocol/hamming_code.hpp"

namespace CCPS {

    // ============= HammingEncode ================
    HammingEncode::HammingEncode(const ProtocolLayerParams& proto_params): _proto_params(proto_params) {
        io.checksum = [this]() -> UInt {
            return UInt(_proto_params.ccps_ecc_width, _hammingEncode(io.data().toBigUInt()));
        };
    }

    BigUInt HammingEncode::_hammingEncode(BigUInt data) {
        int tags[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
        BigUInt res = 0;
        for (size_t i = 0; i < sizeof(tags) / sizeof(tags[0]); i++) {
            auto temp = _genSingleEncode(data, tags[i]);
            res = (res << 1) | temp;
        }
        return res;
    }

    BigUInt HammingEncode::_genSingleEncode(BigUInt data, int index) {
        BigUInt res = 0;
        for (int i = 0; i < _proto_params.ccps_non_ecc_width; i++) {
            BigUInt temp_res = 0;
            if (i % (2 * index) >= index) {
                temp_res = (data >> i) & 0x1;
            }
            res ^= temp_res;
        }
        return res;
    }

    // ============= HammingDecode ================
    HammingDecode::HammingDecode(const ProtocolLayerParams& proto_params) {
        // Instaitiate
        _hamming_encode = createSubmodule<HammingEncode>("hamming_encode", proto_params);

        // connect
        _hamming_encode->io.data = io.data;
        io.matches = [this] () -> Bool {
            return Bool(_hamming_encode->io.checksum() == io.checksum());
        };
    }
} // namespace CCPS