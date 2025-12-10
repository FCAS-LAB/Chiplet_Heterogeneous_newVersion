#include "sideband/sideband_node.hpp"
#include "sideband/sb_msg_encoding.hpp"
#include "utils/helper_functions.hpp"
#include "utils/module.hpp"
#include "utils/queue.hpp"
#include "sideband/sideband_io.hpp"

namespace CCPS {

    // =============================== SidebandEnqArbiter ==========================
    SidebandEnqArbiter::
    SidebandEnqArbiter(const SidebandParams &sb_params) {
        io.out.resize(3);
        _io_out_valid.resize(3);
        _io_out_bits.resize(3);

        _io_out_valid[0] = [this]() -> Bool {
            return Bool(io.in.isValid() && SBM().isComplete(io.in.bits()));
        };
        _io_out_valid[1] = [this]() -> Bool {
            return Bool(io.in.isValid() && SBM().isMessage(io.in.bits()));
        };
        _io_out_valid[2] = [this]() -> Bool {
            return Bool(io.in.isValid() && SBM().isRequest(io.in.bits()));
        };

        for (int i = 0; i < 3; i++) {
            _io_out_bits[i] = [this]() -> UInt {
                return io.in.bits();
            };
            io.out[i].assignBits(_io_out_bits[i]);
            io.out[i].assignValid(_io_out_valid[i]);
        }

        _io_in_ready = [this]() -> Bool {
            return Bool(true);
        };
        io.in.assignReady(_io_in_ready);
    }

    void SidebandEnqArbiter::calcNextState() {
        //std::cout << getPathName() << ": io.in.valid " << static_cast<bool>(io.in.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.in.ready " << static_cast<bool>(io.in.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.in.bits "  << io.in.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.out[0].valid " << static_cast<bool>(io.out[0].isValid()) << std::endl;
        //std::cout << getPathName() << ": io.out[0].ready " << static_cast<bool>(io.out[0].isReady()) << std::endl;
        //std::cout << getPathName() << ": io.out[0].bits "  << io.out[0].bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.out[1].valid " << static_cast<bool>(io.out[1].isValid()) << std::endl;
        //std::cout << getPathName() << ": io.out[1].ready " << static_cast<bool>(io.out[1].isReady()) << std::endl;
        //std::cout << getPathName() << ": io.out[1].bits "  << io.out[1].bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.out[2].valid " << static_cast<bool>(io.out[2].isValid()) << std::endl;
        //std::cout << getPathName() << ": io.out[2].ready " << static_cast<bool>(io.out[2].isReady()) << std::endl;
        //std::cout << getPathName() << ": io.out[2].bits "  << io.out[2].bits().toBigUInt() << std::endl;
    }

    // =============================== SidebandDeqArbiter ==========================
    SidebandDeqArbiter::
    SidebandDeqArbiter(const SidebandParams &sb_params) {
        _io_out_valid = [this]() -> Bool {
            return io.in[0].isValid() || io.in[1].isValid() || io.in[2].isValid();
        };
        io.out.assignValid(_io_out_valid);
        _io_out_bits = [this]() -> UInt {
            if (io.in[0].isValid()) {
                return io.in[0].bits();
            } else if (io.in[1].isValid()) {
                return io.in[1].bits();
            } else if (io.in[2].isValid()) {
                return io.in[2].bits();
            } else {
                return io.in[0].bits();
            }
        };
        io.out.assignBits(_io_out_bits);

        _io_in_ready.resize(3);
        _io_in_ready[0] = [this]() -> Bool {
            return io.in[0].isValid() && io.out.isReady();
        };
        _io_in_ready[1] = [this]() -> Bool {
            return !io.in[0].isValid() && io.in[1].isValid() && io.out.isReady();
        };
        _io_in_ready[2] = [this]() -> Bool {
            return !io.in[0].isValid() && !io.in[1].isValid() && io.in[2].isValid() && io.out.isReady();
        };

        io.in.resize(3);
        for (int i = 0; i < 3; i++) {
            io.in[i].flip();
            io.in[i].assignReady(_io_in_ready[i]);
        }
    }

