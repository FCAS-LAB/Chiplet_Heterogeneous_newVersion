#include "sideband/sideband_channel.hpp"

namespace CCPS {
    // ========================== D2DSidebandChannel ===============================
    D2DSidebandChannel::
    D2DSidebandChannel(
        const SidebandParams& sb_params,
        const FdiParams& fdi_params,
        const BigInt my_id)
    {
        // Instantiate submodule
        _upper_node = createSubmodule<SidebandNode>("upper_node", sb_params, fdi_params);
        _switcher = createSubmodule<SidebandSwitcher>("upper_node", my_id, sb_params);
        _lower_node = createSubmodule<SidebandNode>("lower_node", sb_params, fdi_params);

        // Connect outer signals
        io.to_upper_layer.tx.bits = _upper_node->io.outer.tx.bits;
        io.to_upper_layer.tx.valid = _upper_node->io.outer.tx.valid;
        _upper_node->io.outer.tx.credit = io.to_upper_layer.tx.credit;

        _upper_node->io.outer.rx.bits = io.to_upper_layer.rx.bits;
        _upper_node->io.outer.rx.valid = io.to_upper_layer.rx.valid;
        io.to_upper_layer.rx.credit = _upper_node->io.outer.rx.credit;

        io.to_lower_layer.tx.bits = _lower_node->io.outer.tx.bits;
        io.to_lower_layer.tx.valid = _lower_node->io.outer.tx.valid;
        _lower_node->io.outer.tx.credit = io.to_lower_layer.tx.credit;

        _lower_node->io.outer.rx.bits = io.to_lower_layer.rx.bits;
        _lower_node->io.outer.rx.valid = io.to_lower_layer.rx.valid;
        io.to_lower_layer.rx.credit = _lower_node->io.outer.rx.credit;

        // Connect two sidebandNodes and switcher
        _upper_node->io.inner.layer_to_node.connect(_switcher->io.outer.layer_to_node_above);
        _switcher->io.outer.node_to_layer_above.connect(_upper_node->io.inner.node_to_layer);
        _lower_node->io.inner.layer_to_node.connect(_switcher->io.outer.layer_to_node_below);
        _switcher->io.outer.node_to_layer_below.connect(_lower_node->io.inner.node_to_layer);

        // Connect inner signals
        //io.inner.node_to_layer_above.connect(_switcher->io.inner.node_to_layer_above);
        //_switcher->io.inner.layer_to_node_above.connect(io.inner.layer_to_node_above);
        //io.inner.node_to_layer_below.connect(_switcher->io.inner.node_to_layer_below);
        //_switcher->io.inner.layer_to_node_below.connect(io.inner.layer_to_node_below);
        io.inner.connect(_switcher->io.inner);
    }


    void D2DSidebandChannel::calcNextState() {
        //std::cout << getPathName() << ": upper_node.io.outer.rx.valid " << static_cast<bool>(_upper_node->io.outer.rx.valid()) << std::endl;
        //std::cout << getPathName() << ": upper_node.io.outer.rx.credit " << static_cast<bool>(_upper_node->io.outer.rx.credit()) << std::endl;
        //std::cout << getPathName() << ": upper_node.io.outer.rx.bits " << std::hex << _upper_node->io.outer.rx.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": switcher.io.inner.node_to_layer_above.valid " << static_cast<bool>(_switcher->io.inner.node_to_layer_above.isValid()) << std::endl;
        //std::cout << getPathName() << ": switcher.io.inner.node_to_layer_above.ready " << static_cast<bool>(_switcher->io.inner.node_to_layer_above.isReady()) << std::endl;
    }

    // ========================== PHYSidebandChannel ===============================
    PHYSidebandChannel::
    PHYSidebandChannel(
        const SidebandParams& sb_params,
        const FdiParams& fdi_params,
        const BigInt my_id)
    {
        // Instantiate submodule
        _upper_node = createSubmodule<SidebandNode>("upper_node", sb_params, fdi_params);
        _switcher = createSubmodule<SidebandSwitcher>("upper_node", my_id, sb_params);
        _lower_node = createSubmodule<SidebandLinkNode>("lower_node", sb_params, fdi_params);

        // Connect outer signals
        io.to_upper_layer.connect(_upper_node->io.outer);
        io.to_lower_layer.connect(_lower_node->io.outer);

        // Connect two sidebandNodes and switcher
        _upper_node->io.inner.layer_to_node.connect(_switcher->io.outer.layer_to_node_above);
        _switcher->io.outer.node_to_layer_above.connect(_upper_node->io.inner.node_to_layer);

        _io_inner_raw_input_ready = [this]() -> Bool {
            if (io.inner.input_mode().toBigUInt() == RXTXMode::PACKET) {
                return Bool(false);
            } else {
                return _lower_node->io.inner.layer_to_node.isReady();
            }
        };
        io.inner.raw_input.assignReady(_io_inner_raw_input_ready);

        _lower_node_io_inner_layer_to_node_valid = [this]() -> Bool {
            if (io.inner.input_mode().toBigUInt() == RXTXMode::PACKET) {
                return _switcher->io.outer.layer_to_node_below.isValid();
            } else {
                return io.inner.raw_input.isValid();
            }
        };
        _lower_node->io.inner.layer_to_node.assignValid(_lower_node_io_inner_layer_to_node_valid);

        _lower_node_io_inner_layer_to_node_bits = [this]() -> UInt {
            if (io.inner.input_mode().toBigUInt() == RXTXMode::PACKET) {
                return _switcher->io.outer.layer_to_node_below.bits();
            } else {
                return io.inner.raw_input.bits();
            }
        };
        _lower_node->io.inner.layer_to_node.assignBits(_lower_node_io_inner_layer_to_node_bits);

        _switcher_io_outer_layer_to_node_below_ready = [this]() -> Bool {
            if (io.inner.input_mode().toBigUInt() == RXTXMode::PACKET) {
                return _lower_node->io.inner.layer_to_node.isReady();
            } else {
                return Bool(false);
            }
        };
        _switcher->io.outer.layer_to_node_below.assignReady(_switcher_io_outer_layer_to_node_below_ready);

        _switcher->io.outer.node_to_layer_below.connect(_lower_node->io.inner.node_to_layer);
        _lower_node->io.rx_mode = io.inner.rx_mode;

        // Connect inner signals
        io.inner.switcher_bundle.connect(_switcher->io.inner);
    }

    bool PHYSidebandChannel::propagateClock() {
        std::cout << getPathName() << " propagateClock" << std::endl;
        bool success = true;
        if (io.to_lower_layer.rx.clock != nullptr) {
            _lower_node->io.outer.rx.clock = io.to_lower_layer.rx.clock;
        } else {
            success = false;
        }

        success &= Module::propagateClock();

        if (_lower_node->io.outer.tx.clock != nullptr) {
            io.to_lower_layer.tx.clock = _lower_node->io.outer.tx.clock;
        } else {
            success = false;
        }
        return success;

    }
} // namespace CCPS