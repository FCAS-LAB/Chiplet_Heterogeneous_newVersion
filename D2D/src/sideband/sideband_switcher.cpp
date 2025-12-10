#include "sideband/sideband_switcher.hpp"
#include "sideband/sb_msg_encoding.hpp"

namespace CCPS {
    sidebandOneInTwoOutSwitch::
    sidebandOneInTwoOutSwitch(const BigInt my_id, const SidebandParams& sb_params) {

        _inner_node_to_layer_valid_func = [this, my_id]() -> Bool {
            return Bool(io.outer_node_to_layer.isValid() && io.outer_node_to_layer.bits().toBigUInt(58, 56) == my_id);
        };
        io.inner_node_to_layer.assignValid(_inner_node_to_layer_valid_func);

        _inner_node_to_layer_bits_func = [this]() -> UInt {
            return io.outer_node_to_layer.bits();
        };
        io.inner_node_to_layer.assignBits(_inner_node_to_layer_bits_func);

        _node_to_node_valid_func = [this, my_id]() -> Bool {
            return Bool(io.outer_node_to_layer.isValid() && io.outer_node_to_layer.bits().toBigUInt(58, 56) != my_id);
        };
        io.node_to_node.assignValid(_node_to_node_valid_func);

        _node_to_node_bits_func = [this]() -> UInt {
            return io.outer_node_to_layer.bits();
        };
        io.node_to_node.assignBits(_node_to_node_bits_func);

        _outer_node_to_layer_ready_func = [this, my_id]() -> Bool {
            bool is_ready;
            if (io.outer_node_to_layer.bits().toBigUInt(58, 56) == my_id) {
                is_ready = io.inner_node_to_layer.isReady();
            } else {
                is_ready = io.node_to_node.isReady();
            }
            return Bool(is_ready);
        };
        io.outer_node_to_layer.assignReady(_outer_node_to_layer_ready_func);
    }

    sidebandTwoInOneOutSwitch::
    sidebandTwoInOneOutSwitch(const BigInt my_id, const SidebandParams& sb_params) {

        _priority_node_to_node = [this]() -> UInt {
            if (SBM().isComplete(io.node_to_node.bits())) {
                return UInt(2, 0);
            } else {
                if (SBM().isMessage(io.node_to_node.bits())) {
                    return UInt(2, 1);
                } else {
                    return UInt(2, 2);
                }
            }
        };

        _priority_inner_layer_to_node = [this]() -> UInt {
            if (SBM().isComplete(io.inner_layer_to_node.bits())) {
                return UInt(2, 0);
            } else {
                if (SBM().isMessage(io.inner_layer_to_node.bits())) {
                    return UInt(2, 1);
                } else {
                    return UInt(2, 2);
                }
            }
        };

        _flag = [this]() -> Bool {
            if (io.node_to_node.isValid() && io.inner_layer_to_node.isValid()) {
                return Bool(_priority_inner_layer_to_node() > _priority_node_to_node());
            } else {
                if (io.node_to_node.isValid()) {
                    return Bool(true);
                } else {
                    return Bool(false);
                }
            }
        };

        _io_outer_layer_to_node_valid = [this]() -> Bool {
            return Bool(io.node_to_node.isValid() || io.inner_layer_to_node.isValid());
        };
        io.outer_layer_to_node.assignValid(_io_outer_layer_to_node_valid);

        _io_outer_layer_to_node_bits = [this]() -> UInt {
            if (_flag()) {
                return io.node_to_node.bits();
            } else {
                return io.inner_layer_to_node.bits();
            }
        };
        io.outer_layer_to_node.assignBits(_io_outer_layer_to_node_bits);

        _io_node_to_node_ready = [this]() -> Bool {
            bool res = false;
            if (_flag()) {
                res = io.outer_layer_to_node.isReady();
            }
            return Bool(res);
        };
        io.node_to_node.assignReady(_io_node_to_node_ready);

        _io_inner_layer_to_node_ready = [this]() -> Bool {
            bool res = false;
            if (!_flag()) {
                res = io.outer_layer_to_node.isReady();
            }
            return Bool(res);
        };
        io.inner_layer_to_node.assignReady(_io_inner_layer_to_node_ready);

    }

    SidebandSwitcher::
    SidebandSwitcher(const BigInt my_id, const SidebandParams& sb_params) {
        outer_node_to_layer_below_subswitch = createSubmodule<sidebandOneInTwoOutSwitch>("outer_node_to_layer_below_subswitch", my_id, sb_params);
        outer_node_to_layer_above_subswitch = createSubmodule<sidebandOneInTwoOutSwitch>("outer_node_to_layer_above_subswitch", my_id, sb_params);
        outer_layer_to_node_above_subswitch = createSubmodule<sidebandTwoInOneOutSwitch>("outer_layer_to_node_above_subswitch", my_id, sb_params);
        outer_layer_to_node_below_subswitch = createSubmodule<sidebandTwoInOneOutSwitch>("outer_layer_to_node_below_subswitch", my_id, sb_params);

        outer_node_to_layer_below_subswitch->io.outer_node_to_layer.connect(io.outer.node_to_layer_below);
        outer_node_to_layer_above_subswitch->io.outer_node_to_layer.connect(io.outer.node_to_layer_above);

        io.inner.node_to_layer_below.connect(outer_node_to_layer_below_subswitch->io.inner_node_to_layer);
        io.inner.node_to_layer_above.connect(outer_node_to_layer_above_subswitch->io.inner_node_to_layer);

        node_to_node_below_to_above.connect(outer_node_to_layer_below_subswitch->io.node_to_node);
        node_to_node_above_to_below.connect(outer_node_to_layer_above_subswitch->io.node_to_node);

        outer_layer_to_node_above_subswitch->io.node_to_node.connect(node_to_node_below_to_above);
        outer_layer_to_node_below_subswitch->io.node_to_node.connect(node_to_node_above_to_below);

        io.outer.layer_to_node_above.connect(outer_layer_to_node_above_subswitch->io.outer_layer_to_node);
        io.outer.layer_to_node_below.connect(outer_layer_to_node_below_subswitch->io.outer_layer_to_node);

        outer_layer_to_node_above_subswitch->io.inner_layer_to_node.connect(io.inner.layer_to_node_above);
        outer_layer_to_node_below_subswitch->io.inner_layer_to_node.connect(io.inner.layer_to_node_below);
    };
} // namespace CCPS