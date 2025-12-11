#include "e2e/ccps_top.hpp"
#include "protocol/hamming_code.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include "protocol/common.hpp"
#include "utils/time_slice.hpp"
#include <gtest/gtest.h>
#include <stdio.h>
#include <vector>

using namespace CCPS;

// send LSB first
class CCPSRawpayloadGenerator {
    public:
        CCPSRawpayloadGenerator(CCPSRawPayloadFormat& tx_payload, const ProtocolLayerParams &proto_params):
                _tx_payload(tx_payload)
        {
            // connect hamming encoder
            _hamming_encode = std::make_shared<HammingEncode>(proto_params);

            _tx_bits_without_ecc = tx_payload.toUIntWithOutEcc();
            _hamming_encode->io.data = [&]() -> UInt {
                return _tx_bits_without_ecc;
            };
            tx_payload.ecc = _hamming_encode->io.checksum;
            _ecc = static_cast<int>(tx_payload.ecc().toBigUInt());
            _tx_bits = tx_payload.toUInt();
            std::cout << "-----------------> sending payload: " << std::endl << tx_payload
                      << ", bits width " << std::dec << _tx_bits.size()
                      << ", cmd width " << _tx_payload.cmd.toUInt().size()
                      << ", header1 width " << _tx_payload.header1.toUInt().size()
                      << ", header2 width " << _tx_payload.header2.toUInt().size()
                      //<< ", ecc width " << _tx_payload.ecc().size()
                      << std::endl;

            char file_path[256];
            sprintf(file_path, "./tx_payload_%u.txt", _tx_bits.size());
            tx_payload.dumpData(file_path);
            assert(_tx_bits.size() % tx_payload.data[0]().size() == 0);
        }

        UInt getTxChunk() {
            int start = _tx_base;
            int end = _tx_base + _chunk_bits > _tx_bits.size() ? _tx_bits.size() : _tx_base + _chunk_bits;
            _tx_base += _chunk_bits;
            auto chunk = _tx_bits(end - 1, start);
            //std::cout << "gotTxChunk, start " << start << " end " << end << " chunk: 0x" << chunk.toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase) << std::endl;
            return chunk;
        }

        bool isEmpty() {
            //std::cout << "_tx_base " << _tx_base << std::endl;
            return _tx_base >= _tx_bits.size();
        }

        bool isFull() {
            //std::cout << "_tx_bits.size() " << _tx_bits.size() << " _rx_bits.size() " << _rx_bits.size() << std::endl;
            return _tx_bits.size() <= _rx_bits.size();
        }

        void setRxChunk(UInt chunk) {
            //std::cout << "chunk: 0x" << chunk.toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase) << std::endl;
            _rx_bits = chunk.append(_rx_bits);
        }

        bool check() {
            if (_tx_bits.size() > _rx_bits.size()) {
                std::cout << "compare failed, rx bits size " << _rx_bits.size()
                          << " tx bits size " << _tx_bits.size() << std::endl;
                return false;
            }

            size_t rx_offset = _rx_bits.size() - _tx_bits.size();
            for (size_t i = 0; i < _tx_bits.size(); i++) {
                BigUInt tx_bit = _tx_bits(i).toBigUInt();
                BigUInt rx_bit = _rx_bits(i).toBigUInt();
                if (tx_bit != rx_bit) {
                    std::cout << "compare failed, i " << i
                              << " rx_bits[" << i << "] " << rx_bit
                              << " tx bits[" << i << "] " << tx_bit << std::endl;
                    return false;
                }
            }
            std::cout << "compare success!!!" << std::endl;
            return true;
        }

