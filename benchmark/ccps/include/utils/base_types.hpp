#ifndef __BASE_TYPES_HPP__
#define __BASE_TYPES_HPP__

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <boost/multiprecision/cpp_int.hpp>
#include "big_int.hpp"
#include "utils/helper_functions.hpp"

using namespace boost::multiprecision;

namespace CCPS {
    //using BigUInt = boost::multiprecision::uint128_t;
    //using BigInt = boost::multiprecision::int128_t;

    class UInt;

    class Data {
        public:
            virtual ~Data() = default;
    };

    class Element: public Data {
    public:
        virtual ~Element() = default;
    };

    /*
        index 0 is the LSB bit.
    */
    class Bits: public Element {
    public:
        Bits();
        //Bits(unsigned width);
        Bits(unsigned width, const long val);
        Bits(unsigned width, const long long val);
        Bits(unsigned width, const unsigned long val);
        Bits(unsigned width, const unsigned long long val);
        Bits(unsigned width, const BigUInt val);
        Bits(const Bits& other);
        Bits(Bits&& other);
        void update(unsigned width, const long long val);
        void update(unsigned width, const unsigned long long val);
        void update(unsigned width, const BigUInt val);

        // get the value of the bits.
        BigUInt toBigUInt() const;
        BigUInt toBigUInt(unsigned index) const;
        BigUInt toBigUInt(unsigned end, unsigned start) const; // start inclusive, end inclusive
        bool operator==(const Bits& other) const;
        bool operator!=(const Bits& other) const;

        bool operator==(const std::string& s) const;
        bool operator!=(const std::string& s) const;

        Bits operator|(const Bits& other) const;
        Bits operator&(const Bits& other) const;
        Bits operator^(const Bits& other) const;
        Bits operator~() const;
        Bits operator<<(unsigned shift) const;
        Bits operator>>(unsigned shift) const;
        Bits operator+(const Bits& other) const;
        Bits operator-(const Bits& other) const;
        Bits operator*(const Bits& other) const;
        Bits operator/(const Bits& other) const;
        Bits operator%(const Bits& other) const;
        //long long operator=(const long long val);
        //unsigned long long operator=(const unsigned long long val);
        Bits operator=(const BigUInt val);
        Bits operator=(const Bits& other);
        Bits operator=(Bits&& other);
        operator bool() const;
        Bits xorR() const;

        virtual ~Bits() = default;

        unsigned size() const;
        const std::vector<unsigned char>& value() const;

        // append new bits in the LSB direction.
        Bits& append(const Bits& other);
        const std::vector<unsigned char>& getValue() const;
    protected:
        unsigned _width;
        std::vector<unsigned char> _value;
    };

    class UInt: public Bits {
    public:
        UInt();
        //UInt(unsigned width);
        //UInt(unsigned width, const unsigned long val);
        UInt(unsigned width, const BigUInt val);
        UInt(const UInt& other);
        UInt(UInt&& other);
        //explicit operator BigUInt() const;
        //operator BigUInt() const = delete;
        virtual ~UInt() = default;

        // delete to avoid being compared with int in test cases.
        UInt operator()() const = delete;
        UInt operator()(unsigned index) const;
        UInt operator()(unsigned end, unsigned start) const; // start inclusive, end inclusive

        UInt operator&(const UInt& other) const;

        UInt operator|(const UInt& other) const;
        UInt operator>>(unsigned shift) const;
        UInt operator<<(unsigned shift) const;
        bool operator>(const UInt& other) const;
        bool operator<=(const UInt& other) const;
        bool operator<(const UInt& other) const;
        bool operator>=(const UInt& other) const;
        UInt operator+(const UInt& other) const;
        UInt operator+(const BigUInt other) const = delete;
        UInt operator-(const UInt& other) const;
        UInt operator-(const BigUInt other) const = delete;
        UInt operator*(const UInt& other) const;
        UInt operator/(const UInt& other) const;
        UInt operator%(const UInt& other) const;
        //UInt operator=(const unsigned long long);
        UInt operator=(const BigUInt);
        UInt operator=(const UInt& other);
        UInt operator=(UInt&& other);
        operator bool() const;
        UInt& append(const UInt& other);
    };

    class SInt: public Bits {
    public:
        //SInt(unsigned width);
        //SInt(unsigned width, const long val);
        SInt(unsigned width, const long long val);
        explicit operator long long() const;
        virtual ~SInt() = default;
    };

    class Bool: public UInt {
    public:
        virtual ~Bool() = default;

        // support implicit conversion.
        explicit Bool(bool val);
        Bool(const Bool& other);
        Bool(Bool&& other);
        Bool(const Bits& other);

        //bool operator ()() const;
        operator bool() const;
        operator int() const = delete;
        Bool operator&&(const Bool& other) const;
        Bool operator||(const Bool& other) const;
        Bool operator=(const bool val);
        Bool operator=(const Bool& other);
        Bool operator!() const;
    };

    //template<typename T>
    //class Vec: public Element {
    //public:
    //    Vec(size_t size): _vec(size) {}

    //    T& operator[](int index) {
    //        return _vec[index];
    //    }

    //    size_t size() {
    //        return _vec.size();
    //    }

    //private:
    //    std::vector<T> _vec;

    //};


}

#endif // __BASE_TYPES_HPP__