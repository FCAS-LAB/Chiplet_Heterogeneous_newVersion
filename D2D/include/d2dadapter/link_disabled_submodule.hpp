#ifndef __D2DADAPTER_LINK_DISABLED_SUBMODULE_HPP__
#define __D2DADAPTER_LINK_DISABLED_SUBMODULE_HPP__

#include "utils/wire.hpp"
#include "utils/reg.hpp"
#include "utils/module.hpp"

namespace CCPS {
    // ========================== LinkDisabledSubmoduleIO ===============================
    struct LinkDisabledSubmoduleIO {
        Wire<UInt> fdi_lp_state_req;
        Wire<UInt> fdi_lp_state_req_prev;
        Wire<UInt> link_state;
        Wire<Bool> disabled_entry;
        Wire<UInt> disabled_sb_snd;
        Wire<UInt> disabled_sb_rcv;
        Wire<Bool> disabled_sb_rdy;
    };

    // ========================== LinkDisabledSubmodule ===============================
    /** LinkDisabledSubmodule handles the transition of FDI/RDI state machine from
    * Reset, Active, Retrain, and LinkReset to Disabled state. The transition is
    * triggered by fdi_lp_state_req or though sideband messages coming from
    * partner link.
    *
    */
    class LinkDisabledSubmodule: public RegModule {
    public:
        LinkDisabledSubmoduleIO io;

        LinkDisabledSubmodule();

        void calcNextState() override;

    private:
        // ============ chisel signals ==============
        RegPtr<Bool> _disabled_fdi_req_reg;
        RegPtr<Bool> _disabled_sbmsg_req_rcv_reg;
        RegPtr<Bool> _disabled_sbmsg_rsp_rcv_reg;
        RegPtr<Bool> _disabled_sbmsg_ext_rsp_reg;
        RegPtr<Bool> _disabled_sbmsg_ext_req_reg;

        // ============ helper signals ==============
        Wire<Bool>::TPFUNC _io_disabled_entry;
        Wire<UInt>::TPFUNC _io_disabled_sb_snd;
    };

} // namespace CCPS
#endif // __D2DADAPTER_LINK_DISABLED_SUBMODULE_HPP__
