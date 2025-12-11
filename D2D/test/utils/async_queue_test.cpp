#include "utils/async_queue.hpp"
#include "test_utils.hpp"
#include "utils/common.hpp"
#include "utils/time_slice.hpp"
#include <gtest/gtest.h>
#include <iostream>

using namespace CCPS;

TEST(AsyncQueueTests, ContinuousTest) {
    bool enq_valid = false;
    BigUInt enq_bits = 0;
    bool deq_ready = false;

    const int depth = 16;

    // Cannot use createTopModule, because this module needs to set enq_clock and deq_clock before propagateClock.
    //ModulePtr<AsyncQueue<UInt>> top = createTopModule<AsyncQueue<UInt>>(depth);
    clearAllClockModules();
    resetSimTime();

    auto m = std::make_shared<AsyncQueue<UInt>>(depth);
    m->setModuleName("top");
    m->buildModuleTree("");
    addModuleToClock(createClock(100000, "main_clock"), m);
    //return m;
    auto &c = *m;
    const int ENQ_CYCLE = 1000 * 100;
    const int DEQ_CYCLE = 1000 * 300;
    c.io.enq_clock = createClock(ENQ_CYCLE, "enq_clock");
    c.io.deq_clock = createClock(DEQ_CYCLE, "deq_clock");
    m->topPropagateClock();
    TimeSlice::getInstance().showClockTree();


    // ============= connect ================
    bool io_enq_valid = false;
    BigUInt io_enq_bits = 0;
    bool io_enq_reset = false;
    bool io_deq_ready = false;
    bool io_deq_reset = false;

    c.io.enq.assignValid(io_enq_valid);
    c.io.enq.assignBits(8, io_enq_bits);
    c.io.enq_reset.capture(io_enq_reset);
    c.io.deq.assignReady(io_deq_ready);
    c.io.deq_reset.capture(io_deq_reset);

    // ============== run ================
    // check initial value
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BOOL(c.io.enq.isReady(), true) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(c.io.deq.isValid(), false) << "Queue initial value failed, index: " << i;
        run(DEQ_CYCLE);
    }

    // push until full
    for (int i = 0; i < depth; i++) {
        //std::cout << "run cycle: " << ENQ_CYCLE << " enq data " << i << std::endl;
        io_enq_valid = true;
        io_enq_bits = i;
        EXPECT_EQ_BOOL(c.io.enq.isReady(), true) << "Queue initial value failed, index: " << i;
        run(ENQ_CYCLE);
    }
    io_enq_valid = false;
    EXPECT_EQ_BOOL(c.io.enq.isReady(), false);
    run(ENQ_CYCLE);
    EXPECT_EQ_BOOL(c.io.enq.isReady(), false);

    // check output valid
    run(DEQ_CYCLE);
    EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
    run(DEQ_CYCLE);

    // check output value
    for (int i = 0; i < depth; i++) {
        io_deq_ready = true;
        EXPECT_EQ_BOOL(c.io.deq.isValid(), true);
        EXPECT_EQ_BOOL(c.io.deq.bits(), i);
        run(DEQ_CYCLE);
    }
    EXPECT_EQ_BOOL(c.io.deq.isValid(), false);
    io_deq_ready = false;

    // check io status
    run(ENQ_CYCLE);
    EXPECT_EQ_BOOL(c.io.enq.isReady(), true);
}