#ifndef __REG_HPP__
#define __REG_HPP__

#include <iostream>
#include <functional>
#include <vector>
#include <type_traits>
#include "base_types.hpp"


namespace CCPS {
    class RegModule;


    class RegBase {
    public:
        virtual void update() = 0;
        virtual void reset() = 0;
    };

    template <typename T>
    class Reg: public RegBase{
        //friend RegModule::createReg<T>(const T &init_val);

    private:
        T _current_value;
        T _next_value;
        T _default_value; // used for reset
        std::string _debug_info;

        //// Constructor with initial value
        //Reg(const T &init_val) : _current_value(init_val), _next_value(init_val) {
        //    static_assert(std::is_base_of<Data, T>::value, "type parameter of this class must derive from Data");
        //}

        //Reg() {
        //    static_assert(std::is_base_of<Data, T>::value, "type parameter of this class must derive from Data");
        //}

    public:
        // Constructor with initial value
        Reg(const T &init_val) : _current_value(init_val), _next_value(init_val), _default_value(init_val) {
            static_assert(std::is_base_of<Data, T>::value, "type parameter of this class must derive from Data");
        }
        Reg(T &&init_val) : _current_value(init_val), _next_value(init_val), _default_value(init_val) {
            static_assert(std::is_base_of<Data, T>::value, "type parameter of this class must derive from Data");
        }

        Reg() {
            static_assert(std::is_base_of<Data, T>::value, "type parameter of this class must derive from Data");
        }

        // Get current value (non-blocking read)
        //operator const T() const {
        //    return read();
        //}
        operator const T&() const {
            return read();
        }

        // Assignment for next value (non-blocking write)
        Reg<T>& operator=(const T& val) {
            write(val);
            return *this;
        }

        // Clock update function
        void update() override final {
            if (_debug_info.size()) {
                std::cout << "in Reg, debug_info " << _debug_info
                    << " current_value " << _current_value.toBigUInt()
                    << " next_value " << _next_value.toBigUInt()
                    << std::endl;
            }
            _current_value = _next_value;
        }

        // Initialize the register
        void init(const T& val) {
            _current_value = val;
            _next_value = val;
            _default_value = val;
        }

        // Get current value explicitly
        const T& read() const {
            return _current_value;
        }

        // Set next value explicitly
        void write(const T& val) {
            _next_value = val;
        }

        void setDebug(const std::string &debug_info) {
            _debug_info = debug_info;
        }

        void reset() {
            _next_value = _default_value;
            _current_value = _default_value;
        }
    };



    template<typename T>
    using RegPtr = std::shared_ptr<Reg<T> >;
    //using RegBasePtr = std::shared_ptr<RegBase>;
};

#endif // __REG_HPP__