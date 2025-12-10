#include "utils/clock.hpp"
#include "utils/time_slice.hpp"
#include "utils/print.hpp"

namespace CCPS {
    // =============================== BaseClock ==========================
    BaseClock::BaseClock(const std::string &name): _name(name) {
        ENTER_FUNC();
    }

    //BaseClock::~BaseClock() {}

    void BaseClock::initNextState() {
        ENTER_FUNC();
        // update this clock domain
        if (isPosedge()) {
            for (auto m : _modules) {
                m->initNextState();
            }
        }

        // iterate derived clock domains.
        for (auto &d : _derived_clocks) {
            d->initNextState();
        }
    }
    void BaseClock::calcNextState() {
        ENTER_FUNC();
        // update this clock domain
        if (isPosedge()) {
            for (auto m : _modules) {
                m->calcNextState();
            }
        }

        // iterate derived clock domains.
        for (auto &d : _derived_clocks) {
            d->calcNextState();
        }
    }
    void BaseClock::applyNextState() {
        ENTER_FUNC();
        // update this clock domain
        if (isPosedge()) {
            for (auto m : _modules) {
                m->applyNextState();
            }
        }

        // iterate derived clock domains.
        for (auto &d : _derived_clocks) {
            d->applyNextState();
        }
    }

    void BaseClock::addModule(std::shared_ptr<Module> m) {
        ENTER_FUNC();
        _modules.push_back(m);
    }

    void BaseClock::clearModules() {
        ENTER_FUNC();
        _modules.clear();
    }

    std::vector<std::shared_ptr<Module>>& BaseClock::getModules() {
        ENTER_FUNC();
        return _modules;
    }

    const std::string& BaseClock::getName() {
        return _name;
    }

    void BaseClock::updateCurrentTime(int time_in_ps) {
        ENTER_FUNC();
        for (auto &c : _derived_clocks) {
            c->updateCurrentTime(time_in_ps);
        }
        //std::cout << "current time " << time_in_ps
        //          << ", clock " << getName()
        //          << " is posedge: " << isPosedge()
        //          << std::endl;
    }

    bool BaseClock::isPosedge() {
        return _is_posedge;
    }

    int BaseClock::getLevel() {
        return _level;
    }

    void BaseClock::addDerivedClock(const ClockPtr &derived_clock) {
        ENTER_FUNC();
        assert(derived_clock != nullptr);
        _derived_clocks.push_back(derived_clock);
    }

    void BaseClock::showClockTree() {
        ENTER_FUNC();
        std::cout << "clock name: " << getName() << ", period: " << getPeriod() << std::endl;
        for (auto& m : getModules()) {
            std::cout << "    module: " << m->getPathName() << std::endl;
        }

        for (auto &d : _derived_clocks) {
            d->showClockTree();
        }
    }

    // =============================== Clock ==========================
    Clock::Clock(const int period_in_ps, const std::string &name):
        _period_in_ps(period_in_ps), BaseClock(name) {
            ENTER_FUNC();
        }

    Clock::~Clock() {
        ENTER_FUNC();
        //std::cout << "destroy clock, period " << _period_in_ps << std::endl;
        clearModules();
    }

    void Clock::step() {
        std::cout << "=============== step " << std::dec << current_cycle_++ << " ====================" << std::endl;
        for (auto m : _modules) {
            m->initNextState();
        }
        for (auto m : _modules) {
            m->calcNextState();
        }
        for (auto m : _modules) {
            m->applyNextState();
        }
    }

    int Clock::getPeriod() {
        return _period_in_ps;
    }

    void Clock::updateCurrentTime(int time_in_ps) {
        ENTER_FUNC();
        int p = time_in_ps % _period_in_ps;
        _is_posedge = false;
        if (p < _period_in_ps / 4) {
            _level = 0;
        } else if (p < _period_in_ps / 2) {
            _level = 0;
        } else if (p < _period_in_ps*3/4 && _level == 0) {
            _level = 1;
            _is_posedge = true;
        } else {
            _level = 1;
        }

        BaseClock::updateCurrentTime(time_in_ps);
    }

    // =============================== GatedClock ==========================
    GatedClock::GatedClock(ClockPtr master_clock, const std::string &name, ENABLE_FUNC enable_func):
        _master_clock(master_clock), _is_enable(enable_func), BaseClock(name)
    {
        ENTER_FUNC();
        assert(master_clock != nullptr);
    }

    GatedClock::~GatedClock() {
        ENTER_FUNC();
    }

    void GatedClock::step() {
        std::cerr << "GatedClock does not support step yet." << std::endl;
        assert(false);
    }

    void GatedClock::initNextState() {
        ENTER_FUNC();
        if (!_is_enable()) {
            return;
        }

        BaseClock::initNextState();
    }
    void GatedClock::calcNextState() {
        ENTER_FUNC();
        if (!_is_enable()) {
            return;
        }

        BaseClock::calcNextState();
    }
    void GatedClock::applyNextState() {
        ENTER_FUNC();
        if (!_is_enable()) {
            return;
        }

        BaseClock::applyNextState();
    }
    int GatedClock::getPeriod() {
        if (_master_clock == nullptr) {
            std::cerr << "get period from nullptr master_clock" << std::endl;
            assert (false);
        }
        return _master_clock->getPeriod();
    }
    void GatedClock::updateCurrentTime(int time_in_ps) {
        ENTER_FUNC();
        _is_posedge = false;

        if (!_is_enable()) {
            _level = 0;
        } else {
            int master_level = _master_clock->getLevel();
            if (master_level == 1 && _level == 0) {
                _is_posedge = true;
            }
            _level = master_level;
        }

        BaseClock::updateCurrentTime(time_in_ps);
    }

    // =============================== functions ==========================
    ClockPtr createClock(const int period, const std::string &name) {
        ENTER_FUNC();
        ClockPtr c = std::make_shared<Clock>(period, name);
        TimeSlice::getInstance().addClock(c);
        return c;
    }

    ClockPtr createGatedClock(ClockPtr master_clock, const std::string &name, GatedClock::ENABLE_FUNC enable_func) {
        ENTER_FUNC();
        ClockPtr c = std::make_shared<GatedClock>(master_clock, name, enable_func);
        master_clock->addDerivedClock(c);
        return c;
    }

} // namespace CCPS