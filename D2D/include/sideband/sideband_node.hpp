#ifndef __SIDEBAND_NODE_HPP__
#define __SIDEBAND_NODE_HPP__

#include "utils/module.hpp"
#include "utils/counter.hpp"
#include "sideband/sideband_io.hpp"
#include "utils/queue.hpp"
#include "utils/decoupled.hpp"
#include "utils/clock.hpp"

namespace CCPS {
    class SidebandSerializer;


    // =============================== SidebandSerializer ==========================
    class SidebandSerializer: public RegModule {
    public:
        SidebandSerializer(const SidebandParams &sb_params, const FdiParams &fdi_params);

        struct {
            Decoupled<UInt> in{true};
            struct {
                Wire<UInt> bits;
                Wire<Bool> valid;
                Wire<Bool> credit;
            } out;
        } io;

        void calcNextState()  override;

    private:
        // ============ chisel variables ==============
        RegPtr<Bool> _sending;
        ModulePtr<Counter> _count;
        RegPtr<UInt> _data;
        RegPtr<UInt> _current_credit;
        RegPtr<Bool> _is_complete;

        // ============ our variables ==============
        unsigned _sb_w;

        Wire<Bool>::TPFUNC _io_in_ready;
        Wire<Bool>::TPFUNC _io_out_valid;
        Wire<UInt>::TPFUNC _io_out_bits;
    };

    // =============================== SidebandDeserializer ==========================
    class SidebandDeserializer: public RegModule {
    public:
        SidebandDeserializer(const SidebandParams &sb_params, const FdiParams &fdi_params);

        struct {
            Decoupled<UInt> out;
            struct {
                Wire<UInt> bits;
                Wire<Bool> valid;
            } in;
        } io;

        void calcNextState()  override;

    private:
        // ============ chisel variables ==============
        std::vector<RegPtr<UInt>> _data;
        RegPtr<Bool> _receiving;
        ModulePtr<Counter> _count;

        // ============ our variables ==============
        unsigned _sb_w;

        Wire<Bool>::TPFUNC _io_out_valid;
        Wire<UInt>::TPFUNC _io_out_bits;
        UInt _cat_data;
    };

    // =============================== SidebandEnqArbiter ==========================
    class SidebandEnqArbiter: public WireModule {
    public:
        struct {
            std::vector<Decoupled<UInt> > out;
            Decoupled<UInt> in{true};
        } io;

        SidebandEnqArbiter(const SidebandParams &sb_params);

        void calcNextState() override;

    private:
       std::vector<Wire<Bool>::TPFUNC> _io_out_valid;
       std::vector<Wire<UInt>::TPFUNC> _io_out_bits;
       Wire<Bool>::TPFUNC _io_in_ready;
    };

    // =============================== SidebandDeqArbiter ==========================
    class SidebandDeqArbiter: public WireModule {
    public:
        struct {
            std::vector<Decoupled<UInt> > in;
            Decoupled<UInt> out;
        } io;

        SidebandDeqArbiter(const SidebandParams &sb_params);

    private:
        Wire<Bool>::TPFUNC _io_out_valid;
        Wire<UInt>::TPFUNC _io_out_bits;
        std::vector<Wire<Bool>::TPFUNC> _io_in_ready;
    };

    // =============================== SidebandPriorityQueue ==========================
    class SidebandPriorityQueue: public WireModule {
    public:
        SidebandPriorityQueue(const SidebandParams &sb_params);

        struct {
            Decoupled<UInt> enq{true};
            Decoupled<UInt> deq;
        } io;

        void calcNextState()  override;

    private:
        // =========================== chisel internal signals ==========================
        // highest priority queue, for access completion packets
        ModulePtr<Queue<UInt>> _p0_queue;
        // second highest priority queue, for message packets
        ModulePtr<Queue<UInt>> _p1_queue;
        // lowest priority queue, for access request packets
        ModulePtr<Queue<UInt>> _p2_queue;

        ModulePtr<SidebandEnqArbiter> _enq_arb;
        ModulePtr<SidebandDeqArbiter> _deq_arb;
    };

    // =============================== SidebandNode ==========================
    class SidebandNode: public WireModule {
    public:
        SidebandNode(
            const SidebandParams& sb_params,
            const FdiParams& fdi_params
        );

        SidebandNodeIO io;

        void calcNextState()  override;

    private:
        ModulePtr<SidebandSerializer> _tx_ser;
        ModulePtr<SidebandPriorityQueue> _rx_queue;
        ModulePtr<SidebandDeserializer> _rx_des;

