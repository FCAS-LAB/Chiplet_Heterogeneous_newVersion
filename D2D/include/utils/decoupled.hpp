#ifndef __DECOUPLED_HPP__
#define __DECOUPLED_HPP__

#include "valid.hpp"

namespace CCPS {
    template <typename T>
    class Decoupled: public Valid<T> {
    public:
        Decoupled() : Valid<T>(), _ready{} {}
        Decoupled(const bool flipped): Decoupled() {
            if (flipped) {
                Valid<T>::flip();
            }
        }

        // Get data
        T bits() const {
            return Valid<T>::bits();
        }

        void assignReady(const Wire<Bool>& ready) {
            this->_ready = ready;
        }
        void assignBits(const Wire<T>& bits) {
            Valid<T>::assignBits(bits);
        }
        void assignValid(const Wire<Bool>& valid) {
            Valid<T>::assignValid(valid);
        }
        void assignReady(Wire<Bool>::TPFUNC func) {
            this->_ready = func;
        }
        void assignBits(typename Wire<T>::TPFUNC func) {
            Valid<T>::assignBits(func);
        }
        void assignValid(Wire<Bool>::TPFUNC func) {
            Valid<T>::assignValid(func);
        }
        void assignReady(bool &val) {
            this->_ready.capture(val);
        }
        void assignBits(int width, BigUInt &val) {
            Valid<T>::assignBits(width, val);
        }
        void assignValid(bool &val) {
            Valid<T>::assignValid(val);
        }
        void assignReady(bool &&val) {
            this->_ready.capture(std::move(val));
        }
        void assignBits(int width, BigUInt &&val) {
            Valid<T>::assignBits(width, std::move(val));
        }
        void assignValid(bool &&val) {
            Valid<T>::assignValid(std::move(val));
        }
        void assignReady(RegPtr<Bool> &ready) {
            this->_ready = ready;
        }
        void assignBits(RegPtr<T> &bits) {
            Valid<T>::assignBits(bits);
        }
        void assignValid(RegPtr<Bool> &valid) {
            Valid<T>::assignValid(valid);
        }
        void assignReady(const Wire<Bool>&&) = delete;
        void assignBits(const Wire<T>&& bits) = delete;
        void assignValid(const Wire<Bool>&& valid) = delete;

        // Check if data is ready
        Bool isReady() const {
            return _ready();
        }
        Bool isValid() const {
            return Valid<T>::isValid();
        }

        Bool fire() const {
            return isValid() && isReady();
        }

        void connect(Decoupled<T>& other, std::string debug_info = "") {
            //Valid<T>::io_dir_is_capatable(other);

            //if (Valid<T>::dir() == IO_DIRECTION::MASTER) {
            //    other.connect(*this);
            //} else {
            //    Valid<T>::connect(other);
            //    other.assignReady(this->_ready);
            //}
            Valid<T>::connect(other, debug_info);
            other.assignReady(this->_ready);
        }

        IO_DIRECTION dir() const {
            return Valid<T>::dir();
        }
        void flip() {
            Valid<T>::flip();
        }

    private:
        Wire<Bool> _ready;
    };
} // namespace CCPS

#endif //__DECOUPLED_HPP__