        void toRxPayload() {
            int b = 0;
            _rx_payload.cmd.msg_type.capture(4, _rx_bits(b+4-1, b).toBigUInt()); b += 4;
            _rx_payload.cmd.host_id.capture(_tx_payload.cmd.host_id().size(), _rx_bits(b+_tx_payload.cmd.host_id().size()-1, b).toBigUInt()); b += _tx_payload.cmd.host_id().size();
            _rx_payload.cmd.partner_id.capture(_tx_payload.cmd.partner_id().size(), _rx_bits(b+_tx_payload.cmd.partner_id().size()-1, b).toBigUInt()); b += _tx_payload.cmd.partner_id().size();
            _rx_payload.cmd.tl_a_credit.capture(_tx_payload.cmd.tl_a_credit().size(), _rx_bits(b+_tx_payload.cmd.tl_a_credit().size()-1, b).toBigUInt()); b += _tx_payload.cmd.tl_a_credit().size();
            _rx_payload.cmd.tl_b_credit.capture(_tx_payload.cmd.tl_b_credit().size(), _rx_bits(b+_tx_payload.cmd.tl_b_credit().size()-1, b).toBigUInt()); b += _tx_payload.cmd.tl_b_credit().size();
            _rx_payload.cmd.tl_c_credit.capture(_tx_payload.cmd.tl_c_credit().size(), _rx_bits(b+_tx_payload.cmd.tl_c_credit().size()-1, b).toBigUInt()); b += _tx_payload.cmd.tl_c_credit().size();
            _rx_payload.cmd.tl_d_credit.capture(_tx_payload.cmd.tl_d_credit().size(), _rx_bits(b+_tx_payload.cmd.tl_d_credit().size()-1, b).toBigUInt()); b += _tx_payload.cmd.tl_d_credit().size();
            _rx_payload.cmd.tl_e_credit.capture(_tx_payload.cmd.tl_e_credit().size(), _rx_bits(b+_tx_payload.cmd.tl_e_credit().size()-1, b).toBigUInt()); b += _tx_payload.cmd.tl_e_credit().size();
            _rx_payload.cmd.reserved_cmd.capture(_tx_payload.cmd.reserved_cmd().size(), _rx_bits(b+_tx_payload.cmd.reserved_cmd().size()-1, b).toBigUInt()); b += _tx_payload.cmd.reserved_cmd().size();

            _rx_payload.header1.address.capture(_tx_payload.header1.address().size(), _rx_bits(b+_tx_payload.header1.address().size()-1, b).toBigUInt()); b += _tx_payload.header1.address().size();

            _rx_payload.header2.opcode.capture(_tx_payload.header2.opcode().size(), _rx_bits(b+_tx_payload.header2.opcode().size()-1, b).toBigUInt()); b += _tx_payload.header2.opcode().size();
            _rx_payload.header2.param.capture(_tx_payload.header2.param().size(), _rx_bits(b+_tx_payload.header2.param().size()-1, b).toBigUInt()); b += _tx_payload.header2.param().size();
            _rx_payload.header2.size.capture(_tx_payload.header2.size().size(), _rx_bits(b+_tx_payload.header2.size().size()-1, b).toBigUInt()); b += _tx_payload.header2.size().size();
            _rx_payload.header2.source.capture(_tx_payload.header2.source().size(), _rx_bits(b+_tx_payload.header2.source().size()-1, b).toBigUInt()); b += _tx_payload.header2.source().size();
            _rx_payload.header2.sink.capture(_tx_payload.header2.sink().size(), _rx_bits(b+_tx_payload.header2.sink().size()-1, b).toBigUInt()); b += _tx_payload.header2.sink().size();
            _rx_payload.header2.mask.capture(_tx_payload.header2.mask().size(), _rx_bits(b+_tx_payload.header2.mask().size()-1, b).toBigUInt()); b += _tx_payload.header2.mask().size();
            _rx_payload.header2.reservedh2.capture(_tx_payload.header2.reservedh2().size(), _rx_bits(b+_tx_payload.header2.reservedh2().size()-1, b).toBigUInt()); b += _tx_payload.header2.reservedh2().size();

            _rx_payload.data.resize(_tx_payload.data.size());
            for (size_t i = _tx_payload.data.size(); i != 0; i--) {
                _rx_payload.data[i-1].capture(_tx_payload.data[i-1]().size(), _rx_bits(b+_tx_payload.data[i-1]().size()-1, b).toBigUInt()); b += _tx_payload.data[i-1]().size();
            }

            _rx_payload.ecc.capture(_tx_payload.ecc().size(), _rx_bits(b+_tx_payload.ecc().size()-1, b).toBigUInt());

            std::cout << "-----------------> rcv payload: " << std::endl << _rx_payload << std::endl;

            char file_path[256];
            sprintf(file_path, "./rx_payload_%u.txt", _tx_bits.size());
            _rx_payload.dumpData(file_path);
        }

