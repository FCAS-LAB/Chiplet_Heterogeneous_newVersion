#ifndef __ASYNC_QUEUE_HPP__
#define __ASYNC_QUEUE_HPP__

#include "utils/module.hpp"
#include "utils/helper_functions.hpp"
#include "utils/counter.hpp"
#include "utils/decoupled.hpp"

namespace CCPS {

    template <typename T>
    class EnqModule: public RegModule {
    public:
        struct {
            Decoupled<T> enq{true};
            Wire<UInt> front;   // add one more bits to indicate empty and full
            Wire<UInt> rear;    // add one more bits to indicate empty and full
        } io;
        EnqModule(std::vector<T> &data, int size): _size_bits(log2Ceil(size)), _data(data) {
            _rear_inc = [this] () {
                return io.enq.fire();
            };
            _rear = createSubmodule<Counter>("rear_counter", _rear_inc, 1 << (_size_bits + 1));

            io.rear = [this] () -> UInt {
                return _rear->getCount();
            };
            io.enq.assignReady(
                [this]() -> Bool {
                    auto rear = _rear->getCount()(_size_bits - 1, 0);
                    auto front = io.front()(_size_bits - 1, 0);
                    bool full = (rear == front
                              && _rear->getCount()(_size_bits) != io.front()(_size_bits));
                    return Bool(!full);
                }
            );
        }

        void calcNextState() override {
            if (io.enq.fire()) {
                _data[static_cast<unsigned>(_rear->getCount()(_size_bits - 1, 0).toBigUInt())] = io.enq.bits();
            }

            if (isReset()) {
                _rear->init(_rear_inc, (1<<_size_bits));
            }

            //std::cout << "============== " << getPathName() << " =================" << std::endl;
            //std::cout << getPathName() << ": io.enq.valid " << static_cast<bool>(io.enq.isValid()) << std::endl;
            //std::cout << getPathName() << ": io.enq.ready " << static_cast<bool>(io.enq.isReady()) << std::endl;
            //std::cout << getPathName() << ": io.enq.bits " << io.enq.bits().toBigUInt() << std::endl;
            //std::cout << getPathName() << ": io.rear " << io.rear().toBigUInt() << std::endl;
            //std::cout << getPathName() << ": io.front " << io.front().toBigUInt() << std::endl;

        }
    private:
        int _size_bits;
        std::vector<T> &_data;
        Wire<Bool> _rear_inc;
        ModulePtr<Counter> _rear;
    };

    template <typename T>
    class DeqModule: public WireModule {
    public:
        struct {
            Decoupled<T> deq;
            Wire<UInt> front;   // add one more bits to indicate empty and full
            Wire<UInt> rear;    // add one more bits to indicate empty and full
        } io;

        DeqModule(std::vector<T> &data, int size): _size_bits(log2Ceil(size)), _data(data) {
            _front_inc = [this]() -> Bool {
                return io.deq.fire();
            };
            _front = createSubmodule<Counter>("front_counter", _front_inc, 1 << (_size_bits + 1));

            io.front = [this] () -> UInt {
                return _front->getCount();
            };

            io.deq.assignValid(
                [this]() -> Bool {
                    bool empty = _front->getCount() == io.rear();
                    return Bool(!empty);
                }
            );
            io.deq.assignBits(
                [this]() -> T {
                    return _data[static_cast<unsigned>(_front->getCount()(_size_bits - 1, 0).toBigUInt() % (1 << _size_bits))];
                }
            );
        }

        void calcNextState() override {
            if (isReset()) {
                _front->init(_front_inc, 1<<_size_bits);
            }

            //std::cout << "============== " << getPathName() << " =================" << std::endl;
            //std::cout << getPathName() << ": io.deq.valid " << static_cast<bool>(io.deq.isValid()) << std::endl;
            //std::cout << getPathName() << ": io.deq.ready " << static_cast<bool>(io.deq.isReady()) << std::endl;
            //std::cout << getPathName() << ": io.deq.bits " << io.deq.bits().toBigUInt() << std::endl;
            //std::cout << getPathName() << ": io.rear " << io.rear().toBigUInt() << std::endl;
            //std::cout << getPathName() << ": io.front " << io.front().toBigUInt() << std::endl;
        }

    private:
        int _size_bits;
        std::vector<T> &_data;
        Wire<Bool> _front_inc;
        ModulePtr<Counter> _front;
    };

    template <typename T>
    class AsyncQueue: public WireModule {
    public:
        struct {
            Decoupled<T> enq{true};
            ClockPtr enq_clock;
            Wire<Bool> enq_reset;
            Decoupled<T> deq;
            ClockPtr deq_clock;
            Wire<Bool> deq_reset;
        } io;

        AsyncQueue(int size) {
            // size must be power of 2.
            assert ((size & (size-1)) == 0);
            // Instantiate
            _enq = createSubmodule<EnqModule<T>>("enq_module", _data, size);
            _deq = createSubmodule<DeqModule<T>>("deq_module", _data, size);
            _data.resize(size);

            // connect
            _enq->io.enq.connect(io.enq);
            _enq->io.front = _deq->io.front;
            _deq->io.rear = _enq->io.rear;
            io.deq.connect(_deq->io.deq);

            _enq->setReset(io.enq_reset);
            _deq->setReset(io.deq_reset);
        }

        bool propagateClock() {
            if (io.enq_clock == nullptr || io.deq_clock == nullptr) {
                return false;
            }
            if (_enq->getClock() == nullptr) {
                addModuleToClock(io.enq_clock, _enq);
            }
            if (_deq->getClock() == nullptr) {
                addModuleToClock(io.deq_clock, _deq);
            }
            return Module::propagateClock();
        }

    private:
        ModulePtr<EnqModule<T>> _enq;
        ModulePtr<DeqModule<T>> _deq;
        std::vector<T> _data;
    };
} // namespace CCPS

#endif // __ASYNC_QUEUE_HPP__