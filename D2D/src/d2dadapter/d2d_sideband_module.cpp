#include "d2dadapter/d2d_sideband_module.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"
#include "sideband/sb_msg_encoding.hpp"

namespace CCPS {
    D2DSidebandModule::
    D2DSidebandModule(const FdiParams &fdi_params, const SidebandParams &sb_params) {
        int msg_w = sb_params.sb_node_msg_width;
        // Instantiate
        _fdi_sideband_node = createSubmodule<SidebandNode>("fdi_sideband_node", sb_params, fdi_params);
        _rdi_sideband_node = createSubmodule<SidebandNode>("rdi_sideband_node", sb_params, fdi_params);
        _sideband_switch = createSubmodule<SidebandSwitcher>("rdi_sideband_node", 1, sb_params);

        // connect
        io.fdi_pl_cfg = _fdi_sideband_node->io.outer.tx.bits;
        io.fdi_pl_cfg_vld = _fdi_sideband_node->io.outer.tx.valid;
        _fdi_sideband_node->io.outer.tx.credit = io.fdi_pl_cfg_crd;

        _fdi_sideband_node->io.outer.rx.bits = io.fdi_lp_cfg;
        _fdi_sideband_node->io.outer.rx.valid = io.fdi_lp_cfg_vld;
        io.fdi_lp_cfg_crd = _fdi_sideband_node->io.outer.rx.credit;

        _rdi_sideband_node->io.outer.rx.bits = io.rdi_pl_cfg;
        _rdi_sideband_node->io.outer.rx.valid = io.rdi_pl_cfg_vld;
        io.rdi_pl_cfg_crd = _rdi_sideband_node->io.outer.rx.credit;

        io.rdi_lp_cfg = _rdi_sideband_node->io.outer.tx.bits;
        io.rdi_lp_cfg_vld = _rdi_sideband_node->io.outer.tx.valid;
        _rdi_sideband_node->io.outer.tx.credit = io.rdi_lp_cfg_crd;

        _fdi_sideband_node->io.inner.layer_to_node.connect(_sideband_switch->io.outer.layer_to_node_above);
        _rdi_sideband_node->io.inner.layer_to_node.connect(_sideband_switch->io.outer.layer_to_node_below);
        _sideband_switch->io.outer.node_to_layer_above.connect(_fdi_sideband_node->io.inner.node_to_layer);
        _sideband_switch->io.outer.node_to_layer_below.connect(_rdi_sideband_node->io.inner.node_to_layer);

        _sideband_switch->io.inner.layer_to_node_above.assignBits(msg_w, 0);
        _sideband_switch->io.inner.layer_to_node_above.assignValid(false);

        _sideband_switch->io.inner.node_to_layer_below.assignReady(false);
        _sideband_switch->io.inner.node_to_layer_above.assignReady(true);

        _io_sideband_rcv = [this]() -> UInt {
            const int w = D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH;
            BigUInt res = 0;
            if (_sideband_switch->io.inner.node_to_layer_above.isValid() && _sideband_switch->io.inner.node_to_layer_above.isReady()) {
                UInt data = _sideband_switch->io.inner.node_to_layer_above.bits();
                if (data == SBM().LINK_MGMT_ADAPTER0_REQ_ACTIVE) {
                    res = SideBandMessage::REQ_ACTIVE;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_REQ_L1) {
                    res = SideBandMessage::REQ_L1;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_REQ_L2) {
                    res = SideBandMessage::REQ_L2;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_REQ_LINK_RESET) {
                    res = SideBandMessage::REQ_LINKRESET;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_REQ_DISABLE) {
                    res = SideBandMessage::REQ_DISABLED;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_RSP_ACTIVE) {
                    res = SideBandMessage::RSP_ACTIVE;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_RSP_PM_NAK) {
                    res = SideBandMessage::RSP_PMNAK;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_RSP_L1) {
                    res = SideBandMessage::RSP_L1;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_RSP_L2) {
                    res = SideBandMessage::RSP_L2;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_RSP_LINK_RESET) {
                    res = SideBandMessage::RSP_LINKRESET;
                } else if (data == SBM().LINK_MGMT_ADAPTER0_RSP_DISABLE) {
                    res = SideBandMessage::RSP_DISABLED;
                } else if (data == SBM().PARITY_FEATURE_REQ) {
                    res = SideBandMessage::PARITY_FEATURE_REQ;
                } else if (data == SBM().PARITY_FEATURE_ACK) {
                    res = SideBandMessage::PARITY_FEATURE_ACK;
                } else if (data == SBM().PARITY_FEATURE_NAK) {
                    res = SideBandMessage::PARITY_FEATURE_NAK;
                } else if (data == SBM().ADV_CAP) {
                    res = SideBandMessage::ADV_CAP;
                } else {
                    res = SideBandMessage::NOP;
                }
            } else {
                res = SideBandMessage::NOP;
            }
            return UInt(w, res);
        };
        io.sideband_rcv = _io_sideband_rcv;

        _io_sideband_rdy = [this]() -> Bool {
            bool res = false;
            if (io.sideband_snt().toBigUInt() != SideBandMessage::NOP) {
                res = _sideband_switch->io.inner.layer_to_node_below.isValid() && _sideband_switch->io.inner.layer_to_node_below.isReady();
            } else {
                res = false;
            }
            return Bool(res);
        };
        io.sideband_rdy = _io_sideband_rdy;

        _sideband_switch_io_inner_layer_to_node_below_bits = [this]() -> UInt {
            BigUInt data = io.sideband_snt().toBigUInt();
            if (data != SideBandMessage::NOP) {
                if (data == SideBandMessage::REQ_ACTIVE) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_REQ_ACTIVE, "D2D", true, "D2D");
                } else if (data == SideBandMessage::REQ_L1) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_REQ_L1, "D2D", true, "D2D");
                } else if (data == SideBandMessage::REQ_L2) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_REQ_L2, "D2D", true, "D2D");
                } else if (data == SideBandMessage::REQ_LINKRESET) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_REQ_LINK_RESET, "D2D", true, "D2D");
                } else if (data == SideBandMessage::REQ_DISABLED) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_REQ_DISABLE, "D2D", true, "D2D");
                } else if (data == SideBandMessage::RSP_ACTIVE) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_RSP_ACTIVE, "D2D", true, "D2D");
                } else if (data == SideBandMessage::RSP_PMNAK) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_RSP_PM_NAK, "D2D", true, "D2D");
                } else if (data == SideBandMessage::RSP_L1) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_RSP_L1, "D2D", true, "D2D");
                } else if (data == SideBandMessage::RSP_L2) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_RSP_L2, "D2D", true, "D2D");
                } else if (data == SideBandMessage::RSP_LINKRESET) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_RSP_LINK_RESET, "D2D", true, "D2D");
                } else if (data == SideBandMessage::RSP_DISABLED) {
                    return SBMessage_factory(SBM().LINK_MGMT_ADAPTER0_RSP_DISABLE, "D2D", true, "D2D");
                } else if (data == SideBandMessage::PARITY_FEATURE_REQ) {
                    return SBMessage_factory(SBM().PARITY_FEATURE_REQ, "D2D", true, "D2D");
                } else if (data == SideBandMessage::PARITY_FEATURE_ACK) {
                    return SBMessage_factory(SBM().PARITY_FEATURE_ACK, "D2D", true, "D2D");
                } else if (data == SideBandMessage::PARITY_FEATURE_NAK) {
                    return SBMessage_factory(SBM().PARITY_FEATURE_NAK, "D2D", true, "D2D");
                } else if (data == SideBandMessage::ADV_CAP) {
                    return SBMessage_factory(SBM().ADV_CAP, "D2D", true, "D2D", UInt(64, D2DSidebandConstant().ADV_CAP_MESSAGE_DATA));
                } else {
                    return SBMessage_factory(SBM().NOP_CRD, "D2D", true, "D2D");
                }
            } else {
                return SBMessage_factory(SBM().NOP_CRD, "D2D", true, "D2D");
            }
        };
        _sideband_switch->io.inner.layer_to_node_below.assignBits(_sideband_switch_io_inner_layer_to_node_below_bits);

        _sideband_switch_io_inner_layer_to_node_below_valid = [this]() -> Bool {
            bool res = false;
            if (io.sideband_snt().toBigUInt() != SideBandMessage::NOP) {
                res = true;
            } else {
                res = false;
            }
            return Bool(res);
        };
        _sideband_switch->io.inner.layer_to_node_below.assignValid(_sideband_switch_io_inner_layer_to_node_below_valid);
    }

} // namespace CCPS
