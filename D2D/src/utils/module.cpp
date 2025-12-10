#include "utils/module.hpp"
#include "utils/clock.hpp"
#include "utils/time_slice.hpp"
#include "utils/print.hpp"

namespace CCPS {

    // =============================== functions ==========================
    void clearAllClockModules() {
        ENTER_FUNC();
        TimeSlice::getInstance().clearClocks();
    }

    void addModuleToClock(const ClockPtr &clk, std::shared_ptr<Module> m) {
        ENTER_FUNC();
        clk->addModule(m);
        m->setClock(clk);
    }

    // =============================== Module ==========================

    Module::Module() {
        ENTER_FUNC();
    }

    Module::~Module() {
        ENTER_MODULE_FUNC();
        // Clear modules for next test, but at this time the clock may be dangling.
        //clearCLockModules(getClock());
    }

    void Module::step(int steps) {
        ENTER_MODULE_FUNC();
        for (int i = 0; i < steps; i++) {
            //std::cout << "========================= step ==========================" << std::endl;
            _clock->step();
        }
    }

    void Module::setClock(ClockPtr clk) {
        ENTER_MODULE_FUNC();
        //if (_submodules.size() > 0) {
        //    std::cerr << "setClock must be called before createSubmodule." << std::endl;
        //    exit(-1);
        //}
        if (clk == nullptr) {
            std::cerr << _path_name << ", clk is nullptr" << std::endl;
            assert (false);
        }
        if (_clock != nullptr) {
            std::cerr << _path_name << ": clock has already been set." << std::endl;
            assert (false);
        }
        _clock = clk;
        //std::cout << "setClock: " << _path_name << " period " << _clock->getPeriod() << std::endl;
    }

    ClockPtr Module::getClock() {
        return _clock;
    }

    void Module::setReset(Wire<Bool> &reset) {
        _reset = reset;
    }
    Wire<Bool>& Module::getReset() {
        return _reset;
    }
    bool Module::isReset() {
        if (_reset.isEmpty()) {
            return false;
        }
        return static_cast<bool>(_reset());
    }

    void Module::setModuleName(const std::string &module_name) {
        ENTER_MODULE_FUNC();
        _module_name = module_name;
    }

    const std::string& Module::getPathName() {
        return _path_name;
    }

    void Module::buildModuleTree(const std::string &upper_path) {
        _path_name = upper_path + "." + _module_name;
        for (auto& m : _submodules) {
            m->buildModuleTree(_path_name);
        }
    }

    bool Module::propagateClock() {
        ENTER_MODULE_FUNC();
        if (_clock == nullptr) {
            std::cerr << _path_name << " clock has not been set." << std::endl;
            assert(false);
            return false;
        }
        bool success = true;
        for (auto& m : _submodules) {
            // May be the submodule clock is set when created.
            if (m->getClock() == nullptr) {
                addModuleToClock(_clock, m);
            }
            if (!m->propagateClock()) {
                success = false;
            }
        }
        return success;
    }

    bool Module::topPropagateClock() {
        ENTER_MODULE_FUNC();
        bool all_solved = false;
        while (!all_solved) {
            all_solved = propagateClock();
        }
        return all_solved;
    }

    std::vector<std::shared_ptr<Module>>& Module::getSubmodules() {
        return _submodules;
    }

    // =============================== RegModule ==========================
    void RegModule::initNextState() {
        ENTER_MODULE_FUNC();
    }

    void RegModule::calcNextState() {
        ENTER_MODULE_FUNC();
        if (isReset()) {
            std::cout << getPathName() << ": reset" << std::endl;
            for (auto& reg : _reg_pool) {
                reg->reset();
            }
        }
    }

    void RegModule::applyNextState() {
        ENTER_MODULE_FUNC();
        for (auto& reg : _reg_pool) {
            reg->update();
        }
    }

} // namespace CCPS