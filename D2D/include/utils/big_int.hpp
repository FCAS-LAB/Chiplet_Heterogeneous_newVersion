#ifndef __BIG_INT_HPP__
#define __BIG_INT_HPP__


#include <boost/multiprecision/cpp_int.hpp>

namespace CCPS {

    // Define BigInt as long long.
    using BigInt = boost::multiprecision::int1024_t;
    using BigUInt = boost::multiprecision::uint1024_t;
};

#endif // __BIG_INT_HPP__