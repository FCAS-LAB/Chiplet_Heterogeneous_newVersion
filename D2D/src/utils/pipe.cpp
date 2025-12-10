#include "utils/pipe.hpp"
#include "utils/helper_functions.hpp"

namespace CCPS {

    Pipe::Pipe(int latency, int width): _latency(latency), _width(width) {
        // Instantiate
        assert (latency > 0);
        assert (width > 0);
        _v_bits.resize(latency);
        _v_valid.resize(latency);
        for (int i = 0; i < latency; i++) {
            _v_bits[i] = createReg<UInt>(UInt(width, 0));
            _v_valid[i] = createReg<Bool>(Bool(false));
        }
        _counter = createReg<UInt>(UInt(static_cast<int>(log2Ceil(latency)), 0));
        _temp_valids.resize(latency);
        _temp_readys.resize(latency);

        // Connect
        io.enq.ready = [&]() {
            bool val = static_cast<int>(_counter->read().toBigUInt()) == _latency &&
                       static_cast<bool>(io.deq.ready());
            return Bool(val);
        };
        io.deq.bits = _v_bits[_latency - 1];
        io.deq.valid = _v_valid[_latency - 1];
    }

    void Pipe::calcNextState() {
        _temp_valids[0] = io.enq.valid();
        for (int i = 1; i < _latency; i++) {
            _temp_valids[i] = _v_valid[i - 1]->read();
        }

        _temp_readys[_latency - 1] = io.deq.ready();
        for (int i = _latency - 2; i >= 0; i--) {
            // the next ceil is empty or ready to pop
            _temp_readys[i] = _v_valid[i + 1]->read() == Bool(false) || _temp_readys[i + 1];
        }

        for (int i = 0; i < _latency; i++) {
            *_v_valid[i] = Bool(_temp_valids[i]);
            if (_temp_valids[i] && _temp_readys[i]) {
                *_v_bits[i] = io.enq.bits();
            }
        }
    }

} // namespace CCPS