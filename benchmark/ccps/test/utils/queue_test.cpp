#include "utils/queue.hpp"
#include "test_utils.hpp"
#include "utils/time_slice.hpp"
#include <gtest/gtest.h>
#include <iostream>

using namespace CCPS;

TEST(QueueTests, ContinuousTest) {
    bool enq_valid = false;
    BigUInt enq_bits = 0;
    bool deq_ready = false;

    const int depth = 16;

    //Queue<UInt> queue(depth);
    ModulePtr<Queue<UInt>> top = createTopModule<Queue<UInt>>(depth);
    Queue<UInt> &queue = *top;
    //TimeSlice::getInstance().showClockTree();

    Wire<Bool>::TPFUNC queue_enq_valid_func{
        [&enq_valid]() -> Bool {
            return Bool(enq_valid);
        }
    };
    queue.io.enq.assignValid(queue_enq_valid_func);
    Wire<UInt>::TPFUNC queue_enq_bits_func{
        [&enq_bits]() -> UInt {
            return UInt(8, enq_bits);
        }
    };
    queue.io.enq.assignBits(queue_enq_bits_func);
    Wire<Bool>::TPFUNC queue_deq_ready_func{
        [&deq_ready]() -> Bool {
            return Bool(deq_ready);
        }
    };
    queue.io.deq.assignReady(queue_deq_ready_func);

    // check initial value
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BigUInt(queue.io.count(), 0) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.enq.isReady(), true) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.deq.isValid(), false) << "Queue initial value failed, index: " << i;
        queue.step();
    }

    // push until full
    enq_valid = true;
    enq_bits = 0;
    deq_ready = false;
    for (int i = 0; i < depth; i++) {
        enq_bits = i;
        EXPECT_EQ_BigUInt(queue.io.count(), i) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.enq.isReady(), true) << "Queue initial value failed, index: " << i;
        if (i == 0) {
            EXPECT_EQ_BOOL(queue.io.deq.isValid(), false) << "Queue initial value failed, index: " << i;
        } else {
            EXPECT_EQ_BOOL(queue.io.deq.isValid(), true) << "Queue initial value failed, index: " << i;
        }
        queue.step();
    }

    // check full control
    enq_valid = true;
    enq_bits = 0;
    deq_ready = false;
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BigUInt(queue.io.count(), depth) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.enq.isReady(), false) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.deq.isValid(), true) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BigUInt(queue.io.deq.bits(), BigUInt(0)) << "Queue initial value failed, index: " << i;
        queue.step();
    }

    // push and pop
    enq_valid = true;
    enq_bits = 0;
    deq_ready = true;
    for (int i = 0; i < depth; i++) {
        enq_bits = i % depth;
        // when i == 0, enq.ready is false, deq.valid is true, so leak the 0.
        if (i == 0) {
            EXPECT_EQ_BigUInt(queue.io.count(), depth) << "Queue initial value failed, index: " << i;
            EXPECT_EQ_BOOL(queue.io.enq.isReady(), false) << "Queue initial value failed, index: " << i;
        } else {
            EXPECT_EQ_BigUInt(queue.io.count(), depth-1) << "Queue initial value failed, index: " << i;
            EXPECT_EQ_BOOL(queue.io.enq.isReady(), true) << "Queue initial value failed, index: " << i;
        }
        EXPECT_EQ_BOOL(queue.io.deq.isValid(), true) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BigUInt(queue.io.deq.bits(), BigUInt(i % depth)) << "Queue initial value failed, index: " << i;
        queue.step();
    }
    for (int i = 0; i < depth; i++) {
        enq_bits = i % depth;
        EXPECT_EQ_BigUInt(queue.io.count(), depth-1) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.enq.isReady(), true) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.deq.isValid(), true) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BigUInt(queue.io.deq.bits(), BigUInt((i+1) % depth)) << "Queue initial value failed, index: " << i;
        queue.step();
    }


    // only pop, missing 0 in the above stage.
    enq_valid = false;
    enq_bits = 0;
    deq_ready = true;
    for (int i = 1; i < depth; i++) {
        EXPECT_EQ_BigUInt(queue.io.count(), depth-i) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.enq.isReady(), true) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.deq.isValid(), true) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BigUInt(queue.io.deq.bits(), BigUInt(i)) << "Queue initial value failed, index: " << i;
        queue.step();
    }

    // check empty control
    enq_valid = false;
    enq_bits = 0;
    deq_ready = true;
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BigUInt(queue.io.count(), 0) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.enq.isReady(), true) << "Queue initial value failed, index: " << i;
        EXPECT_EQ_BOOL(queue.io.deq.isValid(), false) << "Queue initial value failed, index: " << i;
        queue.step();
    }




}