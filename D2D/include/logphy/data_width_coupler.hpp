#ifndef __DATA_WIDTH_COUPLER_HPP__
#define __DATA_WIDTH_COUPLER_HPP__

#include "utils/wire.hpp"
#include "utils/decoupled.hpp"
#include "utils/reg.hpp"
#include "utils/module.hpp"

namespace CCPS {
    struct DataWidthCouplerParams {
        int in_width;
        int out_width;
        DataWidthCouplerParams(int in_w, int out_w) : in_width(in_w), out_width(out_w) {}
    };

    struct DataWidthCouplerIO {
        Decoupled<UInt> in{true};
        Decoupled<UInt> out;
    };

    class DataWidthCoupler : public RegModule {
    public:
        DataWidthCouplerIO io;
        DataWidthCoupler(const DataWidthCouplerParams &params);

        void calcNextState() override;

    private:
        enum class State {
            IDLE = 0,
            CHUNK_OR_COLLECT = 1
        };

        // ============= chisel signals ================
        DataWidthCouplerParams _params;
        int _ratio;
        RegPtr<UInt> _current_state;
        RegPtr<UInt> _chunk_counter;
        RegPtr<UInt> _in_slice_counter;
        std::vector<RegPtr<UInt>> _in_data;
    };
} // namespace CCPS

#endif // __DATA_WIDTH_COUPLER_HPP__