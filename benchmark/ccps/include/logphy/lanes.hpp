#ifndef __LANES_HPP__
#define __LANES_HPP__

#include "logphy/log_phy_types.hpp"
#include "utils/async_queue.hpp"
#include "utils/queue.hpp"

namespace CCPS {

    // ======================================= Lanes ===================================
    class Lanes: public WireModule {
    public:
        struct {
            MainbandIO mainband_io;
            MainbandLaneIO mainband_lane_io;
        } io;

        Lanes(const AfeParams &afe_params, const AsyncQueueParams &queue_params);
        void calcNextState() override;
        bool propagateClock() override;

    private:
        // ================ chisel signals =======================
        int _ratio_bytes;
        std::vector<ModulePtr<AsyncQueue<UInt>>> _tx_mb_fifo;
        std::vector<ModulePtr<AsyncQueue<UInt>>> _rx_mb_fifo;
        std::vector<std::vector<Wire<UInt>>> _tx_data_vec;
        std::vector<std::vector<Wire<UInt>>> _rx_data_vec;

        // ================ helper signals =======================
    };

    // ======================================= MainbandSimIO ===================================
    struct MainbandSimIO {
        std::vector<Decoupled<UInt>> tx_data;
        std::vector<Decoupled<UInt>> rx_data;
    };

    // ======================================= SimLanes ===================================
    class SimLanes: public WireModule {
    public:
        struct {
            MainbandSimIO mainband_io;
            MainbandLaneIO mainband_lane_io;
        } io;

        SimLanes(const AfeParams &afe_params, const AsyncQueueParams &queue_params);

        void calcNextState() override;

    private:
        // =========== chisel signals ==========
        int _ratio_bytes;
        std::vector<ModulePtr<Queue<UInt>>> _tx_mb_fifo;
        std::vector<ModulePtr<Queue<UInt>>> _rx_mb_fifo;
        std::vector<std::vector<Wire<UInt>>> _tx_data_vec;
        std::vector<std::vector<Wire<UInt>>> _rx_data_vec;
    };

} // namespace CCPS


#endif // __LANES_HPP__