#include "sideband/sb_msg_encoding.hpp"
#include "utils/cat.hpp"
#include <cassert>

namespace CCPS {
    Bool SBM::isComplete(const UInt &x) {
        return Bool(x == COMP_0 || x == COMP_32 || x == COMP_64);
    }

    Bool SBM::isMessage(const UInt &x) {
        return Bool(x == MSG_0 || x == MSG_64);
    }

    Bool SBM::isRequest(const UInt &x) {
        return Bool(x.toBigUInt(4) == 0);
    }

    BigUInt SBM::bitPatToBigUInt(BitPat x) {
        assert(x.size() <= sizeof(BigUInt)*8+1);
        if (x.size() <= 0) {
            std::cerr << "Error: BitPat is empty." << std::endl;
            exit(-1);
        }
        if (x[0] != 'b') {
            std::cerr << "Error: BitPat is not in binary or hex format." << std::endl;
            exit(-1);
        }
        BigUInt res = 0;
        for (int i = 1; i < x.size(); i++) {
            res <<= 1;
            if (x[i] == '1') {
                res |= 1;
            } else if (x[i] == '0') {
                res |= 0;
            } else if (x[i] == '?') {
                res |= 0;
            } else {
                std::cerr << "Error: BitPat contains invalid character." << std::endl;
                exit(-1);
            }
        }
        return res;
    }


    UInt SBMessage_factory(
        BitPat base,
        std::string src,
        bool remote,
        std::string dst,
        const UInt &data,
        const UInt &msgInfo
    ) {
        assert (data.size() == 64);
        assert (msgInfo.size() == 16);
        // take the bottom 64 bits of the base by modulo
        BitPat base_substr = std::string("b") + base.substr(base.size()-64, 64);
        BigUInt msg = SBM().bitPatToBigUInt(base.size() > (64+1) ? base_substr : base);
        BigUInt src_num = src == "Protocol_0" ? 0 : src == "Protocol_1" ? 4 : src == "D2D" ? 1 : src == "PHY" ? 2 : 0;
        BigUInt dst_num = dst == "Protocol_0" ? 0 : dst == "Protocol_1" ? 4 : dst == "D2D" ? 1 : dst == "PHY" ? 2 : 0;
        dst_num += remote ? 4 : 0;
        msg += src_num << 29;
        dst_num = (dst_num << 30) << 26;
        msg += dst_num;

        UInt res{0, 0};
        res.append(data);
        res.append((UInt(64, msg) | (msgInfo.operator << (32 + 8))));
        //std::cout << "exit " << __FUNCTION__ << std::endl;
        return res;
    }
} // namespace CCPS