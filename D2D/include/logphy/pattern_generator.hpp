#ifndef __PATTERN_GENERATOR_HPP__
#define __PATTERN_GENERATOR_HPP__

#include "utils/wire.hpp"
#include "utils/base_types.hpp"
#include "utils/decoupled.hpp"
#include "utils/module.hpp"
#include "interfaces/afe.hpp"
#include "sideband/sideband_io.hpp"
#include "logphy/log_phy_types.hpp"
#include "utils/reg.hpp"
#include <map>

namespace CCPS {

    struct PatternGeneratorIO {
        struct {
            Wire<Bool> valid;          // I
            Wire<Bool> ready;          // O
            Wire<UInt> pattern;        // I  // 1bit
            Wire<UInt> timeout_cycles; // I  // 32bits
            Wire<Bool> sideband;       // I
        } transmit_req;
        Decoupled<UInt> transmit_pattern_status; // 1bit

        PatternGeneratorIO(bool do_flip = false) {
            if (do_flip) {
                flip();
            }
        }
        void flip() {
            transmit_pattern_status.flip();
        }
    };

    class PatternGenerator: public RegModule {
    public:
        struct {
            PatternGeneratorIO pattern_generator_io;
            SidebandLaneIO sideband_lane_io{true};
        } io;

        PatternGenerator(const AfeParams &afe_params, const SidebandParams &sb_params);
        void calcNextState() override;

    private:
        // ================= chisel signals ===============
        RegPtr<Bool> _write_in_progress;
        RegPtr<Bool> _read_in_progress;
        Wire<Bool> _in_progress;
        RegPtr<UInt> _pattern;
        RegPtr<Bool> _sideband;
        RegPtr<UInt> _timeout_cycles;
        RegPtr<UInt> _status;
        RegPtr<Bool> _status_valid;
        BigUInt _clock_pattern_shift_reg_biguint{"0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
        Wire<UInt> _pattern_to_transmit;
        RegPtr<UInt> _pattern_detected_count;
        RegPtr<UInt> _pattern_written_count;
        std::map<TransmitPattern, int> _pattern_written_count_max = {{TransmitPattern::CLOCK_64_LOW_32, 2}};
        std::map<TransmitPattern, int> _pattern_detected_count_max = {{TransmitPattern::CLOCK_64_LOW_32, 128}};
        BigUInt _pattern_to_detect{"0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
        SidebandParams _sb_params;
    };

} // namespace CCPS

#endif // __PATTERN_GENERATOR_HPP__