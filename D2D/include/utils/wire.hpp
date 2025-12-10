#ifndef __WIRE_HPP__
#define __WIRE_HPP__

#include "base_types.hpp"
#include "reg.hpp"
#include <iostream>
#include <functional>
#include <vector>
#include <type_traits>
#include <cassert>


namespace CCPS {
    // ======================= Wire ==========================
    template <typename T>
    class Wire {
    public:
        using TPFUNC=std::function<T()>;
        // Constructor with initial value, get rid of const to avoid ambiguity with Wire(TPFUNC&&)
        explicit Wire(const TPFUNC pf): _pf(pf) {
            static_assert(std::is_base_of<Data, T>::value, "type parameter of this class must derive from Data");
        }
        // prevent rvalue to be passed to the constructor, to avoid dangling _pf.
        Wire(TPFUNC&& pf) = delete;
        Wire(): _pf(nullptr) {}
        Wire(const T& data) = delete;
        Wire(Wire<T>&&) = delete;
        Wire(const Wire<T>& val) {
            //std::cout << "Wire constructor from Wire<T>&, val: " << &val << std::endl;
            assign(val);
        }
        T operator()() const {
            if (_pf == nullptr) {
                std::cerr << "Error: Wire has not been initialized." << std::endl;
                assert(false);
            }
            //std::cout << "call wire pf " << std::endl;
            return (_pf)();
        }
        Wire<T>& operator=(const Wire<T>& val) {
            //std::cout << "Assigning wire value with const wire" << std::endl;
            assign(val);
            return *this;
        }
        Wire<T>& operator=(const TPFUNC pf) {
            //std::cout << "Assign pf to wire, pf " << &pf << std::endl;
            assign(pf);
            return *this;
        }
        Wire<T>& operator=(RegPtr<T> &reg) {
            TPFUNC pf = [&reg]() -> T {
                return reg->read();
            };
            return *this = pf;
        }
        void capture(bool &val) {
            static_assert(std::is_base_of<Bool, T>::value, "type parameter of this function must be Bool");
            TPFUNC pf = [&val]() -> Bool {
                return Bool(val);
            };
            *this = pf;
        }
        void capture(const int width, BigUInt &val) {
            static_assert(std::is_base_of<UInt, T>::value, "type parameter of this function must be Bool");
            TPFUNC pf = [width, &val]() -> UInt {
                return UInt(width, val);
            };
            *this = pf;
        }
        void capture(bool &&val) {
            static_assert(std::is_base_of<Bool, T>::value, "type parameter of this function must be Bool");
            TPFUNC pf = [val]() -> Bool {
                return Bool(val);
            };
            *this = pf;
        }
        // TODO, this is dangerous, because passing an argument which will be implicited converted into BigUInt
        // may call this unintentionally.
        void capture(const int width, BigUInt &&val) {
            static_assert(std::is_base_of<UInt, T>::value, "type parameter of this function must be Bool");
            TPFUNC pf = [width, val]() -> UInt {
                return UInt(width, val);
            };
            *this = pf;
        }
        // Update the value function
        void update(const Wire<T>& val) {
            if (this == &val) {
                return;
            }
            _pf = nullptr;
            assign(val);
        }
        bool isEmpty() const {
            return _pf == nullptr;
        }

    private:
        // Assign a value function.
        void assign(TPFUNC pf) {
            if (_pf != nullptr) {
                std::cerr << "assign pf Error: Wire has been initialized." << std::endl;
                assert(false);
            }
            if (pf == nullptr) {
                std::cerr << "assign nullptr function." << std::endl;
                assert(false);
            }
            _pf = pf;
        }
        // Assign a wire value.
        void assign(const Wire<T> &val) {
            if (_pf != nullptr) {
                std::cerr << "assign val Error: Wire has been initialized." << std::endl;
                assert(false);
            }
            //std::cout << "Assign wire value, val: " << &val << std::endl;
            _pf = [&val]() -> T {
                //std::cout << "running wire value function, val: " << &val << std::endl;
                return val();
            };
        }
        TPFUNC get() const {
            return _pf;
        }
        // Get current value explicitly
        T read() const {
            return (*this)();
        }

        TPFUNC _pf; // Evalue function to get value.
    }; // Wire
} // namespace CCPS

#endif // __WIRE_HPP__