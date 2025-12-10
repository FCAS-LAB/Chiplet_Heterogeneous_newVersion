#include "utils/helper_functions.hpp"
#include "utils/base_types.hpp"

namespace CCPS {

    BigUInt getMask(int width) {
        assert (width < sizeof(BigUInt)*8);
        return (BigUInt(1) << width) - 1;
    }

    BigUInt getMaskedData(BigUInt data, int width) {
        return data & getMask(width);
    }
} // namespace CCPS