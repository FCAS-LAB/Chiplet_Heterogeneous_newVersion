#ifndef __QUEUE_HPP__
#define __QUEUE_HPP__

#include "utils/module.hpp"
#include "utils/helper_functions.hpp"
#include "utils/counter.hpp"
#include "utils/decoupled.hpp"

namespace CCPS {

    template <typename T>
    class Queue: public RegModule {
    public:
        struct {
            Decoupled<T> enq{true};
            Decoupled<T> deq;
            Wire<UInt> count;
        } io;

        Queue(size_t size) :
            _size(size),
            _count{
                createReg<UInt>(UInt(log2Ceil(size+1), 0))
            },
            _data{size},
            _io_enq_ready{
                [this]() -> Bool {
                    return Bool(_count->read().toBigUInt() < _size);
                }
            },
            _io_deq_valid{
                [this]() -> Bool {
                    return Bool(_count->read().toBigUInt() > 0);
                }
            },
            _io_deq_bits{
                [this]() -> T {
                    return _data[static_cast<unsigned>(_front->getCount().toBigUInt())];
                }
            },
            _io_count{
                [this]() -> UInt {
                    return _count->read();
                }
            }

        {
            _front_inc = [this]() -> Bool {
                //std::cout << "here in debug1" << std::endl;
                return io.deq.fire();
            };
            _front = createSubmodule<Counter>("front_counter", _front_inc, size);

            _rear_inc = [this]() -> Bool {
                //std::cout << "here in debug2" << std::endl;
                return io.enq.fire();
            };
            _rear = createSubmodule<Counter>("rear_counter", _rear_inc, size);

            io.enq.assignReady(_io_enq_ready);
            io.deq.assignValid(_io_deq_valid);
            io.deq.assignBits(_io_deq_bits);
            io.count = _io_count;
        }

        //void init() {
        //    *_front = UInt(log2Ceil(_size), 0);
        //    *_rear = UInt(log2Ceil(_size), 0);
        //    *_count = UInt(log2Ceil(_size), 0);
        //}

        void calcNextState() override {
            if (io.enq.fire() && !io.deq.fire()) {
                *_count = UInt(_count->read().size(), _count->read().toBigUInt() + 1);
            } else if (!io.enq.fire() && io.deq.fire()) {
                *_count = UInt(_count->read().size(), _count->read().toBigUInt() - 1);
            }

            if (io.enq.fire()) {
                _data[static_cast<unsigned>(_rear->getCount().toBigUInt())] = io.enq.bits();
            }

            //std::cout << "in Queue, _path_name " << getPathName() << std::endl;
        }

    private:
        int _size;
        Wire<Bool> _front_inc;
        ModulePtr<Counter> _front;
        Wire<Bool> _rear_inc;
        ModulePtr<Counter> _rear;
        RegPtr<UInt> _count;
        std::vector<T> _data;

        // ========== helper signals ===========
        Wire<Bool>::TPFUNC _io_enq_ready;
        Wire<Bool>::TPFUNC _io_deq_valid;
        typename Wire<T>::TPFUNC _io_deq_bits;
        Wire<UInt>::TPFUNC _io_count;
    };
} // namespace CCPS

#endif // __QUEUE_HPP__