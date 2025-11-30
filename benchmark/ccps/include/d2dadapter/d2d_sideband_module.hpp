#ifndef __D2DADAPTER_D2D_SIDEBAND_MODULE_HPP__
#define __D2DADAPTER_D2D_SIDEBAND_MODULE_HPP__

#include "utils/wire.hpp"
#include "utils/module.hpp"
#include "interfaces/fdi.hpp"
#include "sideband/sideband_io.hpp"
#include "sideband/sideband_node.hpp"
#include "sideband/sideband_switcher.hpp"

namespace CCPS {
    struct D2DSidebandConstant {
        unsigned ADV_CAP_MESSAGE_DATA = 0b0000000000000000000000000000000000000000000000000000000010010001;
    };

    struct D2DSidebandModuleIO{
        Wire<UInt> fdi_pl_cfg;    //O
        Wire<Bool> fdi_pl_cfg_vld;    //O
        Wire<Bool> fdi_pl_cfg_crd;    //I
        Wire<UInt> fdi_lp_cfg;    //I
        Wire<Bool> fdi_lp_cfg_vld;    //I
        Wire<Bool> fdi_lp_cfg_crd;    //O

        Wire<UInt> rdi_pl_cfg;    //I
        Wire<Bool> rdi_pl_cfg_vld;    //I
        Wire<Bool> rdi_pl_cfg_crd;    //O
        Wire<UInt> rdi_lp_cfg;    //O
        Wire<Bool> rdi_lp_cfg_vld;    //O
        Wire<Bool> rdi_lp_cfg_crd;    //I

        // interface to link management controller
        Wire<UInt> sideband_rcv;    //O
        Wire<UInt> sideband_snt;    //I
        Wire<Bool> sideband_rdy;    //O
    };

    class D2DSidebandModule: public WireModule {
    public:
        D2DSidebandModuleIO io;

        D2DSidebandModule(const FdiParams &fdi_params, const SidebandParams &sb_params);

    private:
        // ============ chisel signals ==================
        ModulePtr<SidebandNode> _fdi_sideband_node;
        ModulePtr<SidebandNode> _rdi_sideband_node;
        ModulePtr<SidebandSwitcher> _sideband_switch;

        // ============ helper signals ==================
        Wire<UInt>::TPFUNC _io_sideband_rcv;
        Wire<Bool>::TPFUNC _io_sideband_rdy;
        Wire<UInt>::TPFUNC _sideband_switch_io_inner_layer_to_node_below_bits;
        Wire<Bool>::TPFUNC _sideband_switch_io_inner_layer_to_node_below_valid;

    };


} // namespace CCPS

#endif // __D2DADAPTER_D2D_SIDEBAND_MODULE_HPP__