    // =============================== SidebandPriorityQueue ==========================
    SidebandPriorityQueue::
    SidebandPriorityQueue(const SidebandParams &sb_params) {
        _p0_queue = createSubmodule<Queue<UInt>>("p0_queue", 4);
        _p1_queue = createSubmodule<Queue<UInt>>("p1_queue", sb_params.max_crd);
        _p2_queue = createSubmodule<Queue<UInt>>("p2_queue", sb_params.max_crd);

        _enq_arb = createSubmodule<SidebandEnqArbiter>("enq_arb", sb_params);
        _deq_arb = createSubmodule<SidebandDeqArbiter>("deq_arb", sb_params);

        _enq_arb->io.in.connect(io.enq);
        _p0_queue->io.enq.connect(_enq_arb->io.out[0]);
        _p1_queue->io.enq.connect(_enq_arb->io.out[1]);
        _p2_queue->io.enq.connect(_enq_arb->io.out[2]);

        _deq_arb->io.in[0].connect(_p0_queue->io.deq);
        _deq_arb->io.in[1].connect(_p1_queue->io.deq);
        _deq_arb->io.in[2].connect(_p2_queue->io.deq);
        io.deq.connect(_deq_arb->io.out);
    }

    void SidebandPriorityQueue::calcNextState() {
        //std::cout << getPathName() << ": io.enq.valid " << static_cast<bool>(io.enq.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.enq.ready " << static_cast<bool>(io.enq.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.enq.bits "  << io.enq.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _p0_queue.enq.valid " << static_cast<bool>(_p0_queue->io.enq.isValid()) << std::endl;
        //std::cout << getPathName() << ": _p0_queue.enq.ready " << static_cast<bool>(_p0_queue->io.enq.isReady()) << std::endl;
        //std::cout << getPathName() << ": _p0_queue.enq.bits "  << _p0_queue->io.enq.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _p1_queue.enq.valid " << static_cast<bool>(_p1_queue->io.enq.isValid()) << std::endl;
        //std::cout << getPathName() << ": _p1_queue.enq.ready " << static_cast<bool>(_p1_queue->io.enq.isReady()) << std::endl;
        //std::cout << getPathName() << ": _p1_queue.enq.bits "  << _p1_queue->io.enq.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _p2_queue.enq.valid " << static_cast<bool>(_p2_queue->io.enq.isValid()) << std::endl;
        //std::cout << getPathName() << ": _p2_queue.enq.ready " << static_cast<bool>(_p2_queue->io.enq.isReady()) << std::endl;
        //std::cout << getPathName() << ": _p2_queue.enq.bits "  << _p2_queue->io.enq.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _p0_queue.deq.valid " << static_cast<bool>(_p0_queue->io.deq.isValid()) << std::endl;
        //std::cout << getPathName() << ": _p0_queue.deq.ready " << static_cast<bool>(_p0_queue->io.deq.isReady()) << std::endl;
        //std::cout << getPathName() << ": _p0_queue.deq.bits "  << _p0_queue->io.deq.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _p1_queue.deq.valid " << static_cast<bool>(_p1_queue->io.deq.isValid()) << std::endl;
        //std::cout << getPathName() << ": _p1_queue.deq.ready " << static_cast<bool>(_p1_queue->io.deq.isReady()) << std::endl;
        //std::cout << getPathName() << ": _p1_queue.deq.bits "  << _p1_queue->io.deq.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": _p2_queue.deq.valid " << static_cast<bool>(_p2_queue->io.deq.isValid()) << std::endl;
        //std::cout << getPathName() << ": _p2_queue.deq.ready " << static_cast<bool>(_p2_queue->io.deq.isReady()) << std::endl;
        //std::cout << getPathName() << ": _p2_queue.deq.bits "  << _p2_queue->io.deq.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.deq.valid " << static_cast<bool>(io.deq.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.deq.ready " << static_cast<bool>(io.deq.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.deq.bits "  << io.deq.bits().toBigUInt() << std::endl;
    }


