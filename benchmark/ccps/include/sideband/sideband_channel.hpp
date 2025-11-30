#ifndef __SIDEBAND_CHANNEL_HPP__
#define __SIDEBAND_CHANNEL_HPP__

#include "sideband/sideband_node.hpp"
#include "sideband/sideband_switcher.hpp"
#include "interfaces/fdi.hpp"

namespace CCPS {
    class D2DSidebandChannel: public WireModule {
    public:
        D2DSidebandChannel(
            const SidebandParams& sb_params,
            const FdiParams& fdi_params,
            const BigInt my_id = 1
        );
        ~D2DSidebandChannel() = default;

        D2DSidebandChannelIO io;

        void calcNextState() override;

    private:
        // ============== chisel signals ================
        ModulePtr<SidebandNode> _upper_node;
        ModulePtr<SidebandSwitcher> _switcher;
        ModulePtr<SidebandNode> _lower_node;

        // ============ helper signals ===========================
    };

    // ========================== PHYSidebandChannel ===============================
    class PHYSidebandChannel: public WireModule {
    public:
        PHYSidebandChannel(
            const SidebandParams& sb_params,
            const FdiParams& fdi_params,
            const BigInt my_id = 2
        );

        PHYSidebandChannelIO io;

        bool propagateClock() override;

    private:
        // ============== chisel signals ================
        ModulePtr<SidebandNode> _upper_node;
        ModulePtr<SidebandSwitcher> _switcher;
        ModulePtr<SidebandLinkNode> _lower_node;

        // ============ helper signals ===========================
        Wire<Bool>::TPFUNC _io_inner_raw_input_ready;
        Wire<Bool>::TPFUNC _lower_node_io_inner_layer_to_node_valid;
        Wire<UInt>::TPFUNC _lower_node_io_inner_layer_to_node_bits;
        Wire<Bool>::TPFUNC _switcher_io_outer_layer_to_node_below_ready;

    };
}

#endif // __SIDEBAND_CHANNEL_HPP__