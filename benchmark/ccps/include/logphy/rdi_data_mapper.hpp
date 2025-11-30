#ifndef __RDI_DATA_MAPPER_HPP__
#define __RDI_DATA_MAPPER_HPP__

#include "utils/wire.hpp"
#include "utils/decoupled.hpp"
#include "utils/reg.hpp"
#include "utils/module.hpp"
#include "logphy/log_phy_types.hpp"
#include "interfaces/rdi.hpp"
#include "logphy/data_width_coupler.hpp"


namespace CCPS {

    struct RdiDataMapperIO {
        Decoupled<UInt> lp_data;
        Wire<Bool> lp_data_irdy;   // O
        Valid<UInt> pl_data;

        RdiDataMapperIO(bool do_flip=false);
        void flip();
    };

    class RdiDataMapper : public RegModule {
    public:
        struct {
            RdiDataMapperIO rdi{true};
            MainbandLaneIO mainband_lane_io{true};
        } io;

        RdiDataMapper(const RdiParams &rdi_params, const AfeParams &afe_params);

        void calcNextState() override;

    private:
        // ============== chisel signals ================
        RegPtr<UInt> _rx_slice_counter;
        std::vector<RegPtr<UInt>> _rx_data;
        RegPtr<Bool> _has_rx_data;
        ModulePtr<DataWidthCoupler> _tx_width_coupler;

        // ============== helper signals ================
        int _afe_bits;
        int _ratio;
    };

} // namespace CCPS

#endif // __RDI_DATA_MAPPER_HPP__