    // ========================== SidebandSerializer ===============================
    SidebandSerializer::
    SidebandSerializer(const SidebandParams &sb_params, const FdiParams &fdi_params):
        _sending{createReg<Bool>(Bool(false))},
        _data{createReg<UInt>(UInt(0, 0))},
        _current_credit{createReg<UInt>(UInt(0, 0))},
        _is_complete{createReg<Bool>(Bool(false))},
        _sb_w(fdi_params.sb_width),
        _io_in_ready{
            [this]() -> Bool {
                bool res = (!_sending->read() &&
                    (
                        SBM().isComplete(io.in.bits()) ||
                        (_current_credit->read().toBigUInt() > BigUInt(0))
                    )
                );
                return Bool(res);
            }
        },
        _io_out_valid{
            [this]() -> Bool {
                bool res = _sending->read() && (_current_credit->read() || _is_complete->read());
                return Bool(res);
            }
        },
        _io_out_bits{
            [this]() -> UInt {
                return UInt(_sb_w, _data->read().toBigUInt(_sb_w - 1, 0));
            }
        }

    {
        _count = createSubmodule<Counter>("count");

        int msg_w = sb_params.sb_node_msg_width;
        int cdt_max = sb_params.max_crd;

        int data_bits = msg_w;
        int data_beats = (data_bits - 1) / _sb_w + 1;
        _data->init(UInt(data_bits, 0));

        _count->init(io.out.valid, data_beats);
        _current_credit->init(UInt(log2Ceil(cdt_max) + 1, cdt_max));

        io.in.assignReady(_io_in_ready);
        io.out.valid = (_io_out_valid);
        io.out.bits = (_io_out_bits);
    }

    void SidebandSerializer::calcNextState() {
        if (io.in.fire()) {
            *_data = io.in.bits();
            *_sending = Bool(true);
            *_is_complete = Bool(SBM().isComplete(io.in.bits()));
        }

        if (io.out.valid()) {
            *_data = _data->read().operator >> (_sb_w);
        }

        if (_count->isDone()) {
            *_sending = Bool(false);
            if (!_is_complete->read()) {
                *_current_credit = _current_credit->read() - UInt(1, 1);
            }
        }

        if (io.out.credit()) {
            *_current_credit = _current_credit->read() + UInt(1, 1);
        }

        //std::cout << "io.in.valid " << io.in.isValid()() << std::endl;
        //std::cout << "io.in.ready " << io.in.isReady()() << std::endl;
        //std::cout << "io.in.bits 0x" << std::hex << io.in.bits()() << std::endl;
        //std::cout << "io.out.valid " << io.out.valid()() << std::endl;
        //std::cout << "io.out.bits 0x" << std::hex << io.out.bits()() << std::endl;
        //std::cout << "io.out.credit " << io.out.credit()() << std::endl;
    }


    // ========================== SidebandDeserializer ===============================
    SidebandDeserializer::
    SidebandDeserializer(const SidebandParams &sb_params, const FdiParams &fdi_params):
        //_data{createReg<UInt>(UInt(0, 0))},
        _receiving{createReg<Bool>(Bool(true))},
        _sb_w(fdi_params.sb_width),
        _io_out_valid{
            [this]() -> Bool {
                return !_receiving->read();
            }
        },
        _io_out_bits{
            [this]() -> UInt {
                size_t s = _data.size();
                _cat_data = _data[s-1]->read();
                for (size_t i = 1; i < _data.size(); i++) {
                    _cat_data.append(_data[s-i-1]->read());
                }
                return _cat_data;
            }
        }

    {
        _count = createSubmodule<Counter>("count");

        int msg_w = sb_params.sb_node_msg_width;
        int cdt_max = sb_params.max_crd;

        int data_bits = msg_w;
        int data_beats = (data_bits - 1) / _sb_w + 1;

        _data.resize(data_beats);
        for (size_t i = 0; i < _data.size(); i++) {
            _data[i] = createReg<UInt>(UInt(0, 0));
            _data[i]->init(UInt(data_bits, 0));
        }
        _count->init(io.in.valid, data_beats);

        io.out.assignValid(_io_out_valid);
        io.out.assignBits(_io_out_bits);
    }

