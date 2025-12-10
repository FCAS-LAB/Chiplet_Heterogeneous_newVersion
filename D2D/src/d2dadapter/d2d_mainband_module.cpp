#include "d2dadapter/d2d_mainband_module.hpp"
#include "interfaces/types.hpp"

namespace CCPS {
    // ========================== D2DMainbandModule ===============================
    D2DMainbandModule::D2DMainbandModule(
        const FdiParams &fdi_params,
        const RdiParams &rdi_params,
        const SidebandParams &sb_params
    ) {
        ENTER_MODULE_FUNC();
        // Instantiate
        _data_buff_snt_reg = createReg<UInt>(UInt(8*fdi_params.width, 0));
        _data_buff_snt_fill_reg = createReg<Bool>(Bool(false));
        _data_buff_rcv_reg = createReg<UInt>(UInt(8*fdi_params.width, 0));
        _data_buff_rcv_fill_reg = createReg<Bool>(Bool(false));
        _stall_reg = createReg<Bool>(Bool(false));

        // connect
        _fdi_pl_trdy = [this]() -> Bool {
            if (!_data_buff_snt_fill_reg->read()) {
                return Bool(true);
            } else if (!io.parity_insert() && _snd_success_rdi()) {
                return Bool(true);
            } else {
                return Bool(false);
            }
        };

        _fdi_pl_valid = [this]() -> Bool {
            if (_data_buff_rcv_fill_reg->read()) {
                return Bool(true);
            } else {
                return Bool(false);
            }
        };

        _fdi_pl_data = [this]() -> UInt {
            return _data_buff_rcv_reg->read();
        };

        _fdi_pl_stream_proto_stack = [this]() -> UInt {
            return UInt(4, ProtoStack::stack0);
        };

        _fdi_pl_stream_proto_type = [this]() -> UInt {
            return UInt(4, ProtoStreamType::Stream);
        };

        _rdi_lp_irdy = [this]() -> Bool {
            if (!io.parity_insert() && _data_buff_snt_fill_reg->read() && !_stall_reg->read()) {
                return Bool(true);
            } else if (io.parity_insert() && !_stall_reg->read()) {
                return Bool(true);
            } else {
                return Bool(false);
            }
        };

        _rdi_lp_valid = [this]() -> Bool {
            if (!io.parity_insert() && _data_buff_snt_fill_reg->read() && !_stall_reg->read()) {
                return Bool(true);
            } else if (io.parity_insert() && !_stall_reg->read()) {
                return Bool(true);
            } else {
                return Bool(false);
            }
        };

        _rdi_lp_data = [this]() -> UInt {
            if (io.parity_insert()) {
                return io.parity_data();
            } else {
                return _data_buff_snt_reg->read();
            }
        };

        _mainband_stalldone = [this]() -> Bool {
            return _stall_reg->read();
        };

        _snd_data = [this]() -> UInt {
            return io.fdi_lp_data();
        };
        _snd_data_vld = [this]() -> Bool {
            return io.fdi_pl_trdy() && io.fdi_lp_valid() && io.fdi_lp_irdy();
        };
        _snd_success_rdi = [this]() -> Bool {
            return io.rdi_lp_irdy() && io.rdi_lp_valid() && io.rdi_pl_trdy();
        };
        _rcv_data = [this]() -> UInt {
            return io.rdi_pl_data();
        };
        _rcv_data_vld = [this]() -> Bool {
            return io.rdi_pl_valid();
        };
        _parity_rdy = [this]() -> Bool {
            if (io.parity_insert() && _snd_success_rdi()) {
                return Bool(true);
            } else {
                return Bool(false);
            }
        };

        // connect to io
        io.fdi_pl_trdy = _fdi_pl_trdy;
        io.fdi_pl_valid = _fdi_pl_valid;
        io.fdi_pl_data = _fdi_pl_data;
        io.fdi_pl_stream.proto_stack = _fdi_pl_stream_proto_stack;
        io.fdi_pl_stream.proto_type = _fdi_pl_stream_proto_type;
        io.rdi_lp_irdy = _rdi_lp_irdy;
        io.rdi_lp_valid = _rdi_lp_valid;
        io.rdi_lp_data = _rdi_lp_data;
        io.mainband_stalldone = _mainband_stalldone;
        io.snd_data = _snd_data;
        io.snd_data_vld = _snd_data_vld;
        io.rcv_data = _rcv_data;
        io.rcv_data_vld = _rcv_data_vld;
        io.parity_rdy = _parity_rdy;
    }

    void D2DMainbandModule::calcNextState() {
        // protocol -> d2d -> phy
        if (io.mainband_stallreq()) {
            *_stall_reg = Bool(true);
        } else if (io.d2d_state().toBigUInt() != PhyState::active) {
            *_stall_reg = Bool(false);
        }

        // what condition does data_buff_snt_reg get refill?
        if (!_data_buff_snt_fill_reg->read()) { // can accept data from fdi
            if (io.fdi_lp_irdy() && io.fdi_lp_valid()) { // fdi has data
                *_data_buff_snt_fill_reg = Bool(true);
                *_data_buff_snt_reg = io.fdi_lp_data();
            } else {
                *_data_buff_snt_fill_reg = Bool(false);
                *_data_buff_snt_reg = io.fdi_lp_data();
            }
        } else if (!io.parity_insert() && _snd_success_rdi()) { // when data can be sent through rdi...
            if (io.fdi_lp_irdy() && io.fdi_lp_valid()) { // fdi has data
                *_data_buff_snt_fill_reg = Bool(true);
                *_data_buff_snt_reg = io.fdi_lp_data();
            } else {
                *_data_buff_snt_fill_reg = Bool(false);
                *_data_buff_snt_reg = _data_buff_snt_reg->read();
            }
        }

        if (io.rdi_pl_valid()) { // push the data
            *_data_buff_rcv_reg = io.rdi_pl_data();
        }

        if (io.rdi_pl_valid() && !io.parity_check()) { // buff will be emptied once there is no actual data come in
            *_data_buff_rcv_fill_reg = Bool(true);
        } else {
            *_data_buff_rcv_fill_reg = Bool(false);
        }

        PRINT(io.fdi_lp_irdy);
        PRINT(io.fdi_lp_valid);
        PRINT(io.fdi_lp_data);
        PRINT(io.fdi_lp_stream.proto_stack);
        PRINT(io.fdi_lp_stream.proto_type);
        PRINT(io.fdi_pl_trdy);
        PRINT(io.fdi_pl_valid);
        PRINT(io.fdi_pl_data);
        PRINT(io.fdi_pl_stream.proto_stack);
        PRINT(io.fdi_pl_stream.proto_type);
        PRINT(io.rdi_lp_irdy);
        PRINT(io.rdi_lp_valid);
        PRINT(io.rdi_lp_data);
        PRINT(io.rdi_pl_trdy);
        PRINT(io.rdi_pl_valid);
        PRINT(io.rdi_pl_data);
        PRINT(io.d2d_state);
        PRINT(io.mainband_stallreq);
        PRINT(io.mainband_stalldone);
        PRINT(io.snd_data);
        PRINT(io.snd_data_vld);
        PRINT(io.rcv_data);
        PRINT(io.rcv_data_vld);
        PRINT(io.parity_insert);
        PRINT(io.parity_data);
        PRINT(io.parity_rdy);
        PRINT(io.parity_check);
    }

} // namespace CCPS