#include "utils/counter.hpp"
#include "utils/helper_functions.hpp"

namespace CCPS {

    Counter::Counter(const Wire<Bool> &inc, unsigned n):
        _n(n),
        _inc(inc),
        _width(log2Ceil(n)),
        _count(createReg<UInt>(UInt(_width, 0))) {
            //_count->setDebug("Counter Reg");
        }

    Counter::Counter(): Counter(Wire<Bool>(), 0) {}


    void Counter::init(const Wire<Bool> &inc, unsigned n) {
        _n = n;
        _inc.update(inc);
        _width = log2Ceil(n);
        _count->init(UInt(_width, 0));
    }

    const UInt& Counter::getCount() const {
        return *_count;
    }

    Bool Counter::isDone() {
        const UInt& count = _count->read();
        bool is_done = count.toBigUInt() == _n-1;
        return Bool(is_done);
    }

    void Counter::calcNextState() {
        if (_inc()) {
            if (isDone()) {
                *_count = UInt(_width, 0);
            } else {
                *_count = _count->read() + UInt(_width, 1);
            }
        }

        RegModule::calcNextState();

        //std::cout << "in Counter, _path_name " << getPathName() << " count " << getCount().toBigUInt() << std::endl;

        //std::cout << "in Counter, n " << _n
        //    << ", count " << getCount()()
        //    << " isDone " << isDone()()
        //    << " inc " << _inc()()
        //    << std::endl;
    }
} // namespace CCPS