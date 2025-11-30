#ifndef __D2DADAPTER_LINK_RESET_SUBMODULE_HPP__
#define __D2DADAPTER_LINK_RESET_SUBMODULE_HPP__

#include "utils/wire.hpp"
#include "utils/module.hpp"
#include "utils/reg.hpp"

namespace CCPS {
    struct LinkResetSubmoduleIO {
        Wire<UInt> fdi_lp_state_req;
        Wire<UInt> fdi_lp_state_req_prev;
        Wire<UInt> link_state;
        Wire<Bool> linkreset_entry;
        Wire<UInt> linkreset_sb_snd;
        Wire<UInt> linkreset_sb_rcv;
        Wire<Bool> linkreset_sb_rdy;
    };

    class LinkResetSubmodule: public RegModule {
    public:

        LinkResetSubmoduleIO io;

        LinkResetSubmodule();
        void calcNextState() override;

    private:
        // =============== chisel signals =====================
        RegPtr<Bool> _linkreset_fdi_req_reg;
        RegPtr<Bool> _linkreset_sbmsg_req_rcv_flag;
        RegPtr<Bool> _linkreset_sbmsg_rsp_rcv_flag;
        RegPtr<Bool> _linkreset_sbmsg_ext_rsp_reg;
        RegPtr<Bool> _linkreset_sbmsg_ext_req_reg;

        // =============== helper signals =====================
        Wire<Bool>::TPFUNC _io_linkreset_entry;
        Wire<UInt>::TPFUNC _linkreset_sb_snd;
    };
} // namespace CCPS
#endif // __D2DADAPTER_LINK_RESET_SUBMODULE_HPP__