    void SidebandDeserializer::calcNextState() {
        if (io.in.valid()) {
            *_data[static_cast<unsigned>(_count->getCount().toBigUInt())] = io.in.bits();
        }
        if (_count->isDone()) {
            *_receiving = Bool(false);
        }
        if (io.out.fire()) {
            *_receiving = Bool(true);
        }

        //std::cout << "io.in.valid " << io.in.valid()() << std::endl;
        //std::cout << "io.in.bits 0x" << std::hex << io.in.bits()() << std::endl;
        //std::cout << "io.out.valid " << io.out.isValid()() << std::endl;
        //std::cout << "io.out.bits 0x" << std::hex << io.out.bits()() << std::endl;
        //std::cout << "io.out.ready " << io.out.isReady()() << std::endl;
        //std::cout << "_data size " << _data.size() << std::endl;
    }

    // =============================== SidebandNode ==========================
    SidebandNode::
    SidebandNode(const SidebandParams& sb_params, const FdiParams& fdi_params) {
        // instantiation
        _tx_ser = createSubmodule<SidebandSerializer>("tx_ser", sb_params, fdi_params);
        _rx_queue = createSubmodule<SidebandPriorityQueue>("rx_queue", sb_params);
        _rx_des = createSubmodule<SidebandDeserializer>("rx_des", sb_params, fdi_params);

        _io_outer_rx_credit = [this]() -> Bool {
            if (_rx_queue->io.deq.fire()) {
                auto data = _rx_queue->io.deq.bits();
                return !SBM().isComplete(data);
            }
            return Bool(false);
            //return _rx_queue->io.deq.fire() && (!SBM().isComplete(_rx_queue->io.deq.bits()));
        };
        _tx_ser_io_in_bits = [this]() -> UInt {
            return io.inner.layer_to_node.bits();
        };
        _tx_ser_io_in_valid = [this]() -> Bool {
            return io.inner.layer_to_node.isValid();
        };
        _io_inner_layer_to_node_ready = [this]() -> Bool {
            return _tx_ser->io.in.isReady();
        };

        // Connect outer signals
        io.outer.tx.bits = _tx_ser->io.out.bits;
        io.outer.tx.valid = _tx_ser->io.out.valid;
        _tx_ser->io.out.credit = io.outer.tx.credit;

        _rx_des->io.in.valid = io.outer.rx.valid;
        _rx_des->io.in.bits = io.outer.rx.bits;
        io.outer.rx.credit = _io_outer_rx_credit;

        // Connect rx queue and deserializer
        _rx_queue->io.enq.connect(_rx_des->io.out);

        // Connect inner signals
        io.inner.layer_to_node.assignReady(_io_inner_layer_to_node_ready);

        _tx_ser->io.in.assignBits(_tx_ser_io_in_bits);
        _tx_ser->io.in.assignValid(_tx_ser_io_in_valid);

        io.inner.node_to_layer.connect(_rx_queue->io.deq);
    }

    void SidebandNode::calcNextState() {
        //static int step = 0;
        //std::cout << getPathName() << "======================== step " << step++ << " ======================"<< std::endl;
        //std::cout << getPathName() << ": io.outer.rx.valid " << static_cast<bool>(io.outer.rx.valid()) << std::endl;
        //std::cout << getPathName() << ": io.outer.rx.bits "  << io.outer.rx.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": rx_des.io.in.valid "  << static_cast<bool>(_rx_des->io.in.valid()) << std::endl;
        //std::cout << getPathName() << ": rx_des.io.in.bits "   << _rx_des->io.in.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": rx_queue.io.enq.valid "  << static_cast<bool>(_rx_queue->io.enq.isValid()) << std::endl;
        //std::cout << getPathName() << ": rx_queue.io.enq.ready "  << static_cast<bool>(_rx_queue->io.enq.isReady()) << std::endl;
        //std::cout << getPathName() << ": rx_queue.io.enq.bits  "  << _rx_queue->io.enq.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": rx_queue.io.deq.valid "  << static_cast<bool>(_rx_queue->io.deq.isValid()) << std::endl;
        //std::cout << getPathName() << ": rx_queue.io.deq.ready "  << static_cast<bool>(_rx_queue->io.deq.isReady()) << std::endl;
        //std::cout << getPathName() << ": rx_queue.io.deq.bits  "  << _rx_queue->io.deq.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.inner.node_to_layer.valid "  << static_cast<bool>(io.inner.node_to_layer.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.inner.node_to_layer.ready "  << static_cast<bool>(io.inner.node_to_layer.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.inner.node_to_layer.bits  "  << io.inner.node_to_layer.bits().toBigUInt() << std::endl;
    }

