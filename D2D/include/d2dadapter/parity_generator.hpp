#ifndef __PARITY_GENERATOR_HPP__
#define __PARITY_GENERATOR_HPP__

#include "utils/wire.hpp"
#include "utils/module.hpp"
#include "interfaces/fdi.hpp"
#include <vector>

namespace CCPS {

    struct ParityGeneratorIO {
        std::vector<Wire<UInt>> snd_data;   // I
        Wire<Bool> snd_data_vld; // I
        std::vector<Wire<UInt>> rcv_data;   // I
        Wire<Bool> rcv_data_vld; // I

        std::vector<Wire<UInt>> parity_data; // O
        Wire<Bool> parity_insert; // O
        Wire<Bool> parity_check; // O
        Wire<Bool> parity_rdy; // I

        std::vector<Wire<Bool>> parity_check_result; // O
        Wire<Bool> parity_check_result_valid; // O
        Wire<UInt> rdi_state; // I

        Wire<Bool> parity_rx_enable; // I
        Wire<Bool> parity_tx_enable; // I
        Wire<UInt> parity_n; // I
    };

    class ParityGenerator: public RegModule {
    public:
        ParityGeneratorIO io;
        ParityGenerator(const FdiParams &fdi_params);

        void calcNextState() override;

    private:
        // ================= chisel signals =================
        std::vector<RegPtr<Bool>> _parity_data_snd_reg; // all parity data
        std::vector<RegPtr<Bool>> _parity_data_rcv_reg; // all parity data

        RegPtr<UInt> _parity_dcount_snd_reg; // number of data has sent by the protocol
        RegPtr<UInt> _parity_pcount_snd_reg; // number of parity has sent
        RegPtr<UInt> _parity_dcount_rcv_reg; // number of data has received from the phy
        RegPtr<UInt> _parity_pcount_rcv_reg; //num of parity has checked

        RegPtr<Bool> _parity_check_result_valid_reg;

        std::vector<RegPtr<Bool>> _parity_check_bits_reg;

        // ================= helper signals =================
        const FdiParams _fdi_params;
        std::vector<Wire<UInt>::TPFUNC> _io_parity_data;
        Wire<Bool>::TPFUNC _io_parity_insert;
        Wire<Bool>::TPFUNC _io_parity_check;

        std::vector<Wire<Bool>::TPFUNC> _io_parity_check_result;
        Wire<Bool>::TPFUNC _io_parity_check_result_valid;

        Wire<UInt> _n_64;
        Wire<UInt> _n_256_256;


    };
} // namespace CCPS

#endif // __PARITY_GENERATOR_HPP__