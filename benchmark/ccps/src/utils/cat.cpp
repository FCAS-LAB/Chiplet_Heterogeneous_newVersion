//#include "utils/cat.hpp"
//
//namespace CCPS {
//    Wire<UInt> cat(const std::initializer_list<Wire<Bits>> &bits){
//        Wire<UInt> pfunc{
//            [&bits]() -> UInt {
//                UInt res{0};
//                for (auto& elm : bits) {
//                    res.append(elm());
//                }
//                return res;
//            }
//        };
//        return pfunc;
//    }
//
//    UInt&& cat(const std::initializer_list<Bits> &&bits){
//        UInt res{0};
//        for (auto& elm : bits) {
//            res.append(elm);
//        }
//        return std::move(res);
//    }
//} // namespace CCPS