        // =============== helper variables ==============
        Wire<Bool>::TPFUNC _io_outer_rx_credit;
        Wire<UInt>::TPFUNC _tx_ser_io_in_bits;
        Wire<Bool>::TPFUNC _tx_ser_io_in_valid;
        Wire<Bool>::TPFUNC _io_inner_layer_to_node_ready;
    };

    // =============================== SidebandLinkSerializer ==========================
    class SidebandLinkSerializer: public RegModule {
    public:
        struct {
            Decoupled<UInt> in{true};
            struct {
                Wire<UInt> bits;
                ClockPtr clock;
            } out;
            Wire<UInt> counter;
        } io;

        SidebandLinkSerializer(
            const SidebandParams& sb_params,
            const FdiParams& fdi_params
        );

        void calcNextState()  override;
        // override propagateClock to generate gated clock.
        bool propagateClock() override;

    private:
        // ============== chisel signals ================
        RegPtr<UInt> _data;
        Wire<Bool> _counter_en;
        Wire<UInt> _counter_next;
        RegPtr<UInt> _counter;
        RegPtr<Bool> _sending;
        RegPtr<Bool> _done;
        RegPtr<Bool> _waited;
        ModulePtr<Counter> _count;
        RegPtr<Bool> _is_complete;

        // ============ helper signals ===========================
        int _sb_w;
        Wire<Bool>::TPFUNC _io_in_ready;
        Wire<UInt>::TPFUNC _io_out_bits;
        Wire<UInt>::TPFUNC _io_counter;
        Wire<Bool> _count_en;
        ClockPtr _clock;
        GatedClock::ENABLE_FUNC _clock_enable_func;
    };

    class SidebandLinkDeserializerRemoteClock: public RegModule {
    public:
        SidebandLinkDeserializerRemoteClock(
            const SidebandParams& sb_params,
            const FdiParams& fdi_params
        );

        struct {
            struct {
                Wire<UInt> bits;
            } in;
        } io;

        ModulePtr<Counter> count;
        RegPtr<UInt> recv_count_delay;

        void calcNextState() override;

    private:
        // ============ helper signals ===========================
        Wire<Bool> _count_inc;
    };

    // =============================== SidebandLinkDeserializer ==========================
    class SidebandLinkDeserializer: public RegModule {
        public:
            struct {
                struct {
                    Wire<UInt> bits;
                    ClockPtr remote_clock;
                } in;
                Decoupled<UInt> out;
            } io;

            SidebandLinkDeserializer(
                const SidebandParams& sb_params,
                const FdiParams& fdi_params
            );

            bool propagateClock() override;
            void calcNextState()  override;

        private:
            // ============== chisel signals ================
            //ModulePtr<Counter> _count;
            //RegPtr<UInt> _recv_count_delay;
            ModulePtr<SidebandLinkDeserializerRemoteClock> _remote_clock_module;
            std::vector<RegPtr<UInt>> _data;
            RegPtr<Bool> _receiving;

            // ============ helper signals ===========================
            //Wire<Bool> _count_inc;
            Wire<Bool> _io_out_valid;
            Wire<UInt> _io_out_bits;
            UInt _cat_data;
    };

    // =============================== SidebandLinkNode ==========================
    class SidebandLinkNode: public WireModule {
    public:
        SidebandLinkIO io;

        SidebandLinkNode(
            const SidebandParams& sb_params,
            const FdiParams& fdi_params
        );

        // override propagateClock to generate gated clock.
        bool propagateClock() override;

    private:
        // ============== chisel signals ================
        ModulePtr<SidebandLinkSerializer> _tx_ser;
        ModulePtr<SidebandLinkDeserializer> _rx_des;
        ModulePtr<SidebandPriorityQueue> _rx_queue;

        // ============ helper signals ===========================
        Wire<Bool> _io_inner_layer_to_node_ready;
        Wire<UInt> _tx_ser_io_in_bits;
        UInt _cat_tx_ser_io_in_bits;
        Wire<Bool> _tx_ser_io_in_valid;
        Wire<Bool> _io_inner_node_to_layer_valid;
        Wire<UInt> _io_inner_node_to_layer_bits;
        Wire<Bool> _io_inner_node_to_layer_ready;
        Wire<Bool> _rx_queue_io_enq_valid;
        Wire<UInt> _rx_queue_io_enq_bits;
        Wire<Bool> _rx_des_io_out_ready;
        Wire<Bool> _rx_queue_io_deq_ready;

    };

}

#endif // __SIDEBAND_NODE_HPP__