        int getEcc() {
            return _ecc;
        }

    private:
        UInt _tx_bits;
        UInt _tx_bits_without_ecc;
        UInt _rx_bits;
        const int _chunk_bits = 64 * 8;
        int _tx_base = 0;
        const CCPSRawPayloadFormat& _tx_payload;
        CCPSRawPayloadFormat _rx_payload;
        ModulePtr<HammingEncode> _hamming_encode;
        ModulePtr<HammingDecode> _hamming_decode;
        int _ecc;
};

class CCPSTopTest: public RegModule {
public:
    struct {
        Wire<Bool> fdi_lp_stall_ack; // O. When this signal is high, the link is stall, and all transaction should stop.
        Wire<UInt> tl_pl_state_status; // O 4bits, debug signal, should be ignored.

        Wire<Bool> tl_lp_data_valid;    // I
        Wire<UInt> tl_lp_data_bits;     // I 8 * fdiParams.width
        Wire<Bool> tl_lp_data_irdy;     // I
        Wire<Bool> tl_lp_data_ready;    // O
        Wire<UInt> tl_pl_data_bits;     // O 8 * fdiParams.width
        Wire<Bool> tl_pl_data_valid;    // O
    } io;
    CCPSTopTest(const FdiParams &fdi_params, const RdiParams &rdi_params,
        const SidebandParams &sb_params, BigUInt my_id,
        const LinkTrainingParams &link_training_params, const AfeParams &afe_params,
        const AsyncQueueParams &lane_async_queue_params)
    {
        // Instantiate
        _top = createSubmodule<CCPSTop>("ccps_top", fdi_params, rdi_params, sb_params, my_id,
            link_training_params, afe_params, lane_async_queue_params);

        // connect
        _top->io.fdi_lp_config_credit.capture(false);               // useless
        _top->io.fdi_pl_config.assignValid(false);                  // useless
        _top->io.fdi_pl_config.assignBits(fdi_params.sb_width, 0);  // useless

        io.fdi_lp_stall_ack = _top->io.fdi_lp_stall_ack;
        io.tl_pl_state_status = _top->io.tl_pl_state_status;
        _top->io.tl_lp_data_valid = io.tl_lp_data_valid;
        _top->io.tl_lp_data_bits = io.tl_lp_data_bits;
        _top->io.tl_lp_data_irdy = io.tl_lp_data_irdy;
        io.tl_lp_data_ready = _top->io.tl_lp_data_ready;
        io.tl_pl_data_bits = _top->io.tl_pl_data_bits;
        io.tl_pl_data_valid = _top->io.tl_pl_data_valid;
        _top->io.tl_ready_to_rcv.capture(true);
        _top->io.fault.capture(false);
        _top->io.soft_reset.capture(false);

        for (size_t i = 0; i < _top->io.mb_afe.tx_data.size(); i++) {
            _top->io.mb_afe.rx_data[i].connect(_top->io.mb_afe.tx_data[i]);
        }
        _top->io.mb_afe.fifo_params.reset = getReset();
        _top->io.mb_afe.pll_lock.capture(true);

        _top->io.sb_afe.rx_data = _top->io.sb_afe.tx_data;
        _top->io.sb_afe.fifo_params.reset = getReset();
        _top->io.sb_afe.pll_lock.capture(true);

        _top->setReset(getReset());
    }

    bool propagateClock() override {
        std::cout << "in " << getPathName() << " propagateClock" << std::endl;
        bool success = true;
        _top->io.mb_afe.fifo_params.clk = getClock();
        _top->io.sb_afe.fifo_params.clk = getClock();

        success &= Module::propagateClock();

        if (_top->io.sb_afe.tx_clock != nullptr) {
            _top->io.sb_afe.rx_clock = _top->io.sb_afe.tx_clock;
        } else {
            success = false;
        }

        return success;
    }

