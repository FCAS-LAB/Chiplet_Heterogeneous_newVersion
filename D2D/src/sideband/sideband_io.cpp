#include "sideband/sideband_io.hpp"

namespace CCPS {
    // ======================== SidebandNodeOuterIOSingle ============================
    SidebandNodeOuterIOSingle::SidebandNodeOuterIOSingle(bool do_flip):
        _is_flipped(do_flip) {}
    void SidebandNodeOuterIOSingle::connect(SidebandNodeOuterIOSingle &other) {
        bits = other.bits;
        valid = other.valid;
        other.credit = credit;
    }

    // ======================== SidebandNodeOuterIO ============================
    SidebandNodeOuterIO::SidebandNodeOuterIO(bool do_flip):
        _is_flipped(do_flip), tx(do_flip), rx(!do_flip) {}
    void SidebandNodeOuterIO::connect(SidebandNodeOuterIO &other) {
        if (_is_flipped != other._is_flipped) {
            std::cerr << "can only connect same direction partner." << std::endl;
            assert(false);
        }
        tx.connect(other.tx);
        other.rx.connect(rx);
    }

    // ======================== SidebandLinkNodeOuterIOSingle ============================
    SidebandLinkNodeOuterIOSingle::SidebandLinkNodeOuterIOSingle(bool do_flip):
        _is_flipped(do_flip) {}
    void SidebandLinkNodeOuterIOSingle::connect(SidebandLinkNodeOuterIOSingle &other) {
        bits = other.bits;
    }

    // ======================== SidebandLinkNodeOuterIO ============================
    SidebandLinkNodeOuterIO::SidebandLinkNodeOuterIO(bool do_flip):
        _is_flipped(do_flip), tx(do_flip), rx(!do_flip) {}
    void SidebandLinkNodeOuterIO::connect(SidebandLinkNodeOuterIO &other) {
        if (_is_flipped != other._is_flipped) {
            std::cerr << "can only connect same direction partner." << std::endl;
            assert(false);
        }
        tx.connect(other.tx);
        other.rx.connect(rx);
    }

    // ======================== SidebandSwitcherbundle ============================
    SidebandSwitcherbundle::SidebandSwitcherbundle(bool do_flip): _is_flipped(do_flip) {
        if (_is_flipped) {
            flip();
        }
    }
    SidebandSwitcherbundle::SidebandSwitcherbundle():SidebandSwitcherbundle(false)  {}

    void SidebandSwitcherbundle::flip() {
        node_to_layer_above.flip();
        layer_to_node_above.flip();
        node_to_layer_below.flip();
        layer_to_node_below.flip();
    }

    void SidebandSwitcherbundle::connect(SidebandSwitcherbundle& other) {
        if (!_is_flipped) {
            other.node_to_layer_above.connect(node_to_layer_above);
            layer_to_node_above.connect(other.layer_to_node_above);
            other.node_to_layer_below.connect(node_to_layer_below);
            layer_to_node_below.connect(other.layer_to_node_below);
        } else {
            node_to_layer_above.connect(other.node_to_layer_above);
            other.layer_to_node_above.connect(layer_to_node_above);
            node_to_layer_below.connect(other.node_to_layer_below);
            other.layer_to_node_below.connect(layer_to_node_below);
        }
    }

} // namespace CCPS