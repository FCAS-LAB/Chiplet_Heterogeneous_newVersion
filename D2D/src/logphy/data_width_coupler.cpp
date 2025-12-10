#include "logphy/data_width_coupler.hpp"
#include "utils/helper_functions.hpp"

namespace CCPS {
    DataWidthCoupler::DataWidthCoupler(const DataWidthCouplerParams &params): _params(params) {
        _current_state = createReg<UInt>(UInt(1, State::IDLE));

        if (params.in_width > params.out_width) {
            _ratio = params.in_width / params.out_width;
            assert(params.in_width % params.out_width == 0);

            // Intantiate
            _chunk_counter = createReg<UInt>(UInt(log2Ceil(_ratio), 0));
            _in_data.resize(1);
            for (auto &data : _in_data) {
                data = createReg<UInt>(UInt(params.in_width, 0));
            }
            io.in.assignReady(
                [this]() -> Bool {
                    bool res = false;
                    if (_current_state->read().toBigUInt() == State::IDLE) {
                        res = true;
                    }
                    return Bool(res);
                }
            );
            io.out.assignValid(
                [this]() -> Bool {
                    bool res = false;
                    if (_current_state->read().toBigUInt() == State::CHUNK_OR_COLLECT) {
                        res = true;
                    }
                    return Bool(res);
                }
            );
            io.out.assignBits(
                [this]() -> UInt {
                    int i = static_cast<int>(_chunk_counter->read().toBigUInt());
                    int start = ((_ratio - 1) - i) * _params.out_width;
                    return _in_data[0]->read()(start + _params.out_width - 1, start);
                }
            );
        } else {
            assert(params.out_width % params.in_width == 0);
            _ratio = params.out_width / params.in_width;
            // Intantiate
            _in_slice_counter = createReg<UInt>(UInt(log2Ceil(_ratio), 0));
            _in_data.resize(_ratio);
            for (auto &data : _in_data) {
                data = createReg<UInt>(UInt(params.in_width, 0));
            }
            io.in.assignReady(
                [this]() -> Bool {
                    bool res = false;
                    if (_current_state->read().toBigUInt() == State::IDLE) {
                        res = true;
                    }
                    return Bool(res);
                }
            );
            io.out.assignValid(
                [this]() -> Bool {
                    bool res = false;
                    if (_current_state->read().toBigUInt() == State::CHUNK_OR_COLLECT) {
                        res = true;
                    }
                    return Bool(res);
                }
            );
            io.out.assignBits(
                [this]() -> UInt {
                    UInt res;
                    for (int i = 0; i < _ratio; i++) {
                        res.append(_in_data[_ratio - i - 1]->read());
                    }
                    return res;
                }
            );
        }
    }

    void DataWidthCoupler::calcNextState() {
        State current_state = static_cast<State>(_current_state->read().toBigUInt());

        if (_params.in_width > _params.out_width) {
            if (current_state == State::IDLE) {
                if (io.in.fire()) {
                    *_in_data[0] = io.in.bits();
                    *_chunk_counter = UInt(log2Ceil(_ratio), 0);
                    *_current_state = UInt(1, State::CHUNK_OR_COLLECT);
                }
            } else if (current_state == State::CHUNK_OR_COLLECT) {
                if (io.out.fire()) {
                    int chunk_counter = static_cast<int>(_chunk_counter->read().toBigUInt());
                    *_chunk_counter = UInt(log2Ceil(_ratio), chunk_counter + 1);
                    if (chunk_counter == (_ratio - 1)) {
                        *_current_state = UInt(1, State::IDLE);
                    }
                }
            }
        } else {
            if (current_state == State::IDLE) {
                int in_slice_counter = static_cast<int>(_in_slice_counter->read().toBigUInt());
                if (io.in.fire()) {
                    *_in_data[_ratio - 1 - in_slice_counter] = io.in.bits();
                    *_in_slice_counter = UInt(log2Ceil(_ratio), in_slice_counter + 1);
                }
                if (in_slice_counter == (_ratio - 1)) {
                    *_in_slice_counter = UInt(log2Ceil(_ratio), 0);
                    *_current_state = UInt(1, State::CHUNK_OR_COLLECT);
                }
            } else if (current_state == State::CHUNK_OR_COLLECT) {
                if (io.out.fire()) {
                    *_current_state = UInt(1, State::IDLE);
                }
            }
        }

        //std::cout << getPathName() << ": io.in.valid " << static_cast<bool>(io.in.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.in.ready " << static_cast<bool>(io.in.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.in.bits " << std::hex << io.in.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.out.valid " << static_cast<bool>(io.out.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.out.ready " << static_cast<bool>(io.out.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.out.bits " << std::hex << io.out.bits().toBigUInt() << std::endl;
    }
} // namespace CCPS