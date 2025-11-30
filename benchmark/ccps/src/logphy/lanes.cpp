#include "logphy/lanes.hpp"

namespace CCPS {
    // ======================================= Lanes ===================================
    Lanes::Lanes(const AfeParams &afe_params, const AsyncQueueParams &queue_params):
        _ratio_bytes(afe_params.mb_serializer_ratio / 8)
    {
        int mb_lanes = afe_params.mb_lanes;
        int mb_serializer_ratio = afe_params.mb_serializer_ratio;
        assert (mb_serializer_ratio > 8 && mb_serializer_ratio % 8 == 0);

        // Instantiate
        _tx_mb_fifo.resize(mb_lanes);
        for (int i = 0; i < mb_lanes; i++) {
            std::string name = "tx_mb_fifo_" + i;
            _tx_mb_fifo[i] = createSubmodule<AsyncQueue<UInt>>(name, queue_params.depth);
        }

        _rx_mb_fifo.resize(mb_lanes);
        for (int i = 0; i < mb_lanes; i++) {
            std::string name = "rx_mb_fifo_" + i;
            _rx_mb_fifo[i] = createSubmodule<AsyncQueue<UInt>>(name, queue_params.depth);
        }

        io.mainband_io.tx_data.resize(mb_lanes);
        io.mainband_io.rx_data.resize(mb_lanes);

        // connect
        for (int i = 0; i < mb_lanes; i++) {
            io.mainband_io.rx_data[i].flip();
            _rx_mb_fifo[i]->io.enq.connect(io.mainband_io.rx_data[i]);
            _rx_mb_fifo[i]->io.deq_reset = getReset();
            _rx_mb_fifo[i]->io.enq_reset = io.mainband_io.fifo_params.reset;
        }

        for (int i = 0; i < mb_lanes; i++) {
            io.mainband_io.tx_data[i].connect(_tx_mb_fifo[i]->io.deq);
            _tx_mb_fifo[i]->io.enq_reset = getReset();
            _tx_mb_fifo[i]->io.deq_reset = io.mainband_io.fifo_params.reset;

            _tx_mb_fifo[i]->io.enq.assignValid(
                [this]() -> Bool {
                    return io.mainband_lane_io.tx_data.isValid();
                }
            );
        }
        io.mainband_lane_io.rx_data.assignValid(
            [this]() -> Bool {
                return _rx_mb_fifo[0]->io.deq.isValid();
            }
        );

        _tx_data_vec.resize(mb_lanes);
        for (auto &data : _tx_data_vec) {
            data.resize(_ratio_bytes);
        }

        _rx_data_vec.resize(_ratio_bytes);
        for (auto &data : _rx_data_vec) {
            data.resize(mb_lanes);
        }

        for (int i = 0; i < mb_lanes; i++) {
            for (int j = 0; j < _ratio_bytes; j++) {
                _tx_data_vec[mb_lanes - 1 - i][j] = [this, i, j, mb_lanes]() -> UInt {
                    return
                        io.mainband_lane_io.tx_data.bits()(mb_lanes * 8 * j + (i * 8) + 7, mb_lanes * 8 * j + (i * 8));
                };
                _rx_data_vec[j][mb_lanes - 1 - i] = [this, i, j]() -> UInt {
                    return
                        _rx_mb_fifo[i]->io.deq.bits()((j + 1) * 8 - 1, j * 8);
                };
            }
            _tx_mb_fifo[i]->io.enq.assignBits(
                [this, i]() -> UInt {
                    UInt res;
                    auto &tx_data = _tx_data_vec[i];
                    for (int j = 0; j < tx_data.size(); j++) {
                        auto &d = tx_data[tx_data.size() - 1 - j];
                        res.append(d());
                    }
                    return res;
                }
            );
        }
        io.mainband_lane_io.rx_data.assignBits(
            [this]() -> UInt {
                UInt res;
                for (size_t i = 0; i < _rx_data_vec.size(); i++) {
                    auto &rx_data = _rx_data_vec[_rx_data_vec.size() - 1 - i];
                    for (size_t j = 0; j < rx_data.size(); j++) {
                        auto &data = rx_data[rx_data.size() - 1 - j];
                        res.append(data());
                    }
                }
                return res;
            }
        );
        for (auto &fifo : _rx_mb_fifo) {
            fifo->io.deq.assignReady(true);
        }
        io.mainband_lane_io.tx_data.assignReady(
            [this]() -> Bool {
                return _tx_mb_fifo[0]->io.enq.isReady();
            }
        );
    }

    bool Lanes::propagateClock() {
        if (io.mainband_io.fifo_params.clk == nullptr) {
            return false;
        }

        for (auto &fifo : _rx_mb_fifo) {
            fifo->io.deq_clock = getClock();
            fifo->io.enq_clock = io.mainband_io.fifo_params.clk;
        }
        for (auto &fifo : _tx_mb_fifo) {
            fifo->io.enq_clock = getClock();
            fifo->io.deq_clock = io.mainband_io.fifo_params.clk;
        }

        return Module::propagateClock();
    }

    void Lanes::calcNextState() {
        // Check reset connection
        isReset();
    }

