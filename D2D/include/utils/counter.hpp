#ifndef __COUNTER_HPP__
#define __COUNTER_HPP__

#include "utils/module.hpp"
#include "utils/wire.hpp"
#include "utils/reg.hpp"

namespace CCPS {
    class Counter: public RegModule {
    public:
        Counter(const Wire<Bool> &inc, unsigned n);
        Counter();

        void init(const Wire<Bool> &inc, unsigned n);
        const UInt& getCount() const;
        Bool isDone();

        void calcNextState()  override;
    private:
        unsigned _n;
        Wire<Bool> _inc;
        unsigned _width;
        RegPtr<UInt> _count;
    };
} // namespace CCPS

#endif // __COUNTER_HPP__