#ifndef __TIME_SLICE_HPP__
#define __TIME_SLICE_HPP__

#include "utils/clock.hpp"

namespace CCPS {

    class TimeSlice {
    public:
        // thread-safe since C++11
        static TimeSlice& getInstance() {
            static TimeSlice instance;
            return instance;
        }

        void addClock(ClockPtr c);
        const std::vector<ClockPtr>& getClocks();
        // time_limit_in_ps == -1 means run unlimited.
        // all clocks start at time 0, to avoid phase mismatch for gated clocks with the master clock.
        void run(const int time_limit_in_ps);
        void clearClocks();
        void showClockTree();
        int getTime();
        void resetTime();
    private:
        TimeSlice() = default;
        ~TimeSlice() = default;
        std::vector<ClockPtr> _clocks;
        int _current_time{0};
    };

} // namespace CCPS

#endif // __TIME_SLICE_HPP__