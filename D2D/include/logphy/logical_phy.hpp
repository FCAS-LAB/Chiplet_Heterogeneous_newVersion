#ifndef __LOGICAL_PHY_HPP__
#define __LOGICAL_PHY_HPP__

#include "utils/wire.hpp"
#include "utils/base_types.hpp"
#include "utils/decoupled.hpp"
#include "utils/module.hpp"
#include "sideband/sideband_io.hpp"
#include "logphy/log_phy_types.hpp"
#include "interfaces/afe.hpp"
#include "interfaces/rdi.hpp"
#include "interfaces/fdi.hpp"
#include "logphy/link_training_fsm.hpp"
#include "logphy/mb_init_fsm.hpp"
#include "logphy/rdi_data_mapper.hpp"
#include "logphy/pattern_generator.hpp"
#include "logphy/rdi_bringup.hpp"
#include "logphy/lanes.hpp"
#include "sideband/sideband_channel.hpp"

namespace CCPS {

    class LogicalPhy: public RegModule {
    public:
        struct {
            Rdi rdi;    // flipped
            MainbandAfeIo mb_afe;    // non-flipped
            SidebandAfeIo sb_afe;    // non-flipped
        } io;

        LogicalPhy(
            int my_id,
            const LinkTrainingParams &link_training_params,
            const AfeParams &afe_params,
            const RdiParams &rdi_params,
            const FdiParams &fdi_params,
            const SidebandParams &sb_params,
            const AsyncQueueParams &lane_async_queue_params
        );

        void calcNextState() override;

        bool propagateClock() override;

    private:
        // ============== chisel signals ================
        ModulePtr<LinkTrainingFSM> _training_module;
        ModulePtr<RdiDataMapper> _rdi_data_mapper;
        ModulePtr<Lanes> _lanes;
        ModulePtr<PHYSidebandChannel> _sideband_channel;
    };
} // namespace CCPS

#endif // __LOGICAL_PHY_HPP__