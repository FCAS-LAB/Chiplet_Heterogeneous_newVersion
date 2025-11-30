#ifndef __VALID_HPP__
#define __VALID_HPP__

#include "base_types.hpp"
#include "wire.hpp"
#include "io_direction.hpp"
#include "utils/reg.hpp"
#include <string>
#include <cassert>

namespace CCPS {
    template <typename T>
    class Valid {
    public:
        Valid() : _valid{} {}
        Valid(const bool flipped): Valid() {
            if (flipped) {
                flip();
            }
        }

        // Get data
        T bits() const {
            //std::cout << "bits: " << _bits()() << std::endl;
            return _bits();
        }

        void assignValid(const Wire<Bool>& valid, std::string debug_info = "") {
            if (debug_info.size() > 0) {
                this->_valid = [&valid, debug_info]() -> Bool {
                    std::cout << "in Valid assignValid, debug info: " << debug_info << std::endl;
                    //std::cout << "Valid assign valid: " << valid()() << " debug_info: "<< debug_info << std::endl;
                    return valid();
                };
            } else {
                this->_valid = std::move(valid);
            }
        }
        void assignBits(const Wire<T>& bits) {
            this->_bits = bits;
        }
        void assignValid(Wire<Bool>::TPFUNC func) {
            this->_valid = func;
        }
        void assignBits(typename Wire<T>::TPFUNC func) {
            this->_bits = func;
        }
        void assignValid(Wire<Bool>&& valid, std::string debug_info = "") = delete;
        void assignBits(Wire<T>&& bits) = delete;
        void assignValid(bool &val) {
            this->_valid.capture(val);
        }
        void assignBits(int width, BigUInt &val) {
            this->_bits.capture(width, val);
        }
        void assignValid(bool &&val) {
            this->_valid.capture(std::move(val));
        }
        void assignBits(int width, BigUInt &&val) {
            this->_bits.capture(width, std::move(val));
        }
        void assignValid(RegPtr<Bool> &valid) {
            this->_valid = valid;
        }
        void assignBits(RegPtr<T> &bits) {
            this->_bits = bits;
        }
        void assignValid(const Wire<Bool>&&) = delete;
        void assignBits(const Wire<T>&& bits) = delete;

        Bool isValid() const {
            //auto temp = _valid();
            //auto temp2 = temp();
            //std::cout << "Valid isValid: " << temp2 << std::endl;
            return _valid();
        }

        void connect(Valid<T>& other, std::string debug_info = "") {
            //io_dir_is_capatable(other);

            //if (dir() == IO_DIRECTION::MASTER) {
            //    other.connect(*this);
            //} else {
            //    this->assignValid(other._valid);
            //    this->assignBits(other._bits);
            //}
            this->assignValid(other._valid, debug_info);
            this->assignBits(other._bits);
        }

        void io_dir_is_capatable(Valid<T>& other) {
            if (dir() == IO_DIRECTION::MASTER) {
                assert(other.dir() == IO_DIRECTION::SLAVE);
            }
            else if (dir() == IO_DIRECTION::SLAVE) {
                assert(other.dir() == IO_DIRECTION::MASTER);
            } else {
                std::cerr << "unsupported _dir: " << _dir << std::endl;
                exit(-1);
            }
        }

        IO_DIRECTION dir() const {
            return _dir;
        }

        void flip() {
            if (_dir == IO_DIRECTION::MASTER) {
                _dir = IO_DIRECTION::SLAVE;
            } else {
                _dir = IO_DIRECTION::MASTER;
            }
        }

    private:
        Wire<T> _bits;
        Wire<Bool> _valid;
        IO_DIRECTION _dir{IO_DIRECTION::MASTER}; // master or slave
    };
};

#endif //__VALID_HPP__