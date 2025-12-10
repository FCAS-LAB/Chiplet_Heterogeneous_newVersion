#ifndef __PARITY_NEGOTIATION_SUBMODULE_HPP__
#define __PARITY_NEGOTIATION_SUBMODULE_HPP__
#include "utils/wire.hpp"
#include "utils/module.hpp"
#include "utils/reg.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"


namespace CCPS {

    struct ParityNegotiationSubmoduleIO {
        Wire<Bool> start_negotiation; // I
        Wire<Bool> negotiation_complete; // O

        Wire<UInt> parity_sb_rcv; // I
        Wire<UInt> parity_sb_snd; // O
        Wire<Bool> parity_sb_rdy; // I

        Wire<Bool> parity_tx_sw_en; // I
        Wire<Bool> parity_rx_sw_en; // I
        Wire<Bool> parity_rx_enable; // O
        Wire<Bool> parity_tx_enable; // O

        Wire<UInt> cycles_1us; // I
    };

    class ParityNegotiationSubmodule: public RegModule {
    public:
        ParityNegotiationSubmoduleIO io;
        ParityNegotiationSubmodule();

        void calcNextState() override;
    private:
        // ================= chisel signals =================
        RegPtr<Bool> _parity_req_snt_flag_reg;
        RegPtr<Bool> _parity_rsp_snt_flag_reg;
        RegPtr<Bool> _parity_req_rcv_flag_reg;
        RegPtr<Bool> _parity_rsp_rcv_flag_reg;
        RegPtr<Bool> _parity_rx_enable_reg;
        RegPtr<Bool> _parity_tx_enable_reg;

        RegPtr<UInt> _parity_req_timeout_counter_reg;

        // ================= helper signals =================
        Wire<Bool>::TPFUNC _io_negoriation_complete;
        Wire<UInt>::TPFUNC _io_parity_sb_snd;
        Wire<Bool>::TPFUNC _io_parity_rx_enable;
        Wire<Bool>::TPFUNC _io_parity_tx_enable;

        Wire<Bool> _reqcomplete;
        Wire<Bool> _rspcomplete;
        Wire<UInt> _timeout;
    };
} // namespace CCPS





#endif // __PARITY_NEGOTIATION_SUBMODULE_HPP__