#ifndef __LINK_MANAGEMENT_CONTROLLER_HPP__
#define __LINK_MANAGEMENT_CONTROLLER_HPP__


#include "interfaces/fdi.hpp"
#include "interfaces/rdi.hpp"
#include "sideband/sideband_io.hpp"
#include "d2dadapter/link_disabled_submodule.hpp"
#include "d2dadapter/link_reset_submodule.hpp"
#include "d2dadapter/link_init_submodule.hpp"
#include "d2dadapter/parity_negotiation_submodule.hpp"
#include "utils/wire.hpp"
#include "utils/module.hpp"
#include "utils/counter.hpp"

namespace CCPS {

    struct LinkManagementControllerIO {
        Wire<UInt> fdi_lp_state_req; // I
        Wire<Bool> fdi_lp_linkerror; // I
        Wire<Bool> fdi_lp_rx_active_sts; // I
        Wire<UInt> fdi_pl_state_sts; // O
        Wire<Bool> fdi_pl_rx_active_req; // O
        Wire<Bool> fdi_pl_inband_pres; // O
        Wire<Bool> rdi_lp_linkerror; // O
        Wire<UInt> rdi_lp_state_req; // O  // 4bits
        Wire<UInt> rdi_pl_state_sts; // I  // 4bits
        Wire<Bool> rdi_pl_inband_pres; // I
        Wire<UInt> sb_snd; // O
        Wire<UInt> sb_rcv; // I
        Wire<Bool> sb_rdy; // I
        Wire<Bool> linkmgmt_stallreq; // O
        Wire<Bool> linkmgmt_stalldone; // I
        Wire<UInt> cycles_1us; // I     // 32bits
        Wire<Bool> parity_tx_sw_en; // I
        Wire<Bool> parity_rx_sw_en; // I
        Wire<Bool> parity_rx_enable; // O
        Wire<Bool> parity_tx_enable; // O
    };

    class LinkManagementController: public RegModule {
    public:
        LinkManagementControllerIO io;
        LinkManagementController(
            const FdiParams &fdi_params,
            const RdiParams &rdi_params,
            const SidebandParams &sb_params
        );

        void calcNextState() override;

    private:
        // ================ chisel signals =================
        ModulePtr<LinkDisabledSubmodule> _disabled_submodule;
        ModulePtr<LinkResetSubmodule> _linkreset_submodule;
        ModulePtr<LinkInitSubmodule> _linkinit_submodule;
        ModulePtr<ParityNegotiationSubmodule> _parity_negotiation_submodule;
        RegPtr<Bool> _rdi_lp_linkerror_reg;
        RegPtr<UInt> _rdi_lp_state_req_reg;
        RegPtr<Bool> _fdi_pl_rxactive_req_reg;
        RegPtr<Bool> _fdi_pl_inband_pres_reg;
        RegPtr<Bool> _linkmgmt_stallreq_reg;
        RegPtr<UInt> _fdi_lp_state_req_prev_reg;
        RegPtr<UInt> _link_state_reg;

        // ================ helper signals =================
        //Wire<UInt>::TPFUNC _io_fdi_pl_state_sts;
        //Wire<Bool>::TPFUNC _io_fdi_pl_rx_active_req;
        //Wire<Bool>::TPFUNC _io_fdi_pl_inband_pres;
        //Wire<Bool>::TPFUNC _io_rdi_lp_linkerror;
        //Wire<UInt>::TPFUNC _io_rdi_lp_state_req;
        Wire<UInt>::TPFUNC _io_sb_snd;
        //Wire<Bool>::TPFUNC _io_linkmgmt_stallreq;
        //Wire<Bool>::TPFUNC _io_parity_rx_enable;
        //Wire<Bool>::TPFUNC _io_parity_tx_enable;

        Wire<Bool> _disabled_sb_rdy;
        Wire<Bool> _linkreset_sb_rdy;
        Wire<Bool> _linkinit_sb_rdy;
        Wire<Bool> _parity_negotiation_sb_rdy;
        Wire<Bool> _linkerror_phy_sts;
        Wire<Bool> _stallhandler_handshake_done;
        Wire<Bool> _rx_deactive;
        Wire<Bool> _rx_active;
        Wire<Bool> _retrain_phy_sts;

        ////Wire<UInt>::TPFUNC _disabled_submodule_io_fdi_lp_state_req_prev;
        //Wire<Bool> _disabled_sb_rdy;
        //Wire<Bool> _linkreset_sb_rdy;
        //Wire<Bool> _linkinit_sb_rdy;
        //Wire<Bool> _parity_negotiation_sb_rdy;
        //Wire<Bool> _linkerror_phy_sts;
        //Wire<Bool> _stallhandler_handshake_done;
        //Wire<Bool> _rx_deactive;
        //Wire<Bool> _rx_active;
        //Wire<Bool> _retrain_phy_sts;
        ModulePtr<Counter> _debug_count;
        Wire<Bool> _debug_count_en;
    };

} // namespace CCPS


#endif // __LINK_MANAGEMENT_CONTROLLER_HPP__