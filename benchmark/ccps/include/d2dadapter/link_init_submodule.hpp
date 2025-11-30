#ifndef __D2DADAPTER_LINK_INIT_SUBMODULE_HPP__
#define __D2DADAPTER_LINK_INIT_SUBMODULE_HPP__

#include "utils/wire.hpp"
#include "utils/module.hpp"
#include "utils/reg.hpp"

namespace CCPS {

    struct LinkInitSubmoduleIO {
        Wire<UInt> fdi_lp_state_req; //I
        Wire<UInt> fdi_lp_state_req_prev; //I
        //Wire<Bool> fdi_lp_inband_pres; //I

        Wire<Bool> fdi_lp_rxactive_sts; //I

        Wire<Bool> linkinit_fdi_pl_inband_pres; //O
        Wire<Bool> linkinit_fdi_pl_rxactive_req; //O
        Wire<UInt> linkinit_fdi_pl_state_sts; //O

        Wire<UInt> rdi_pl_state_sts; //I
        Wire<Bool> rdi_pl_inband_pres; //I
        Wire<UInt> linkinit_rdi_lp_state_req; //O

        Wire<UInt> link_state; //I
        Wire<Bool> active_entry; //O
        Wire<UInt> linkinit_sb_snd; //O
        Wire<UInt> linkinit_sb_rcv; //I
        Wire<Bool> linkinit_sb_rdy; //I
    };


    class LinkInitSubmodule: public RegModule {
    public:
        LinkInitSubmoduleIO io;

        LinkInitSubmodule();

        void calcNextState() override;

    private:
        // ============ chisel signals ==================
        // State register for link initialization
        RegPtr<UInt> _linkinit_state_reg;

        // Parameter exchange on sideband message arbitration flags
        RegPtr<Bool> _param_exch_sbmsg_rcv_flag;
        RegPtr<Bool> _param_exch_sbmsg_snt_flag;

        // Active state sb message arbitration flags
        RegPtr<Bool> _active_sbmsg_req_rcv_flag;
        RegPtr<Bool> _active_sbmsg_rsp_rcv_flag;
        RegPtr<Bool> _active_sbmsg_ext_rsp_reg;
        RegPtr<Bool> _active_sbmsg_ext_req_reg;
        RegPtr<Bool> _transition_to_active_reg;

        // ============ helper signals ==================
        Wire<Bool>::TPFUNC _linkinit_fdi_pl_inband_pres;
        Wire<Bool>::TPFUNC _linkinit_fdi_pl_rxactive_req;
        Wire<UInt>::TPFUNC _linkinit_fdi_pl_state_sts;

        Wire<UInt>::TPFUNC _linkinit_rdi_lp_state_req;

        Wire<Bool>::TPFUNC _active_entry;
        Wire<UInt>::TPFUNC _linkinit_sb_snd;

    };


} // namespace CCPS

#endif // __D2DADAPTER_LINK_INIT_SUBMODULE_HPP__
