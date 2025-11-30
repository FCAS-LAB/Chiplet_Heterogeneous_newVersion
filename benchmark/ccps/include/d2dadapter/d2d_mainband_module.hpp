#ifndef __D2DADAPTER_D2D_MAINBAND_MODULE_HPP__
#define __D2DADAPTER_D2D_MAINBAND_MODULE_HPP__

#include "interfaces/types.hpp"
#include "utils/module.hpp"
#include "utils/reg.hpp"
#include "interfaces/fdi.hpp"
#include "interfaces/rdi.hpp"
#include "sideband/sideband_io.hpp"

namespace CCPS {
    struct D2DMainbandModuleIO {
        //protocol to d2d
        Wire<Bool> fdi_lp_irdy;         // input
        Wire<Bool> fdi_lp_valid;        // input
        Wire<UInt> fdi_lp_data;         // input
        ProtoStream fdi_lp_stream;      // input
        Wire<Bool> fdi_pl_trdy;         // output
        // d2d to protocol
        Wire<Bool> fdi_pl_valid;        // output
        Wire<UInt> fdi_pl_data;         // output
        ProtoStream fdi_pl_stream;      // output
        // d2d to physical
        Wire<Bool> rdi_lp_irdy;         // output
        Wire<Bool> rdi_lp_valid;        // output
        Wire<UInt> rdi_lp_data;         // output
        Wire<Bool> rdi_pl_trdy;         // input
        // physical to d2d
        Wire<Bool> rdi_pl_valid;        // input
        Wire<UInt> rdi_pl_data;         // input

        Wire<UInt> d2d_state;       // input

        Wire<Bool> mainband_stallreq;// // input
        Wire<Bool> mainband_stalldone;// complete stall // output

        Wire<UInt> snd_data;            // output
        Wire<Bool> snd_data_vld;        // output
        Wire<UInt> rcv_data;            // output
        Wire<Bool> rcv_data_vld;        // output
        Wire<Bool> parity_insert;// need to send parity     // input
        Wire<UInt> parity_data;// the data needed to be sent first as parity        // input
        Wire<Bool> parity_rdy; // indicating that the parity data is sent, next can come in         // output
        Wire<Bool> parity_check;// mean that the next data from RDI does not need to send to FDI    // input
    };

    class D2DMainbandModule: public RegModule {
    public:
        D2DMainbandModuleIO io;

        D2DMainbandModule(
            const FdiParams &fdi_params,
            const RdiParams &rdi_params,
            const SidebandParams &sb_params
        );

        void calcNextState() override;

    private:
        // ============ chisel signals ==============
        RegPtr<UInt> _data_buff_snt_reg;
        RegPtr<Bool> _data_buff_snt_fill_reg;
        RegPtr<UInt> _data_buff_rcv_reg;
        RegPtr<Bool> _data_buff_rcv_fill_reg;
        RegPtr<Bool> _stall_reg;

        Wire<Bool> _snd_success_rdi;
        Wire<UInt> _streaming_proto_stack;
        Wire<UInt> _streaming_proto_type;


        // ============ helper signals ==============
        Wire<Bool>::TPFUNC _fdi_pl_trdy;
        Wire<Bool>::TPFUNC _fdi_pl_valid;
        Wire<UInt>::TPFUNC _fdi_pl_data;
        Wire<UInt>::TPFUNC _fdi_pl_stream_proto_stack;
        Wire<UInt>::TPFUNC _fdi_pl_stream_proto_type;
        Wire<Bool>::TPFUNC _rdi_lp_irdy;
        Wire<Bool>::TPFUNC _rdi_lp_valid;
        Wire<UInt>::TPFUNC _rdi_lp_data;
        Wire<Bool>::TPFUNC _mainband_stalldone;
        Wire<UInt>::TPFUNC _snd_data;
        Wire<Bool>::TPFUNC _snd_data_vld;
        Wire<UInt>::TPFUNC _rcv_data;
        Wire<Bool>::TPFUNC _rcv_data_vld;
        Wire<Bool>::TPFUNC _parity_rdy;

    };
} // namespace CCPS

#endif // __D2DADAPTER_D2D_MAINBAND_MODULE_HPP__
