#include "d2dadapter/d2d_sideband_module.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

static const FdiParams fdi_params(8, 8, 32);
static const SidebandParams sb_params;

TEST (D2DSidebandModuleTest, DontKnowHowTOCheck) {
    auto top = createTopModule<D2DSidebandModule>(fdi_params, sb_params);
    auto &c = *top;

    // ======================== signals ========================
    bool io_fdi_pl_cfg_crd = false;
    BigUInt io_fdi_lp_cfg = 0;
    bool io_fdi_lp_cfg_vld = false;

    BigUInt io_rdi_pl_cfg = 0;
    bool io_rdi_pl_cfg_vld = false;
    bool io_rdi_pl_cfg_crd = false;

    BigUInt io_sideband_snt = 0;

    // ======================== connect ========================
    c.io.fdi_pl_cfg_crd.capture(io_fdi_pl_cfg_crd);
    c.io.fdi_lp_cfg.capture(fdi_params.sb_width, io_fdi_lp_cfg);
    c.io.fdi_lp_cfg_vld.capture(io_fdi_lp_cfg_vld);

    c.io.rdi_pl_cfg.capture(fdi_params.sb_width, io_rdi_pl_cfg);
    c.io.rdi_pl_cfg_vld.capture(io_rdi_pl_cfg_vld);
    c.io.rdi_lp_cfg_crd.capture(io_rdi_pl_cfg_crd);

    c.io.sideband_snt.capture(D2DAdapterSignalSize().SIDEBAND_MESSAGE_OP_WIDTH, io_sideband_snt);

    // ======================== run ========================
    c.step();
    c.step();

}