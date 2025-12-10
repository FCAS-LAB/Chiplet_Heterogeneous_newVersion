#ifndef __CREDIT_FLOW_HPP__
#define __CREDIT_FLOW_HPP__

#include "utils/module.hpp"
#include "utils/queue.hpp"
#include "utils/decoupled.hpp"
#include "tilelink/rocket_chip/bundles.hpp"

namespace CCPS {
    // ============= DecoupledtoCreditedMsg ==============
    template<typename T>
    class DecoupledtoCreditedMsg: public RegModule {
    public:
        struct {
            struct {
                Wire<Bool> valid;               // I
                Wire<Bool> ready;               // O
                T bits;                         // I
            } in;
            struct {
                Wire<Bool> valid;               // O
                Wire<Bool> ready;               // I
                T bits;                         // O
            } out;
            struct {
                Wire<Bool> valid;               // I
                Wire<Bool> ready;               // O
                Wire<UInt> bits;                // I
            } credit;
        } io;


        DecoupledtoCreditedMsgTLBundleA(int flit_width, int buffer_sz):
            _credit_width(log2Ceil(buffer_sz))
        {
            assert (flit_width < sizeof(int) * 8);
            assert (_credit_width <= flit_width);

            // Instantiate
            _credits = createReg<UInt>(UInt(log2Ceil(_credit_width), 0));

            // Connect
            _credit_incr = [this] () -> Bool {
                return io.out.fire();
            };
            _credit_decr = [this] () -> Bool {
                return io.credit.fire();
            };

            io.out.valid = [this]() -> Bool {
                return Bool(static_cast<bool>(io.in.valid()) &&
                    _credits->read().toBigUInt() < buffer_sz);
            };

            io.out.bits = io.in.bits;
            io.in.ready = [this]() -> Bool {
                return Bool(static_cast<bool>(io.out.ready()) &&
                    _credits->read().toBigUInt() < buffer_sz);
            };
            io.credit.ready.capture(true);
        }

        void calcNextState() {
            if (_credit_incr() || _credit_decr()) {
                BigUInt credits = _credits->read().toBigUInt();
                int credit_inc = _credit_incr() ? 1 : 0;
                BigUInt credit_mux = io.credit.valid() ? io.credit.bits().toBigUInt() + 1 : 0;
                BigUInt val = credits + credit_inc - credit_mux;
                *_credits = UInt(_credit_width  , val);
            }
        }

    private:
        // =============== chisel signals ===============
        RegPtr<UInt> _credits;
        Wire<Bool> _credit_incr;
        Wire<Bool> _credit_decr;
        int _credit_width;
    };

    // ============= DecoupledtoCreditedMsg ==============
    template<typename T>
    class CreditedToDecoupledMsg: public RegModule {
    public:
        struct {
            struct {
                Wire<Bool> valid;               // I
                Wire<Bool> ready;               // O
                T bits;                // I
            } in;
            struct {
                Wire<Bool> valid;               // O
                Wire<Bool> ready;               // I
                T bits;                // O
            } out;
            struct {
                Wire<Bool> valid;               // O
                Wire<Bool> ready;               // I
                Wire<UInt> bits;                // O
            } credit;
        } io;

        CreditedToDecoupledMsg(int flit_width, int buffer_sz):
            _credit_width(log2Ceil(buffer_sz))
        {
            assert (flit_width < sizeof(int) * 8);
            assert (_credit_width <= flit_width);

            // Instaintiate
            _credits = createReg<UInt>(UInt(log2Ceil(_credit_width), 0));
            _buffer = createSubModule<Queue<T>>("buffer", buffer_sz);

            // Connect
            _credit_incr = [this] () -> Bool {
                return _buffer->io.deq.fire();
            };
            _credit_decr = [this] () -> Bool {
                return io.credit.fire();
            };

            _buffer->io.enq.assignValid(io.in.valid);
            _buffer.io.enq.assignBits(io.in.bits);
            io.in.ready.capture(true);

            io.out.valid = [this]() -> Bool {
                return _buffer->io.deq.isValid();
            };
            io.out.bits = [this]() -> T {
                return _buffer->io.deq.bits();
            };
            _buffer->io.deq.assignReady(io.out.ready);

            io.credit.valid = [this]() -> Bool {
                return _credits->read().toBigUInt() != 0;
            };

            io.credit.bits = [this]() -> UInt {
                return UInt(_credit_width, _credits->read().toBigUInt() - 1);
            };
        }

    private:
        RegPtr<UInt> _credits;
        ModulePtr<Queue<T>> _buffer;
        Wire<Bool> _credit_incr;
        Wire<Bool> _credit_decr;
        int _credit_width;
    };
}

#endif // __CREDIT_FLOW_HPP__