    //void calcNextState() override {
    //    std::cout << std::endl;
    //    std::cout << "in CCPSTopTest" << std::endl;
    //    std::cout << "io.fdi_lp_stall_ack " << static_cast<bool>(io.fdi_lp_stall_ack()) << std::endl;
    //    std::cout << "io.tl_pl_state_status " << io.tl_pl_state_status().toBigUInt() << std::endl;
    //    std::cout << "io.tl_lp_data_valid " << static_cast<bool>(io.tl_lp_data_valid()) << std::endl;
    //    std::cout << "io.tl_lp_data_bits " << io.tl_lp_data_bits().toBigUInt() << std::endl;
    //    std::cout << "io.tl_lp_data_irdy " << static_cast<bool>(io.tl_lp_data_irdy()) << std::endl;
    //    std::cout << "io.tl_lp_data_ready " << static_cast<bool>(io.tl_lp_data_ready()) << std::endl;
    //    std::cout << "io.tl_pl_data_bits " << io.tl_pl_data_bits().toBigUInt() << std::endl;
    //    std::cout << "io.tl_pl_data_valid " << static_cast<bool>(io.tl_pl_data_valid()) << std::endl;
    //}
private:
    ModulePtr<CCPSTop> _top;
};

// IOs

static bool val_tl_lp_data_valid = false;
static BigUInt val_tl_lp_data_bits = 0;
static bool val_tl_lp_data_irdy = false;

bool run(CCPSTopTest &c, const int data_chunks, const ProtocolLayerParams &proto_params, const
        TileLinkParams &tl_params) {
    std::cout << "================ running " << std::dec << data_chunks << " chunks =================" << std::endl;
    CCPSRawPayloadFormat tx_payload;

    tx_payload.cmd.msg_type.capture(4, 0x1);
    tx_payload.cmd.host_id.capture(proto_params.host_id_width, 0x1);
    tx_payload.cmd.partner_id.capture(proto_params.partner_id_width, 0x2);
    tx_payload.cmd.tl_a_credit.capture(proto_params.credit_width, 0xf);
    tx_payload.cmd.tl_b_credit.capture(proto_params.credit_width, 0xf);
    tx_payload.cmd.tl_c_credit.capture(proto_params.credit_width, 0xf);
    tx_payload.cmd.tl_d_credit.capture(proto_params.credit_width, 0xf);
    tx_payload.cmd.tl_e_credit.capture(proto_params.credit_width, 0xf);
    tx_payload.cmd.reserved_cmd.capture(proto_params.reserved_cmd_width, 0xf);

    tx_payload.header1.address.capture(tl_params.address_width, 0xa5a5a5a5);

    tx_payload.header2.opcode.capture(tl_params.opcode_width, 0x1);
    tx_payload.header2.param.capture(tl_params.param_width, 0x2);
    tx_payload.header2.size.capture(tl_params.size_width, data_chunks);
    tx_payload.header2.source.capture(tl_params.source_id_width, 0x4);
    tx_payload.header2.sink.capture(tl_params.sink_id_width, 0x5);
    tx_payload.header2.mask.capture(tl_params.mask_width, 0x6);
    tx_payload.header2.reservedh2.capture(tl_params.reserved_h2_width, 0x7);

    tx_payload.data.resize(data_chunks);
    std::vector<BigUInt> data_vec(data_chunks);
    BigUInt data_seed = 0x12345689;
    std::cout << "proto_params.ccps_filt_width " << std::dec << proto_params.ccps_filt_width << std::endl;
    std::cout << "sizeof(BigUInt) " << std::dec << sizeof(BigUInt) << std::endl;
    for (size_t j = 0; j < tx_payload.data.size(); j++) {
        auto& data = data_vec[j];
        data = 0;
        for (size_t i = 0; i < proto_params.ccps_filt_width * 2 / 8; i++) {
            data |= (data_seed << (i * 8));
        }
        //std::cout << "seting data j " << j << " to " << data.str(0, std::ios_base::hex | std::ios_base::uppercase) << " " << data << std::endl;
        data_seed++;
        tx_payload.data[j].capture(proto_params.ccps_filt_width, data);
    }
    //tx_payload.ecc.capture(proto_params.ccps_ecc_width, 0x55);

    CCPSRawpayloadGenerator generator(tx_payload, proto_params);

    bool sending_packet_head = true;
    static int send_packet_count = 0;
    auto &pi_manager = PacketInfoManager::GetInstance();
    while (!generator.isFull()) {
        // send
        if (generator.isEmpty() == false) {
            val_tl_lp_data_valid = true;
            val_tl_lp_data_irdy = true;
            if (c.io.tl_lp_data_ready()) {
                val_tl_lp_data_bits = generator.getTxChunk().toBigUInt();
                if (sending_packet_head) {
                    std::cout << "sending packet " << send_packet_count << std::endl;
                    pi_manager.addPacket({send_packet_count, 0, generator.getEcc(), false});
                } else {
                    pi_manager.incTagIndex();
                }
                sending_packet_head = false;
            }

            if (generator.isEmpty() == true) {
                pi_manager.setFinished();
                sending_packet_head = true;
                send_packet_count++;
            }
        } else {
            val_tl_lp_data_valid = false;
            val_tl_lp_data_irdy = false;
        }

        // receive
        if (c.io.tl_pl_data_valid()) {
            generator.setRxChunk(c.io.tl_pl_data_bits());
            auto packet_info = pi_manager.peek();
            std::cout << "received packet " << packet_info.packet_index << " " << packet_info.tag_index << std::endl;
        }

        std::cout << "sending head " << sending_packet_head
                  << " packet_idx " << send_packet_count
                  << " val_tl_lp_data_valid " << val_tl_lp_data_valid
                  << " c.io.tl_lp_data_ready() " << static_cast<bool>(c.io.tl_lp_data_ready())
                  << " finish " << pi_manager.peek().finished
                  << std::endl;

        c.step();
    }
    val_tl_lp_data_valid = false;
    val_tl_lp_data_irdy = false;

    pi_manager.popPacket();

    generator.toRxPayload();
    return generator.check();
}

