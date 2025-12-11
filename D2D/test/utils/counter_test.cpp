#include "utils/counter.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <iostream>

using namespace CCPS;

TEST(CounterTests, CounterTest) {
    bool inc_val = false;
    Wire<Bool>::TPFUNC inc_func{
        [&inc_val]() -> Bool {
            return Bool(inc_val);
        }
    };
    Wire<Bool> inc{inc_func};

    const int n = 15;
    ModulePtr<Counter> top = createTopModule<Counter>(inc, n);
    Counter &counter = *top;
    int software_count = 0;

    // ============== 1st round ==============
    inc_val = false;
    // check initial value
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BigUInt(counter.getCount(), 0) << "Counter initial value failed, index: " << i;
        counter.step();
    }

    // check incremental count.
    inc_val = true;
    software_count = 0;
    for (int i = 0; i < 100; i++) {
        software_count = i % n;
        EXPECT_EQ_BigUInt(counter.getCount(), software_count) << "Counter initial value failed, index: " << i;
        if (software_count == n-1) {
            if (counter.isDone()) {
                //std::cout << "Counter is done" << std::endl;
            } else {
                std::cout << "Counter is not done" << std::endl;
                EXPECT_EQ(1, 0) << "Counter is not done";
            }
        }
        counter.step();
    }
    software_count = ++software_count % n; // for the last step

    // check static count.
    inc_val = false;
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BigUInt(counter.getCount(), software_count) << "Counter initial value failed, index: " << i;
        counter.step();
    }

    // ============== 2nd round ==============
    counter.init(inc, n);
    inc_val = false;
    // check initial value
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BigUInt(counter.getCount(), 0) << "Counter initial value failed, index: " << i;
        counter.step();
    }

    // check incremental count.
    inc_val = true;
    software_count = 0;
    for (int i = 0; i < 100; i++) {
        software_count = i % n;
        EXPECT_EQ_BigUInt(counter.getCount(), software_count) << "Counter initial value failed, index: " << i;
        if (software_count == n-1) {
            if (counter.isDone()) {
                //std::cout << "Counter is done" << std::endl;
            } else {
                std::cout << "Counter is not done" << std::endl;
                EXPECT_EQ(1, 0) << "Counter is not done";
            }
        }
        counter.step();
    }
    software_count = ++software_count % n; // for the last step

    // check static count.
    inc_val = false;
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ_BigUInt(counter.getCount(), software_count) << "Counter initial value failed, index: " << i;
        counter.step();
    }
}

TEST(CounterTests, CounterResetTest) {
    bool inc_val = false;
    Wire<Bool>::TPFUNC inc_func{
        [&inc_val]() -> Bool {
            return Bool(inc_val);
        }
    };
    Wire<Bool> inc{inc_func};

    bool reset_val = false;
    Wire<Bool> reset;
    reset.capture(reset_val);

    const int n = 15;
    ModulePtr<Counter> top = createTopModule<Counter>(inc, n);
    Counter &counter = *top;
    counter.setReset(reset);

    // run
    inc_val = true;
    for (int i = 0; i < n/2; i++) {
        EXPECT_EQ_BigUInt(counter.getCount(), i) << "Counter initial value failed, index: " << i;
        counter.step();
    }

    reset_val = true;
    counter.step();
    reset_val = false;

    for (int i = 0; i < n; i++) {
        EXPECT_EQ_BigUInt(counter.getCount(), i) << "Counter initial value failed, index: " << i;
        counter.step();
    }

}