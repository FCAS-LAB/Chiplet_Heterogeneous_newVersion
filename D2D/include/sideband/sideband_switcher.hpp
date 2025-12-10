#ifndef __SIDEBAND_SWITCHER_HPP__
#define __SIDEBAND_SWITCHER_HPP__

#include "sideband/sideband_io.hpp"
#include "sideband/sideband_node.hpp"
#include "utils/module.hpp"
#include "utils/base_types.hpp"

namespace CCPS {
    class sidebandOneInTwoOutSwitch: public WireModule {
    public:
        struct {
            Decoupled<UInt> outer_node_to_layer{true};
            Decoupled<UInt> inner_node_to_layer;
            Decoupled<UInt> node_to_node;
        } io;

        sidebandOneInTwoOutSwitch(const BigInt my_id, const SidebandParams& sb_params);

    private:
        // ===================== module internal signals ========================
        Wire<Bool> _inner_node_to_layer_valid_func;
        Wire<UInt> _inner_node_to_layer_bits_func;
        Wire<Bool> _node_to_node_valid_func;
        Wire<UInt> _node_to_node_bits_func;
        Wire<Bool> _outer_node_to_layer_ready_func;
    };

    class sidebandTwoInOneOutSwitch: public WireModule {
    public:
        struct {
            Decoupled<UInt> outer_layer_to_node;
            Decoupled<UInt> inner_layer_to_node{true};
            Decoupled<UInt> node_to_node{true};
        } io;

        sidebandTwoInOneOutSwitch(const BigInt my_id, const SidebandParams& sb_params);

        // ===================== module internal signals ========================
    private:
        Wire<Bool> _flag;
        Wire<UInt> _priority_node_to_node;
        Wire<UInt> _priority_inner_layer_to_node;

        //Wire<UInt> _priority_node_to_node_func;
        //Wire<UInt> _priority_inner_layer_to_node_func;
        //Wire<Bool> _flag_func;
        Wire<Bool> _io_outer_layer_to_node_valid;
        Wire<UInt> _io_outer_layer_to_node_bits;
        Wire<Bool> _io_node_to_node_ready;
        Wire<Bool> _io_inner_layer_to_node_ready;
    };

    class SidebandSwitcher: public WireModule {
    public:
        SidebandSwitcher(const BigInt my_id, const SidebandParams& sb_params);

        SidebandSwitcherIO io;
        Decoupled<UInt> node_to_node_below_to_above;
        Decoupled<UInt> node_to_node_above_to_below;
        ModulePtr<sidebandOneInTwoOutSwitch> outer_node_to_layer_below_subswitch;
        ModulePtr<sidebandOneInTwoOutSwitch> outer_node_to_layer_above_subswitch;
        ModulePtr<sidebandTwoInOneOutSwitch> outer_layer_to_node_above_subswitch;
        ModulePtr<sidebandTwoInOneOutSwitch> outer_layer_to_node_below_subswitch;
    };
}

#endif // __SIDEBAND_SWITCHER_HPP__
