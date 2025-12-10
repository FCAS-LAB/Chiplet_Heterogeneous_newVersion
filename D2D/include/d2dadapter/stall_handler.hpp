#ifndef __STALL_HANDLER_HPP__
#define __STALL_HANDLER_HPP__
#include "utils/wire.hpp"
#include "utils/module.hpp"
#include "utils/reg.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"


namespace CCPS {
    // ========================== FDIStallHandler ===============================
    struct FDIStallHandlerIO {
        Wire<Bool> linkmgmt_stallreq; // I
        Wire<Bool> linkmgmt_stalldone; // O
        Wire<Bool> fdi_pl_stallreq; // O
        Wire<Bool> fdi_lp_stallack; // I
    };


    class FDIStallHandler: public RegModule {
    public:
        FDIStallHandlerIO io;
        FDIStallHandler();

        void calcNextState() override;
    private:
        // ================= chisel signals =================
        RegPtr<Bool> _fdi_lp_stallreq_reg;
        RegPtr<Bool> _linkmgmt_stalldone_reg;
        RegPtr<UInt> _stall_handshake_state_reg;

        // ================= helper signals =================
        Wire<Bool>::TPFUNC _io_linkmgmt_stalldone;
        Wire<Bool>::TPFUNC _io_fdi_pl_stallreq;
    };

    // ========================== RDIStallHandler ===============================
    struct RDIStallHandlerIO {
        Wire<Bool> mainband_stallreq; // O
        Wire<Bool> mainband_stalldone; // I
        Wire<Bool> rdi_pl_stallreq; // I
        Wire<Bool> rdi_lp_stallack; // O
    };

    class RDIStallHandler: public RegModule {
    public:
        RDIStallHandlerIO io;
        RDIStallHandler();

        void calcNextState() override;

    private:
        // ================= chisel signals =================
        RegPtr<Bool> _rdi_lp_stallack_reg;
        RegPtr<Bool> _mainband_stallreq_reg;
        RegPtr<UInt> _stall_handshake_state_reg;

        // ================= helper signals =================
        Wire<Bool>::TPFUNC _io_mainband_stallreq;
        Wire<Bool>::TPFUNC _io_rdi_lp_stallack;
    };
} // namespace CCPS





#endif // __STALL_HANDLER_HPP__