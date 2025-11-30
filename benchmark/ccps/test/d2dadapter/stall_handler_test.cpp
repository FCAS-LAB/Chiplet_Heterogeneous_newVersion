#include "d2dadapter/d2d_adapter_constants.hpp"
#include "d2dadapter/stall_handler.hpp"
#include "interfaces/types.hpp"
#include "test_utils.hpp"
#include "utils/time_slice.hpp"
#include "utils/common.hpp"
#include "interfaces/fdi.hpp"
#include "interfaces/rdi.hpp"
#include "sideband/sideband_io.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

TEST (StallHandlerTest, RDIComplteStallHandshake) {
    auto top = createTopModule<RDIStallHandler>();
    auto &c = *top;

    // ======================== signals ========================
    bool io_mainband_stalldone = false;
    bool io_rdi_pl_stallreq = false;

    // ======================== connect ========================
    c.io.mainband_stalldone.capture(io_mainband_stalldone);
    c.io.rdi_pl_stallreq.capture(io_rdi_pl_stallreq);

    // ======================== run ========================
    for (int a = 0; a < 5; a++) {
        // init
        io_rdi_pl_stallreq = false;
        io_mainband_stalldone = false;
        c.step(1);
        c.step(1);
        EXPECT_EQ_BOOL(c.io.rdi_lp_stallack(), false);
        // Rising edge of stall
        io_rdi_pl_stallreq = true;
        c.step(1);
        // should tell the mainband to stall
        while (!c.io.mainband_stallreq()) {
            io_rdi_pl_stallreq = true;
            c.step(1);
        }
        // mainband processing
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BOOL(c.io.mainband_stallreq(), true);
            EXPECT_EQ_BOOL(c.io.rdi_lp_stallack(), false);
            c.step(1);
        }
        // mainband reply
        io_mainband_stalldone = true;
        std::cout << "set mainband_stalldone" << std::endl;
        c.step(1);
        // should create rising edge on stallack
        while (!c.io.rdi_lp_stallack()) {
            io_rdi_pl_stallreq = true;
            c.step(1);
        }
        // stallack should keep high, as the handshake needs to wait for falling edge of req
        // the mainband should tell that it has stalled the transmission
        io_rdi_pl_stallreq = true;
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BOOL(c.io.rdi_lp_stallack(), true);
            EXPECT_EQ_BOOL(c.io.mainband_stalldone(), true);
            c.step(1);
        }
        // falling edge of req
        io_rdi_pl_stallreq = false;
        c.step(1);
        // waiting for falling edge of
        while (c.io.rdi_lp_stallack()) {
            io_rdi_pl_stallreq = false;
            c.step(1);
        }
        // stallack should keep low, as the handshake has completed
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BOOL(c.io.rdi_lp_stallack(), false);
            c.step(1);
        }
    }
}

TEST (StallHandlerTest, FDIComplteStallHandshake) {
    auto top = createTopModule<FDIStallHandler>();
    auto &c = *top;

    // ======================== signals ========================
    bool io_linkmgmt_stallreq = false;
    bool io_fdi_lp_stallack = false;

    // ======================== connect ========================
    c.io.linkmgmt_stallreq.capture(io_linkmgmt_stallreq);
    c.io.fdi_lp_stallack.capture(io_fdi_lp_stallack);

    // ======================== run ========================
    for (int a = 0; a < 5; a++) {
        // init
        io_linkmgmt_stallreq = false;
        c.step(1);
        // check if the initial signals are all false
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BOOL(c.io.linkmgmt_stalldone(), false);
            EXPECT_EQ_BOOL(c.io.fdi_pl_stallreq(), false);
            c.step(1);
        }
        // link management require handshake
        io_linkmgmt_stallreq = true;
        c.step(1);

        while (!c.io.fdi_pl_stallreq()) {
            EXPECT_EQ_BOOL(c.io.linkmgmt_stalldone(), false);
            c.step(1);
        }
        // start handshaking, waiting for ack
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BOOL(c.io.linkmgmt_stalldone(), false);
            c.step(1);
        }

        io_fdi_lp_stallack = true;
        c.step(1);
        // fdi ack, req should fall
        while (c.io.fdi_pl_stallreq()) {
            EXPECT_EQ_BOOL(c.io.linkmgmt_stalldone(), false);
            c.step(1);
        }
        // req fall, the ack should fall
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ_BOOL(c.io.linkmgmt_stalldone(), false);
            c.step(1);
        }

        // ack fall
        io_fdi_lp_stallack = false;
        c.step(1);
        // tell the link management that the handshake has completed
        while (!c.io.linkmgmt_stalldone()) {
            EXPECT_EQ_BOOL(c.io.fdi_pl_stallreq(), false);
            c.step(1);
        }
    }
}
