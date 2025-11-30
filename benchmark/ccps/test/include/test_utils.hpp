#ifndef __TEST_UTILS_HPP__
#define __TEST_UTILS_HPP__

#define EXPECT_EQ_BOOL(x, y) EXPECT_EQ(static_cast<bool>(x), static_cast<bool>(y))
#define EXPECT_EQ_BigUInt(x, y) EXPECT_EQ(x.toBigUInt(), BigUInt(y))

#endif // __TEST_UTILS_HPP__