    // ======================================= SimLanes ===================================
    SimLanes::SimLanes(const AfeParams &afe_params, const AsyncQueueParams &queue_params):
        _ratio_bytes(afe_params.mb_serializer_ratio / 8)
    {
        int mb_lanes = afe_params.mb_lanes;
        int mb_serializer_ratio = afe_params.mb_serializer_ratio;
        assert (mb_serializer_ratio > 8 && mb_serializer_ratio % 8 == 0);

        // Instantiate
        _tx_mb_fifo.resize(mb_lanes);
        for (int i = 0; i < mb_lanes; i++) {
            std::string name = "tx_mb_fifo_" + i;
            _tx_mb_fifo[i] = createSubmodule<Queue<UInt>>(name, queue_params.depth);
        }

        _rx_mb_fifo.resize(mb_lanes);
        for (int i = 0; i < mb_lanes; i++) {
            std::string name = "rx_mb_fifo_" + i;
            _rx_mb_fifo[i] = createSubmodule<Queue<UInt>>(name, queue_params.depth);
        }

        io.mainband_io.tx_data.resize(mb_lanes);
        io.mainband_io.rx_data.resize(mb_lanes);

        // connect
        for (int i = 0; i < mb_lanes; i++) {
            _rx_mb_fifo[i]->io.enq.connect(io.mainband_io.rx_data[i]);
            io.mainband_io.tx_data[i].connect(_tx_mb_fifo[i]->io.deq);
        }

        for (int i = 0; i < mb_lanes; i++) {
            _tx_mb_fifo[i]->io.enq.assignValid(
                [this]() -> Bool {
                    return io.mainband_lane_io.tx_data.isValid();
                }
            );
        }
        io.mainband_lane_io.rx_data.assignValid(
            [this]() -> Bool {
                return _rx_mb_fifo[0]->io.deq.isValid();
            }
        );

        _tx_data_vec.resize(mb_lanes);
        for (auto &data : _tx_data_vec) {
            data.resize(_ratio_bytes);
        }

        _rx_data_vec.resize(_ratio_bytes);
        for (auto &data : _rx_data_vec) {
            data.resize(mb_lanes);
        }

        for (int i = 0; i < mb_lanes; i++) {
            for (int j = 0; j < _ratio_bytes; j++) {
                _tx_data_vec[mb_lanes - 1 - i][j] = [this, i, j, mb_lanes]() -> UInt {
                    return
                        io.mainband_lane_io.tx_data.bits()(mb_lanes * 8 * j + (i * 8) + 7, mb_lanes * 8 * j + (i * 8));
                };
                _rx_data_vec[j][mb_lanes - 1 - i] = [this, i, j]() -> UInt {
                    return
                        _rx_mb_fifo[i]->io.deq.bits()((j + 1) * 8 - 1, j * 8);
                };
            }
            _tx_mb_fifo[i]->io.enq.assignBits(
                [this, i]() -> UInt {
                    UInt res;
                    auto &tx_data = _tx_data_vec[i];
                    for (int j = 0; j < tx_data.size(); j++) {
                        auto &d = tx_data[tx_data.size() - 1 - j];
                        res.append(d());
                    }
                    return res;
                }
            );
        }
        io.mainband_lane_io.rx_data.assignBits(
            [this]() -> UInt {
                UInt res;
                for (size_t i = 0; i < _rx_data_vec.size(); i++) {
                    auto &rx_data = _rx_data_vec[_rx_data_vec.size() - 1 - i];
                    for (size_t j = 0; j < rx_data.size(); j++) {
                        auto &data = rx_data[rx_data.size() - 1 - j];
                        res.append(data());
                    }
                }
                return res;
            }
        );
        for (auto &fifo : _rx_mb_fifo) {
            fifo->io.deq.assignReady(true);
        }
        io.mainband_lane_io.tx_data.assignReady(
            [this]() -> Bool {
                return _tx_mb_fifo[0]->io.enq.isReady();
            }
        );
    }

    void SimLanes::calcNextState() {
        // Check reset connection
        isReset();

        //std::cout << getPathName() << ": io.mainband_lane_io.tx_data.valid " << static_cast<bool>(io.mainband_lane_io.tx_data.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.mainband_lane_io.tx_data.ready " << static_cast<bool>(io.mainband_lane_io.tx_data.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.mainband_lane_io.tx_data.bits 0x" << std::hex << io.mainband_lane_io.tx_data.bits().toBigUInt() << std::endl;
        //for (size_t i = 0; i < io.mainband_io.tx_data.size(); i++) {
        //    const auto &tx_data = io.mainband_io.tx_data[i];
        //    std::cout << getPathName() << ": io.mainband_io.tx_data[" << i << "].valid " << static_cast<bool>(tx_data.isValid()) << std::endl;
        //    std::cout << getPathName() << ": io.mainband_io.tx_data[" << i << "].ready " << static_cast<bool>(tx_data.isReady()) << std::endl;
        //    std::cout << getPathName() << ": io.mainband_io.tx_data[" << i << "].bits 0x" << std::hex << tx_data.bits().toBigUInt() << std::endl;
        //}
        //for (size_t i = 0; i < io.mainband_io.rx_data.size(); i++) {
        //    const auto &rx_data = io.mainband_io.rx_data[i];
        //    std::cout << getPathName() << ": io.mainband_io.rx_data[" << i << "].valid " << static_cast<bool>(rx_data.isValid()) << std::endl;
        //    std::cout << getPathName() << ": io.mainband_io.rx_data[" << i << "].ready " << static_cast<bool>(rx_data.isReady()) << std::endl;
        //    std::cout << getPathName() << ": io.mainband_io.rx_data[" << i << "].bits 0x" << std::hex << rx_data.bits().toBigUInt() << std::endl;
        //}
        //std::cout << getPathName() << ": io.mainband_lane_io.rx_data.valid " << static_cast<bool>(io.mainband_lane_io.rx_data.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.mainband_lane_io.rx_data.bits 0x" << std::hex << io.mainband_lane_io.rx_data.bits().toBigUInt() << std::endl;

    }
} // namespace CCPS