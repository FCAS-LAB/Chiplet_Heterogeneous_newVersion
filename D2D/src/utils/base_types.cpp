#include "utils/base_types.hpp"
#include <cassert>
#include <iostream>
#include <utility>

namespace CCPS {

    // =========================== Bits ==========================
    Bits::
    Bits(): Bits(0U, 0UL) {};
    // TODO, comment to avoid implicit convension from int to Bits.
    //Bits::
    //Bits(unsigned width): _width(width), _value(_width, 0) {}
    Bits::
    Bits(unsigned width, const long val): Bits(_width, static_cast<const long long>(val)) {}
    Bits::
    Bits(unsigned width, const long long val) {
        update(width, val);
    }
    Bits::
    Bits(unsigned width, const unsigned long val): Bits(width, static_cast<const unsigned long long>(val)) {}
    Bits::
    Bits(unsigned width, const unsigned long long val) {
        update(width, val);
    }
    Bits::
    Bits(unsigned width, const BigUInt val) {
        update(width, val);
    }
    Bits::
    Bits(const Bits& other): _width(other._width) {
        //std::cout << "call Bits lreferenct constructor" << std::endl;
        _value.resize(_width);
        for (unsigned i = 0; i < _width; ++i) {
            _value[i] = other._value[i];
        }
    }
    Bits::
    Bits(Bits&& other): _width(std::exchange(other._width, 0)), _value(std::move(other._value)) {}

    void Bits::update(unsigned width, const long long val) {
        assert(width <= sizeof(long long) * 8);
        _value.resize(width, 0);
        for (unsigned i = 0; i < width; ++i) {
            _value[i] = (val >> i) & 1;
        }
        _width = width;
    }
    void Bits::update(unsigned width, const unsigned long long val) {
        assert(width <= sizeof(unsigned long long) * 8);
        _value.resize(width, 0);
        for (unsigned i = 0; i < width; ++i) {
            _value[i] = (val >> i) & 1;
        }
        _width = width;
    }
    void Bits::update(unsigned width, const BigUInt val) {
        assert(width <= sizeof(BigUInt) * 8);
        _value.resize(width, 0);
        for (unsigned i = 0; i < width; ++i) {
            BigUInt temp = val >> i;
            unsigned char bit = static_cast<unsigned>(temp) & 1;
            _value[i] = bit;
            //_value[i] = (val >> i) & 1;
        }
        _width = width;
    }

    BigUInt Bits::toBigUInt() const {
        return toBigUInt(_width-1, 0);
    }

    BigUInt Bits::toBigUInt(unsigned index) const {
        return toBigUInt(index, index);
    }

    BigUInt  Bits::toBigUInt(unsigned end, unsigned start) const {
        BigUInt value = 0;
        // be careful of end+1 overflow.
        if (_width == 0) {
            return value;
        }
        assert(start <= end && end+1 <= _width && end <= _width);
        // unsigned i will wrap back when reach zero
        for (unsigned i = end; i >= start && i <= end; --i) {
            value <<= 1;
            value |= _value[i];
        }
        return value;
    }

    bool Bits::operator==(const Bits& other) const {
        unsigned w = _width < other._width ? _width : other._width;
        for (unsigned i = 0; i < w; ++i) {
            if (_value[i] != other._value[i]) {
                return false;
            }
        }
        for (unsigned i = w; i < _width; ++i) {
            if (_value[i] != 0) {
                return false;
            }
        }
        for (unsigned i = w; i < other._width; ++i) {
            if (other._value[i] != 0) {
                return false;
            }
        }
        return true;
    }

    bool Bits::operator!=(const Bits& other) const {
        return !(*this == other);
    }

    bool Bits::operator==(const std::string& s) const {
        if (s.size() > 0) {
            if (s[0] != 'b') {
                std::cerr << "Error: the first character of a bit string should be 'b'" << std::endl;
                exit(-1);
            }
        } else {
            std::cerr << "Error: the bit string is empty" << std::endl;
            exit(-1);
        }

        size_t ssize = s.size() - 1;

        unsigned w = _width < ssize ? _width : ssize;
        for (unsigned i = 0; i < w; ++i) {
            const char &c = s[s.size()-1-i];
            bool both_zeros = (_value[i] == 0 && c=='0');
            bool both_ones = (_value[i] == 1 && c=='1');
            bool is_the_same = c=='?' || both_zeros || both_ones;
            if (!is_the_same) {
                return false;
            }
        }
        for (unsigned i = w; i < _width; ++i) {
            if (_value[i] != 0) {
                return false;
            }
        }
        for (unsigned i = w; i < ssize; ++i) {
            const char &c = s[s.size()-1-i];
            bool is_the_same = c=='?' || c=='0';
            if (!is_the_same) {
                return false;
            }
        }

        return true;
    }

    bool Bits::operator!=(const std::string& s) const {
        return !(*this == s);
    }

