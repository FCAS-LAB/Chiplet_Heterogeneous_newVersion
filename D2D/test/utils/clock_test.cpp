#include "utils/base_types.hpp"
#include "test_utils.hpp"
#include "utils/module.hpp"
#include "utils/time_slice.hpp"
#include "utils/counter.hpp"
#include <gtest/gtest.h>

using namespace CCPS;

class ClockDomain1: public RegModule {
public:
    void calcNextState() override {
        //std::cout << "in clock domain1" << std::endl;
    }
};

class ClockDomain2: public RegModule {
public:
    ModulePtr<ClockDomain1> d1;

    ClockDomain2() {
        d1 = createSubmodule<ClockDomain1>("clock_domain1");
    }

    bool propagateClock() override {
        auto c = createClock(50000, "clock1");
        addModuleToClock(c, d1);
        return Module::propagateClock();
    }

    void calcNextState() override {
        //std::cout << "in clock domain2" << std::endl;
    }
};

TEST(ClockTests, MultipleClocksTest) {
    auto top = createTopModule<ClockDomain2>();
    auto &c = *top;

    TimeSlice::getInstance().run(1000*1000); // run 1 us
}

class ClockDomain3: public RegModule {
    public:

        Wire<Bool> count_inc;
        ModulePtr<Counter> count;
        ModulePtr<ClockDomain1> d1;
        GatedClock::ENABLE_FUNC clock1_enable_func;
        RegPtr<Bool> r_clock1_enabled;

        ClockDomain3() {
            count_inc = [this]() -> Bool {
                return Bool(true);
            };
            count = createSubmodule<Counter>("counter", count_inc, 16);
            d1 = createSubmodule<ClockDomain1>("clock_domain1");
            r_clock1_enabled = createReg<Bool>(Bool(true));
        }

        bool propagateClock() override {
            clock1_enable_func = [this]() -> bool {
                return static_cast<bool>(r_clock1_enabled->read());
            };
            auto c = createGatedClock(getClock(), "clock1", clock1_enable_func);
            addModuleToClock(c, d1);
            return Module::propagateClock();
        }

        void calcNextState() override {
            //std::cout << "in clock domain2" << std::endl;

            if (count->isDone()) {
                //std::cout << "clock1 enable: " << !r_clock1_enabled->read().operator bool() << std::endl;
                *r_clock1_enabled = !r_clock1_enabled->read();
            }
        }
    };


TEST(ClockTests, GatedClockTest) {
    auto top = createTopModule<ClockDomain3>();
    auto &c = *top;

    TimeSlice::getInstance().run(1000*1000*10); // run 10 us
}