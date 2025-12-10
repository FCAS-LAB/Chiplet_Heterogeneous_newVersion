#include "utils/time_slice.hpp"
#include <iostream>
#include <numeric>

namespace CCPS {

    void TimeSlice::addClock(ClockPtr c) {
        _clocks.push_back(c);
    }

    const std::vector<ClockPtr>& TimeSlice::getClocks() {
        return _clocks;
    }

    void TimeSlice::run(const int time_limit_in_ps) {
        size_t clock_num = _clocks.size();
        if (clock_num == 0) {
            return;
        }

        // get greatest common divisor clock periods
        int gcd = _clocks[0]->getPeriod();
        for (size_t i = 1; i < clock_num; i++) {
            gcd = std::gcd(gcd, _clocks[i]->getPeriod());
        }

        // get least common multiple
        int lcm = _clocks[0]->getPeriod();
        for (size_t i = 1; i < clock_num; i++) {
            lcm = std::lcm(lcm, _clocks[i]->getPeriod());
        }

        // run
        int time_offset = 0;
        const int time_slice = gcd/8;
        while (time_limit_in_ps > 0 && time_offset < time_limit_in_ps) {
            //std::cout << "time_limit_in_ps " << time_limit_in_ps << " current_time " << _current_time << std::endl;
            for (const auto& c : _clocks) {
                c->updateCurrentTime(_current_time);
            }
            for (const auto& c : _clocks) {
                c->initNextState();
            }
            for (const auto& c : _clocks) {
                c->calcNextState();
            }
            for (const auto& c : _clocks) {
                c->applyNextState();
            }
            _current_time += time_slice;
            time_offset += time_slice;
        }
    }

    void TimeSlice::clearClocks() {
        for (auto& c : _clocks) {
            c->clearModules();
        }
        _clocks.clear();
    }

    // TODO, update
    void TimeSlice::showClockTree() {
        std::cout << "clocks tree:" << std::endl;
        for (auto& c : _clocks) {
            c->showClockTree();
        }
    }

    int TimeSlice::getTime() {
        return _current_time;
    }
    void TimeSlice::resetTime() {
        _current_time = 0;
    }
} // namespace CCPS