    // =============================== SidebandLinkSerializer ==========================
    SidebandLinkSerializer::SidebandLinkSerializer(
        const SidebandParams& sb_params,
        const FdiParams& fdi_params
    ):
        _data{createReg<UInt>()},
        _counter{createReg<UInt>(UInt(32, 0))},
        _sending{createReg<Bool>(Bool(false))},
        _done{createReg<Bool>(Bool(false))},
        _waited{createReg<Bool>(Bool(true))},
        _count{createSubmodule<Counter>("count")}
    {
        _sb_w = 1;
        int msg_w = sb_params.sb_node_msg_width;
        int data_bits = msg_w;
        int data_beats = (data_bits - 1) / _sb_w + 1;

        _data->init(UInt(data_bits, 0));
        _counter_en = [this]() -> Bool {
            return Bool(_done->read());
        };
        _counter_next = [this]() -> UInt {
            BigUInt v = 0;
            if (io.in.fire()) {
                v = 0;
            } else {
                BigUInt cv = _counter->read().toBigUInt();
                if (cv == 31) {
                    v = 31;
                } else {
                    v = cv + 1;
                }
            }
            return UInt(log2Ceil(32), v);
        };
        _io_in_ready = [this]() -> Bool {
            return Bool(_waited->read());
        };
        _io_out_bits = [this]() -> UInt {
            return _data->read()(_sb_w - 1, 0);
        };
        _io_counter = [this]() -> UInt {
            return _counter->read();
        };
        _count_en = [this]() -> Bool {
            return Bool(_sending->read());
        };

        io.in.assignReady(_io_in_ready);
        io.out.bits = _io_out_bits;
        io.counter = _io_counter;
        _count->init(_count_en, data_beats);
    }

    bool SidebandLinkSerializer::propagateClock() {
        if (io.out.clock == nullptr) {
            _clock_enable_func = [this]() -> bool {
                return static_cast<bool>(_sending->read());
            };
            io.out.clock = createGatedClock(getClock(), getPathName() + "_gated_clock", _clock_enable_func);
        }

        return Module::propagateClock();
    }

    void SidebandLinkSerializer::calcNextState() {
        if (io.in.fire()) {
            *_data = io.in.bits();
            *_sending = Bool(true);
            *_waited = Bool(false);
        }

        if (_sending->read()) {
            *_data = _data->read().operator >> (_sb_w);
        }

        if (_count->isDone()) {
            *_sending = Bool(false);
            *_done = Bool(true);
        }

        if (_done->read()) {
            *_waited = Bool(_counter->read().toBigUInt() == 31);
        }

        if (_counter_en()) {
            *_counter = _counter_next();
        }

        //std::cout << "in " << getPathName() << ", io.in.fire " << io.in.fire().operator bool() << std::endl;
        //std::cout << "in " << getPathName() << ", sending " << _sending->read().operator bool() << std::endl;
        //std::cout << "in " << getPathName() << ", count.count " << _count->getCount().toBigUInt() << std::endl;
        //std::cout << "in " << getPathName() << ", count.isDone " << _count->isDone().operator bool() << std::endl;
        //std::cout << "in " << getPathName() << ", done " << _done->read().operator bool() << std::endl;
        //std::cout << "in " << getPathName() << ", counter " << _counter->read().toBigUInt() << std::endl;
        //std::cout << "in " << getPathName() << ", waited " << _waited->read().operator bool() << std::endl;
        //std::cout << "in " << getPathName() << ", io.out.bits " << io.out.bits().toBigUInt() << std::endl;
    }