    Bits Bits::operator|(const Bits& other) const {
        unsigned maxw = (_width > other._width ? _width : other._width);
        unsigned minw = (_width < other._width ? _width : other._width);
        const Bits *max_one = (_width > other._width ? this : &other);
        Bits res{maxw, 0ULL};
        for (unsigned i = 0; i < minw; ++i) {
            res._value[i] = _value[i] | other._value[i];
        }
        for (unsigned i = minw; i < maxw; ++i) {
            res._value[i] = max_one->_value[i];
        }
        return res;
    }
    Bits Bits::operator&(const Bits& other) const {
        unsigned maxw = (_width > other._width ? _width : other._width);
        unsigned minw = (_width < other._width ? _width : other._width);
        const Bits *max_one = (_width > other._width ? this : &other);
        Bits res{maxw, 0ULL};
        for (unsigned i = 0; i < minw; ++i) {
            res._value[i] = _value[i] & other._value[i];
        }
        for (unsigned i = minw; i < maxw; ++i) {
            res._value[i] = !max_one->_value[i];
        }
        return res;
    }
    Bits Bits::operator^(const Bits& other) const {
        unsigned maxw = (_width > other._width ? _width : other._width);
        unsigned minw = (_width < other._width ? _width : other._width);
        const Bits *max_one = (_width > other._width ? this : &other);
        Bits res{maxw, 0ULL};
        for (unsigned i = 0; i < minw; ++i) {
            res._value[i] = _value[i] ^ other._value[i];
        }
        for (unsigned i = minw; i < maxw; ++i) {
            res._value[i] = max_one->_value[i] == 1 ? 1 : 0;
        }
        return res;
    }
    Bits Bits::operator<<(unsigned shift) const {
        Bits res(_width + shift, 0ULL);
        for (unsigned i = 0; i < _width; ++i) {
            res._value[i + shift] = _value[i];
        }
        return res;
    }
    Bits Bits::operator>>(unsigned shift) const {
        Bits res(_width, 0ULL);
        if (shift >= _width) {
            return res;
        }
        for (unsigned i = 0; i < _width - shift; ++i) {
            res._value[i] = _value[i + shift];
        }
        return res;
    }
    //long long Bits::operator=(const long long val) {
    //    update(sizeof(long long) * 8, val);
    //    return val;
    //}
    //unsigned long long Bits::operator=(const unsigned long long val) {
    //    update(sizeof(unsigned long long) * 8, val);
    //    return val;
    //}
    Bits Bits::operator=(const BigUInt val) {
        update(sizeof(BigUInt) * 8, val);
        return *this;
    }
    Bits Bits::operator=(const Bits& other) {
        if (this != &other) {
            _width = other._width;
            _value.resize(_width);
            for (unsigned i = 0; i < _width; ++i) {
                _value[i] = other._value[i];
            }
        }
        return *this;
    }
    Bits Bits::operator=(Bits&& other) {
        if (this != &other) {
            _width = std::exchange(other._width, 0);
            _value = std::move(other._value);
        }
        return *this;
    }
    Bits::operator bool() const {
        return toBigUInt() != 0;
    }
    Bits Bits::xorR() const {
        long res = 0;
        for (unsigned i = 0; i < _width; ++i) {
            res ^= _value[i];
        }
        return Bits(1, res);
    }

    unsigned Bits::size() const {
        return _width;
    }
    const std::vector<unsigned char>& Bits::value() const {
        return _value;
    }
    Bits& Bits::append(const Bits& other) {
        _value.insert(_value.begin(), other._value.begin(), other._value.end());
        _width += other._width;
        return *this;
    }
    const std::vector<unsigned char>& Bits::getValue() const {
        return _value;
    }

    // =========================== UInt ==========================
    UInt::
    UInt(): UInt(0, 0) {}
    //UInt::
    //UInt(unsigned width): Bits(width) {}
    //UInt::
    //UInt(unsigned width, const unsigned long val): Bits(width, val) {}
    UInt::
    UInt(unsigned width, const BigUInt val): Bits(width, val) {}
    UInt::
    UInt(const UInt& other): Bits(other) {}
    UInt::
    UInt(UInt&& other): Bits(std::move(other)) {
        //std::cout << "call UInt rreferenct constructor" << std::endl;
    }
    //UInt::
    //operator BigUInt() const {
    //    return (*this)();
    //}


    //UInt UInt::operator()() const {
    //    return operator()(_width - 1, 0);
    //}
    UInt UInt::operator()(unsigned index) const {
        return operator()(index, index);
    }
    UInt UInt::operator()(unsigned end, unsigned start) const {
        // be careful of end+1 overflow.
        if (_width == 0) {
            return *this;
        }
        if (end == _width - 1 && start == 0) {
            return *this;
        }
        assert(start <= end && end+1 <= _width && end <= _width);
        UInt res(end - start + 1, 0);
        // unsigned i will wrap back when reach zero
        for (unsigned i = end; i >= start && i <= end; --i) {
            res._value[i - start] = _value[i];
        }
        return res;
    }
    UInt UInt::operator|(const UInt& other) const {
        unsigned maxw = (_width > other._width ? _width : other._width);
        unsigned minw = (_width < other._width ? _width : other._width);
        const UInt *max_one = (_width > other._width ? this : &other);
        UInt res{maxw, 0ULL};
        for (unsigned i = 0; i < minw; ++i) {
            res._value[i] = _value[i] | other._value[i];
        }
        for (unsigned i = minw; i < maxw; ++i) {
            res._value[i] = max_one->_value[i];
        }
        return res;
    }
    UInt UInt::operator&(const UInt& other) const {
    unsigned maxw = (_width > other._width ? _width : other._width);
    unsigned minw = (_width < other._width ? _width : other._width);
    const UInt *max_one = (_width > other._width ? this : &other);
    UInt res{maxw, 0ULL};
    for (unsigned i = 0; i < minw; ++i) {
        res._value[i] = _value[i] & other._value[i];
    }
    for (unsigned i = minw; i < maxw; ++i) {
        res._value[i] = !max_one->_value[i];
    }
    return res;
    }

