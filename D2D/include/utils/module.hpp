#ifndef __MODULE_HPP__
#define __MODULE_HPP__

#include "utils/reg.hpp"
#include "utils/common.hpp"
#include "utils/wire.hpp"
#include <vector>
#include <memory>
#include <string>
#include "utils/print.hpp"

namespace CCPS {
    class BaseClock;
    class RegModule;
    class Module;
    class TimeSlice;


    template<typename T>
    using ModulePtr = std::shared_ptr<T>;

    using ClockPtr = std::shared_ptr<BaseClock>;

    // =============================== functions ==========================
    void clearAllClockModules();
    void addModuleToClock(const ClockPtr &clk, std::shared_ptr<Module> m);
    ClockPtr createClock(const int period, const std::string &name);

    // factory creater
    template <typename T, typename ...Args>
    // must pass Args as reference, because copying Wire/Function
    // will make the downstream Wire reference invalid(dangling).
    // For single clock domain.
    ModulePtr<T> createTopModule(Args&& ... args) {
        static_assert(std::is_base_of<Module, T>::value,
            "type parameter of this class must derive from Module");

        clearAllClockModules();
        resetSimTime();

        ModulePtr<T> m = std::make_shared<T>(std::forward<Args>(args)...);
        m->setModuleName("top");
        m->buildModuleTree("");
        addModuleToClock(createClock(100000, "main_clock"), m);
        m->topPropagateClock();
        return m;
    }

    // =============================== Module ==========================
    class Module {
        public:
            //virtual void step() = 0;
            Module();
            void step(int steps=1);
            virtual ~Module();

            // A step is divided into 3 phases.
            // In the first phase, the module initializes the next state.
            // In the second phase, the module calculates the next state.
            // In the third phase, the module copy values from the next state to the current state.
            virtual void initNextState() = 0;
            virtual void calcNextState() = 0;
            virtual void applyNextState() = 0;

            // setClock must be called before createSubmodule.
            void setClock(ClockPtr clk);
            ClockPtr getClock();
            bool isReset();

            void setReset(Wire<Bool> &reset);
            Wire<Bool>& getReset();


            // set module name for debug
            void setModuleName(const std::string &module_name);
            // get path name for debug
            const std::string& getPathName();
            // build submodule trees from top to bottom.
            void buildModuleTree(const std::string &upper_path);
            // return true for success.
            // A module decide if to add it self to the solved vector.
            // A module decide if to propagate
            // Set submodule main clock and iterate submodule propagateClock.
            // Set other clocks of this module.
            virtual bool propagateClock();
            bool topPropagateClock();
            std::vector<std::shared_ptr<Module>>& getSubmodules();

            // factory creater
            template <typename T, typename ...Args>
            // must pass Args as reference, because copying Wire/Function
            // will make the downstream Wire reference invalid(dangling).
            ModulePtr<T> createSubmodule(
                const std::string &module_name,
                Args&& ... args) {
                static_assert(std::is_base_of<Module, T>::value,
                    "type parameter of this class must derive from Module");
                ModulePtr<T> m = std::make_shared<T>(std::forward<Args>(args)...);
                m->setModuleName(module_name);
                _submodules.push_back(m);
                //addModuleToClock(getClock(), m);
                return m;
            }

            //// factory creater
            //template <typename T, typename ...Args>
            //// must pass Args as reference, because copying Wire/Function
            //// will make the downstream Wire reference invalid(dangling).
            //ModulePtr<T> createSubmoduleWithClock(
            //    const ClockPtr &clk,
            //    const std::string &module_name,
            //    Args&& ... args) {
            //    static_assert(std::is_base_of<Module, T>::value,
            //        "type parameter of this class must derive from Module");
            //    ModulePtr<T> m = std::make_shared<T>(std::forward<Args>(args)...);
            //    m->setModuleName(module_name);
            //    _submodules.push_back(m);
            //    addModuleToClock(clk, m);
            //    return m;
            //}

        private:
            std::string _module_name; // module name
            std::string _path_name;
            ClockPtr _clock;
            Wire<Bool> _reset;
            std::vector<std::shared_ptr<Module>> _submodules;
            //std::vector<bool> _clock_solved_indicator;
    };

    // =============================== WireModule ==========================
    // WireModule is a module that has only wire connections.
    class WireModule: public Module {
    public:
        virtual ~WireModule() = default;

        void initNextState()  override final {};
        void calcNextState()  override {};
        void applyNextState() override final {};

    };

    // =============================== RegModule ==========================
    // RegModule is a module that has reg connections.
    class RegModule: public Module {
    public:
        virtual ~RegModule() = default;

        void initNextState() override;
        virtual void calcNextState();
        void applyNextState() override;

    protected:
        template <typename T, typename ...Args>
        RegPtr<T> createReg(Args ... args) {
            RegPtr<T> r = std::make_shared<Reg<T>>(std::forward<Args>(args)...);
            _reg_pool.push_back(r);
            return r;
        }
        RegModule() = default;
    private:
        std::vector<std::shared_ptr<RegBase>> _reg_pool;
    };

} // namespace CCPS

#endif // __MODULE_HPP__
