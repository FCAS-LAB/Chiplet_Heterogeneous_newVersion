#ifndef __HELPER_FUNCTIONS_HPP__
#define __HELPER_FUNCTIONS_HPP__

#include "utils/big_int.hpp"
namespace CCPS {

    // get the number of bits needed to represent a number in binary

    template<typename T>
    T log2Ceil(T n) {
        assert (n != (T(0)-1));
        int log = 0;
        while ((T(1) << log) < n) {
            log++;
        }
        return static_cast<T>(log);
    }

    BigUInt getMask(int width);

    BigUInt getMaskedData(BigUInt data, int width);

} // namespace CCPS

#endif // __HELPER_FUNCTIONS_HPP__