    // =============================== SidebandLinkDeserializerRemoteClock ==========================
    SidebandLinkDeserializerRemoteClock::SidebandLinkDeserializerRemoteClock(
        const SidebandParams& sb_params,
        const FdiParams& fdi_params
    ) {
        int sb_w = 1;
        int msg_w = sb_params.sb_node_msg_width;
        int data_bits = msg_w;
        int data_beats = (data_bits - 1) / sb_w + 1;

        // instantiation
        _count_inc = [this]() -> Bool {
            return Bool(true);
        };
        count = createSubmodule<Counter>("counter", _count_inc, data_beats);

        recv_count_delay = createReg<UInt>(UInt(log2Ceil(data_beats), 0));
    }

    void SidebandLinkDeserializerRemoteClock::calcNextState() {
        *recv_count_delay = count->getCount();
    }

    // =============================== SidebandLinkDeserializer ==========================
    SidebandLinkDeserializer::SidebandLinkDeserializer(
        const SidebandParams& sb_params,
        const FdiParams& fdi_params
    ) {
        int sb_w = 1;
        int msg_w = sb_params.sb_node_msg_width;
        int data_bits = msg_w;
        int data_beats = (data_bits - 1) / sb_w + 1;

        // instantiation
        _remote_clock_module = createSubmodule<SidebandLinkDeserializerRemoteClock>(
            "remote_clock_module", sb_params, fdi_params);

        _data.resize(data_beats);
        for (auto& d : _data) {
            d = createReg<UInt>(UInt(sb_w, 0));
        }
        _receiving = createReg<Bool>(Bool(true));

        _io_out_valid = [this]() {
            return !_receiving->read();
        };

        _io_out_bits = [this]() {
            size_t s = _data.size();
            _cat_data = _data[s-1]->read();
            for (size_t i = 1; i < _data.size(); i++) {
                _cat_data.append(_data[s-i-1]->read());
            }
            return _cat_data;
        };

        io.out.assignValid(_io_out_valid);
        io.out.assignBits(_io_out_bits);
    }

    bool SidebandLinkDeserializer::propagateClock() {
        //std::cout << getPathName() << " propagateClock" << std::endl;
        if (getClock() == nullptr) {
            std::cerr << getPathName() << " clock has not been set." << std::endl;
            assert(false);
            return false;
        }
        if (io.in.remote_clock == nullptr) {
            return false;
        } else {
            if (_remote_clock_module->getClock() == nullptr) {
                addModuleToClock(io.in.remote_clock, _remote_clock_module);
            }
            return Module::propagateClock();
        }
    }

    void SidebandLinkDeserializer::calcNextState() {
        unsigned index = static_cast<unsigned>(_remote_clock_module->recv_count_delay->read().toBigUInt());
        *_data[index] = io.in.bits();
        if (_remote_clock_module->count->isDone()) {
            *_receiving = Bool(false);
        }
        if (io.out.fire()) {
            *_receiving = Bool(true);
        }

        //std::cout << "in " << getPathName() << ", io.in.bits " << io.in.bits().toBigUInt() << std::endl;
        //std::cout << "in " << getPathName() << ", _recv_count_delay  " << _remote_clock_module->recv_count_delay->read().toBigUInt() << std::endl;
        //std::cout << "in " << getPathName() << ", io.out.valid " << static_cast<bool>(io.out.isValid()) << std::endl;
        //std::cout << "in " << getPathName() << ", io.out.ready " << static_cast<bool>(io.out.isReady()) << std::endl;
        //std::cout << "in " << getPathName() << ", io.out.bits  " << io.out.bits().toBigUInt() << std::endl;
        //std::cout << "in " << getPathName() << ", count.count  " << _remote_clock_module->count->getCount().toBigUInt() << std::endl;
        //std::cout << "in " << getPathName() << ", count.isDone  " << static_cast<bool>(_remote_clock_module->count->isDone()) << std::endl;
        //std::cout << "in " << getPathName() << ", receiving  " << static_cast<bool>(_receiving->read()) << std::endl;
    }

