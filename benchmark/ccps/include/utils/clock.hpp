#ifndef __CLOCK_HPP__
#define __CLOCK_HPP__

#include "module.hpp"
#include <vector>

namespace CCPS{
    // =============================== BaseClock ==========================
    class BaseClock {
    public:
        BaseClock(const std::string &name);
        virtual ~BaseClock() = default;

        virtual void step() = 0;

        virtual void initNextState();
        virtual void calcNextState();
        virtual void applyNextState();
        void addModule(std::shared_ptr<Module> m);
        void clearModules();
        // get clock period in ps.
        virtual int getPeriod() = 0;
        std::vector<std::shared_ptr<Module>>& getModules();
        const std::string& getName();
        virtual void updateCurrentTime(int time_in_ps);
        bool isPosedge();
        int getLevel();
        void addDerivedClock(const ClockPtr &derived_clock);
        void showClockTree();

    private:
        std::string _name;
    protected:
        bool _is_posedge{false};
        int _level{0};
        std::vector<ClockPtr> _derived_clocks;
        std::vector<std::shared_ptr<Module>> _modules;
        int current_cycle_{0};
    };


    // =============================== Clock ==========================
    class Clock: public BaseClock {
    public:

        Clock(const int period_in_ps, const std::string &name);
        Clock(const Clock&) = delete;
        ~Clock();

        // Only for single clock domain.
        void step() override;

        int getPeriod() override;
        void updateCurrentTime(int time_in_ps) override;

    private:
        Clock& operator=(const Clock&) = delete;
        const int _period_in_ps;
        //int _disabled_level{0};

    };

    // =============================== GatedClock ==========================
    class GatedClock: public BaseClock {
    public:
        using ENABLE_FUNC = std::function<bool()>;

        GatedClock(ClockPtr master_clock, const std::string &name, ENABLE_FUNC enable_func);
        ~GatedClock();

        //void setEnableFunc(ENABLE_FUNC);
        void step() override;

        void initNextState() override;
        void calcNextState() override;
        void applyNextState() override;

        int getPeriod() override;
        void updateCurrentTime(int time_in_ps) override;

    private:
        ENABLE_FUNC _is_enable{
            []() -> bool {
                return true;
            }
        };
        ClockPtr _master_clock;
    };

    // =============================== functions ==========================
    //void clearAllClockModules();
    ClockPtr createClock(const int period, const std::string &name);
    ClockPtr createGatedClock(ClockPtr master_clock, const std::string &name, GatedClock::ENABLE_FUNC enable_func);
}

#endif // __CLOCK_HPP__