    UInt UInt::operator<<(unsigned shift) const {
        UInt res(_width + shift, 0ULL);
        for (unsigned i = 0; i < _width; ++i) {
            res._value[i + shift] = _value[i];
        }
        return res;
    }
    UInt UInt::operator>>(unsigned shift) const {
        UInt res(_width, 0ULL);
        if (shift >= _width) {
            return res;
        }
        for (unsigned i = 0; i < _width - shift; ++i) {
            res._value[i] = _value[i + shift];
        }
        return res;
    }

    bool UInt::operator>(const UInt& other) const {
        return toBigUInt() > other.toBigUInt();
    }
    bool UInt::operator<=(const UInt& other) const {
        return operator>(other) == false;
    }
    bool UInt::operator<(const UInt& other) const {
        return toBigUInt() < other.toBigUInt();
    }
    bool UInt::operator>=(const UInt& other) const {
        return operator<(other) == false;
    }
    UInt UInt::operator+(const UInt& other) const {
        unsigned maxw = size() > other.size() ? size() : other.size();
        BigUInt res = this->toBigUInt() + other.toBigUInt();
        return UInt(maxw, res);
    }
    UInt UInt::operator-(const UInt& other) const {
        unsigned maxw = size() > other.size() ? size() : other.size();
        BigUInt res = this->toBigUInt() - other.toBigUInt();
        return UInt(maxw, res);
    }
    UInt UInt::operator*(const UInt& other) const {
        unsigned s = size() + other.size();
        return UInt(s, this->toBigUInt() * other.toBigUInt());
    }
    UInt UInt::operator/(const UInt& other) const {
        unsigned s = size() + other.size();
        return UInt(s, this->toBigUInt() / other.toBigUInt());
    }
    UInt UInt::operator%(const UInt& other) const {
        unsigned s = size() + other.size();
        return UInt(s, this->toBigUInt() % other.toBigUInt());
    }
    UInt UInt::operator=(const UInt& other) {
        Bits::operator=(other);
        return *this;
    }
    UInt UInt::operator=(UInt&& other) {
        Bits::operator=(other);
        return *this;
    }
    UInt::operator bool() const {
        return Bits::operator bool();
    }

    UInt& UInt::append(const UInt& other) {
        _value.insert(_value.begin(), other._value.begin(), other._value.end());
        _width += other._width;
        return *this;
    }

    // =========================== SInt ==========================
    //SInt::
    //SInt(unsigned width): Bits(width) {}
    //SInt::
    //SInt(unsigned width, const long val): Bits(width, val) {}
    SInt::
    SInt(unsigned width, const long long val): Bits(width, val) {}

    SInt::
    operator long long() const {
        return static_cast<long long>(this->toBigUInt());
    }

    // =========================== Bool ==========================
    Bool::
    Bool(const bool val): UInt(1, static_cast<unsigned long>(val)) {}
    Bool::
    Bool(const Bool& other): UInt(other) {}
    Bool::
    Bool(Bool&& other): UInt(std::move(other)) {
        std::cout << "call Bool rreferenct constructor" << std::endl;
    }
    Bool::
    Bool(const Bits &other): UInt(1, 0) {
        if (other.size() != 1) {
            std::cerr << "Error: Bool constructor only accept 1 bit" << std::endl;
            exit(-1);
        }
        _value[0] = other.getValue()[0];
    }
    //bool Bool::operator ()() const {
    //    return operator bool();
    //}
    Bool::
    operator bool() const {
        return static_cast<bool>(UInt::toBigUInt());
    }

    Bool Bool::operator&&(const Bool& other) const {
        return Bool(this->toBigUInt() && other.toBigUInt());
    }
    Bool Bool::operator||(const Bool& other) const {
        return Bool(this->toBigUInt() || other.toBigUInt());
    }
    Bool Bool::operator=(const bool val) {
        update(1, static_cast<BigUInt>(val));
        return *this;
    }
    Bool Bool::operator=(const Bool& other) {
        if (this != &other) {
            _width = other._width;
            _value.resize(_width);
            for (unsigned i = 0; i < _width; ++i) {
                _value[i] = other._value[i];
            }
        }
        return *this;
    }
    Bool Bool::operator!() const {
        return Bool(operator bool() == false);
    }

}