    // =============================== SidebandLinkNode ==========================
    SidebandLinkNode::SidebandLinkNode(
        const SidebandParams& sb_params,
        const FdiParams& fdi_params
    ) {
        _tx_ser = createSubmodule<SidebandLinkSerializer>("tx_ser", sb_params, fdi_params);
        _rx_des = createSubmodule<SidebandLinkDeserializer>("rx_des", sb_params, fdi_params);
        _rx_queue = createSubmodule<SidebandPriorityQueue>("rx_queue", sb_params);

        // Connect outer signals
        io.outer.tx.bits = _tx_ser->io.out.bits;
        _rx_des->io.in.bits = io.outer.rx.bits;

        // Connect rx queue and deserializer
        _rx_queue_io_enq_valid = [this]() -> Bool {
            if (io.rx_mode().toBigUInt() == RXTXMode::PACKET) {
                return _rx_des->io.out.isValid();
            } else {
                return Bool(false);
            }
        };
        _rx_queue->io.enq.assignValid(_rx_queue_io_enq_valid);

        _rx_queue_io_enq_bits = [this]() -> UInt {
            return _rx_des->io.out.bits();
        };
        _rx_queue->io.enq.assignBits(_rx_queue_io_enq_bits);

        _rx_des_io_out_ready = [this]() -> Bool {
            if (io.rx_mode().toBigUInt() == RXTXMode::PACKET) {
                return _rx_queue->io.enq.isReady();
            } else {
                return Bool(false);
            }
        };
        _rx_des->io.out.assignReady(_rx_des_io_out_ready);

        // Connect inner signals
        _io_inner_layer_to_node_ready = [this]() -> Bool {
            return _tx_ser->io.in.isReady();
        };
        io.inner.layer_to_node.assignReady(_io_inner_layer_to_node_ready);

        _tx_ser_io_in_bits = [this] () -> UInt {
            _cat_tx_ser_io_in_bits = io.inner.layer_to_node.bits()(127, 59);
            _cat_tx_ser_io_in_bits.append(UInt(1, 0));
            _cat_tx_ser_io_in_bits.append(io.inner.layer_to_node.bits()(57, 0));
            return _cat_tx_ser_io_in_bits;
        };
        _tx_ser->io.in.assignBits(_tx_ser_io_in_bits);

        _tx_ser_io_in_valid = [this]() -> Bool {
            return io.inner.layer_to_node.isValid();
        };
        _tx_ser->io.in.assignValid(_tx_ser_io_in_valid);

        _io_inner_node_to_layer_valid = [this]() -> Bool {
            if (io.rx_mode().toBigUInt() == RXTXMode::PACKET) {
                return _rx_queue->io.deq.isValid();
            } else {
                return _rx_des->io.out.isValid();
            }
        };
        io.inner.node_to_layer.assignValid(_io_inner_node_to_layer_valid);

        _io_inner_node_to_layer_bits = [this]() -> UInt {
            if (io.rx_mode().toBigUInt() == RXTXMode::PACKET) {
                return _rx_queue->io.deq.bits();
            } else {
                return _rx_des->io.out.bits();
            }
        };
        io.inner.node_to_layer.assignBits(_io_inner_node_to_layer_bits);

        _rx_queue_io_deq_ready = [this]() -> Bool {
            if (io.rx_mode().toBigUInt() == RXTXMode::PACKET) {
                return io.inner.node_to_layer.isReady();
            } else {
                return Bool(false);
            }
        };
        _rx_queue->io.deq.assignReady(_rx_queue_io_deq_ready);
    }

    bool SidebandLinkNode::propagateClock() {
        //std::cout << getPathName() << " propagateClock" << std::endl;
        bool submodule_success = true;
        if (io.outer.rx.clock != nullptr) {
            _rx_des->io.in.remote_clock = io.outer.rx.clock;
        } else {
            submodule_success = false;
        }
        submodule_success &= Module::propagateClock();
        io.outer.tx.clock = _tx_ser->io.out.clock;
        return submodule_success;
    }

} // namespace CCPS