TEST (CCPSTopTest, LoopbackTest) {
    const ProtocolLayerParams proto_params;
    const TileLinkParams tl_params {0, 0xffff, 0x40000, 8, 8};
    const FdiParams fdi_params(64, 64, 32);
    const RdiParams rdi_params(64, 32);
    const SidebandParams sb_params;
    const int my_id = 1;
    const LinkTrainingParams link_training_params;
    const AfeParams afe_params;
    const AsyncQueueParams lane_async_queue_params;

    const int CLOCK_CYCLE = 100'000;

    //
    auto top = createTopModule<CCPSTopTest>(fdi_params, rdi_params, sb_params, my_id,
        link_training_params, afe_params, lane_async_queue_params);
    auto &c = *top;
    //TimeSlice::getInstance().showClockTree();

    // This module needs reset
    Wire<Bool> reset;
    reset.capture(false);
    c.setReset(reset);

    // IOs
    c.io.tl_lp_data_valid.capture(val_tl_lp_data_valid);
    c.io.tl_lp_data_bits.capture(8 * fdi_params.width, val_tl_lp_data_bits);
    c.io.tl_lp_data_irdy.capture(val_tl_lp_data_irdy);


    // run
    c.step();
    while (c.io.fdi_lp_stall_ack()) {
        std::cout << "waiting for fdi_lp_stall_ack" << std::endl;
        c.step();
    }

    assert (proto_params.ccps_filt_width == 64);
    const std::vector<int> expected_data_bytes = {8, 16, 96, 136, 144, 776, 784, 800, 1016, 1024, 56, 72, 104, 792, 992};
    for (const auto expected: expected_data_bytes) {
        int actual_chunks = expected * 8 / proto_params.ccps_filt_width;
        int actual_bytes = actual_chunks * proto_params.ccps_filt_width / 8;
        std::cout << std::dec << "expected data bytes " << expected << ", rounded data bytes " << actual_bytes << std::endl;
        run(c, actual_chunks, proto_params, tl_params);
    }

    c.step();
}
