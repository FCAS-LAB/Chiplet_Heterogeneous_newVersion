#include "protocol/common.hpp"
#include <fstream>
#include <stdio.h>

namespace CCPS {

    UInt CCPSCmdFormat::toUInt() const {
        UInt bits;
        bits.append(reserved_cmd()).append(tl_e_credit()).append(tl_d_credit())
            .append(tl_c_credit()).append(tl_b_credit()).append(tl_a_credit())
            .append(partner_id()).append(host_id()).append(msg_type());
        return bits;
    }

    std::ostream& operator<<(std::ostream& os, const CCPSCmdFormat& obj) {
        os << "-- CCPSCmdFormat --,";
        os << " msg_type: 0x" << std::hex << obj.msg_type().toBigUInt();
        os << " host_id: 0x" << std::hex << obj.host_id().toBigUInt();
        os << " partner_id: 0x" << std::hex << obj.partner_id().toBigUInt();
        os << " tl_a_credit: 0x" << std::hex << obj.tl_a_credit().toBigUInt();
        os << " tl_b_credit: 0x" << std::hex << obj.tl_b_credit().toBigUInt();
        os << " tl_c_credit: 0x" << std::hex << obj.tl_c_credit().toBigUInt();
        os << " tl_d_credit: 0x" << std::hex << obj.tl_d_credit().toBigUInt();
        os << " tl_e_credit: 0x" << std::hex << obj.tl_e_credit().toBigUInt();
        os << " reserved_cmd: 0x" << std::hex << obj.reserved_cmd().toBigUInt();
        return os;
    }

    UInt CCPSHeader1Format::toUInt() const {
        UInt bits;
        bits.append(address());
        return bits;
    }

    std::ostream& operator<<(std::ostream& os, const CCPSHeader1Format& obj) {
        os << "-- CCPSHeader1Format --,";
        os << " address: 0x" << std::hex << obj.address().toBigUInt();
        return os;
    }

    UInt CCPSHeader2Format::toUInt() const {
        UInt bits;
        bits.append(reservedh2()).append(mask()).append(sink()).append(source())
            .append(size()).append(param()).append(opcode());
        return bits;
    }

    std::ostream& operator<<(std::ostream& os, const CCPSHeader2Format& obj) {
        os << "-- CCPSHeader2Format --,";
        os << " opcode: 0x" << std::hex << obj.opcode().toBigUInt();
        os << " param: 0x" << std::hex << obj.param().toBigUInt();
        os << " size: 0x" << std::hex << obj.size().toBigUInt();
        os << " source: 0x" << std::hex << obj.source().toBigUInt();
        os << " sink: 0x" << std::hex << obj.sink().toBigUInt();
        os << " mask: 0x" << std::hex << obj.mask().toBigUInt();
        os << " reservedh2: 0x" << std::hex << obj.reservedh2().toBigUInt();
        return os;
    }

    UInt CCPSRawPayloadFormat::toUInt() const {
        UInt bits = ecc();
        for (auto &d : data) {
            bits.append(d());
        }
        bits.append(header2.toUInt()).append(header1.toUInt()).append(cmd.toUInt());
        return bits;
    }

    UInt CCPSRawPayloadFormat::toUIntWithOutEcc() const {
        UInt bits;
        for (auto &d : data) {
            bits.append(d());
        }
        bits.append(header2.toUInt()).append(header1.toUInt()).append(cmd.toUInt());
        return bits;
    }


    void CCPSRawPayloadFormat::dumpData(const std::string& file_path) const {
        std::ofstream outfile(file_path);

        char m[256];
        int base = 0;
        for (auto &d : data) {
            sprintf(m, "0x%08X: 0x%016lX\n", base, static_cast<uint64_t>(d().toBigUInt()));
            outfile << m;
            base += d().size() / 8;
        }
        outfile.close();
    }

    std::ostream& operator<<(std::ostream& os, const CCPSRawPayloadFormat& obj) {
        os << obj.cmd << std::endl;
        os << obj.header1 << std::endl;
        os << obj.header2 << std::endl;
        //for (size_t i = 0; i < obj.data.size(); i++) {
        //    os << " data[" << i << "]: 0x" << std::hex << obj.data[i]().toBigUInt() << std::endl;
        //}
        os << " ecc: 0x" << std::hex << obj.ecc().toBigUInt() << std::endl;
        return os;
    }



} // namespace CCPS