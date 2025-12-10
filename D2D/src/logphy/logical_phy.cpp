#include "logphy/logical_phy.hpp"
#include <fstream>

namespace CCPS {
    LogicalPhy::LogicalPhy(int my_id, const LinkTrainingParams &link_training_params, const AfeParams &afe_params,
                           const RdiParams &rdi_params, const FdiParams &fdi_params, const SidebandParams &sb_params,
                           const AsyncQueueParams &lane_async_queue_params)
    {
        // Instantiate
        _training_module = createSubmodule<LinkTrainingFSM>("training_module", link_training_params, sb_params, afe_params);
        _rdi_data_mapper = createSubmodule<RdiDataMapper>("rdi_data_mapper", rdi_params, afe_params);
        _lanes = createSubmodule<Lanes>("lanes", afe_params, lane_async_queue_params);
        _sideband_channel = createSubmodule<PHYSidebandChannel>("sideband_channel", sb_params, fdi_params, my_id);
        assert(afe_params.sb_serializer_ratio == 1);

        // connect
        _training_module->io.mainband_fsm_io.pll_lock = io.mb_afe.pll_lock;
        _training_module->io.sideband_fsm_io.pll_lock = io.sb_afe.pll_lock;
        io.mb_afe.rx_en = _training_module->io.mainband_fsm_io.rx_en;
        io.sb_afe.rx_en = _training_module->io.sideband_fsm_io.rx_en;
        _training_module->io.rdi.rdi_bringup_io.lp_state_req = io.rdi.lp_state_req;

        io.rdi.pl_error = [this]() -> Bool {
            return Bool(_training_module->io.current_state().toBigUInt() == LinkTrainingState::linkError);
        };
        io.rdi.pl_train_error = [this]() -> Bool {
            return Bool(_training_module->io.current_state().toBigUInt() == LinkTrainingState::linkError);
        };

        io.rdi.pl_non_fatal_error.capture(false);
        io.rdi.pl_correctable_error.capture(false);

        io.rdi.pl_retimer_crd.capture(false);

        io.rdi.pl_phy_in_recenter = [this]() -> Bool {
            return Bool(io.rdi.pl_state_status().toBigUInt() == PhyState::retrain);
        };
        io.rdi.pl_speed_mode = _training_module->io.mainband_fsm_io.tx_freq_sel;
        io.mb_afe.tx_freq_sel = _training_module->io.mainband_fsm_io.tx_freq_sel;
        io.rdi.pl_link_width.capture(3, PhyWidth::width16);
        io.rdi.pl_clk_req = _training_module->io.rdi.rdi_bringup_io.pl_clk_req;
        io.rdi.pl_wake_ack = _training_module->io.rdi.rdi_bringup_io.pl_wake_ack;
        _training_module->io.rdi.rdi_bringup_io.lp_clk_ack = io.rdi.lp_clk_ack;
        _training_module->io.rdi.rdi_bringup_io.lp_wake_req = io.rdi.lp_wake_req;
        io.rdi.pl_stall_req = _training_module->io.rdi.rdi_bringup_io.pl_stall_req;
        _training_module->io.rdi.rdi_bringup_io.lp_stall_ack = io.rdi.lp_stall_ack;
        io.rdi.pl_state_status = _training_module->io.rdi.rdi_bringup_io.pl_state_status;
        _training_module->io.rdi.rdi_bringup_io.lp_link_error = io.rdi.lp_link_error;

        io.rdi.pl_inband_pres = [this]() -> Bool {
            return Bool(_training_module->io.current_state().toBigUInt() == LinkTrainingState::active);
        };

        /** Connect internal FIFO to AFE */
        io.mb_afe.tx_data.resize(_lanes->io.mainband_io.tx_data.size());
        for (size_t i = 0; i < _lanes->io.mainband_io.tx_data.size(); i++) {
            io.mb_afe.tx_data[i].connect(_lanes->io.mainband_io.tx_data[i]);
        }

        io.mb_afe.rx_data.resize(_lanes->io.mainband_io.rx_data.size());
        for (size_t i = 0; i < _lanes->io.mainband_io.rx_data.size(); i++) {
            io.mb_afe.rx_data[i].flip();
            _lanes->io.mainband_io.rx_data[i].connect(io.mb_afe.rx_data[i]);
        }

        // TODO, connect clock in propagrateClock
        _lanes->io.mainband_io.fifo_params.reset = io.mb_afe.fifo_params.reset;

        _lanes->io.mainband_lane_io.tx_data.connect(_rdi_data_mapper->io.mainband_lane_io.tx_data);
        _rdi_data_mapper->io.mainband_lane_io.rx_data.connect(_lanes->io.mainband_lane_io.rx_data);

        /** Connect RDI to Mainband IO */
        _rdi_data_mapper->io.rdi.lp_data.connect(io.rdi.lp_data);
        _rdi_data_mapper->io.rdi.lp_data_irdy = io.rdi.lp_data_irdy;
        io.rdi.pl_data.connect(_rdi_data_mapper->io.rdi.pl_data);

        /** TODO: Double check that this is the right direction */
        io.rdi.pl_config.assignBits(_sideband_channel->io.to_upper_layer.tx.bits);
        io.rdi.pl_config.assignValid(_sideband_channel->io.to_upper_layer.tx.valid);
        _sideband_channel->io.to_upper_layer.tx.credit = io.rdi.pl_config_credit;
        _sideband_channel->io.to_upper_layer.rx.bits = [this] () -> UInt {
            return io.rdi.lp_config.bits();
        };
        _sideband_channel->io.to_upper_layer.rx.valid = [this] () -> Bool {
            return io.rdi.lp_config.isValid();
        };
        io.rdi.lp_config_credit = _sideband_channel->io.to_upper_layer.rx.credit;

        /** Inner connections to lower layer */
        _sideband_channel->io.inner.switcher_bundle.layer_to_node_below.connect(_training_module->io.sideband_fsm_io.packet_tx_data);
        _training_module->io.sideband_fsm_io.rx_data.connect(_sideband_channel->io.inner.switcher_bundle.node_to_layer_below);
        _sideband_channel->io.inner.raw_input.connect(_training_module->io.sideband_fsm_io.pattern_tx_data);
        _sideband_channel->io.inner.input_mode = _training_module->io.sideband_fsm_io.tx_mode;
        _sideband_channel->io.inner.rx_mode = _training_module->io.sideband_fsm_io.rx_mode;

        /** TODO: layer to node above not connected? Not sure when might receive SB
          * packet from above layer
          */
        _sideband_channel->io.inner.switcher_bundle.layer_to_node_above.assignValid(false);
        _sideband_channel->io.inner.switcher_bundle.layer_to_node_above.assignBits(32, 0);
        _sideband_channel->io.inner.switcher_bundle.node_to_layer_above.assignReady(false);

        assert(afe_params.sb_width == 1);
        io.sb_afe.tx_data = _sideband_channel->io.to_lower_layer.tx.bits;
        // TODO
        // io.sbAfe.txClock <> sidebandChannel.io.to_lower_layer.tx.clock
        // io.sbAfe.rxClock <> sidebandChannel.io.to_lower_layer.rx.clock
        _sideband_channel->io.to_lower_layer.rx.bits = io.sb_afe.rx_data;


        // Lanes module needs reset signal
        _lanes->setReset(getReset());
    }

    void LogicalPhy::calcNextState() {
#ifdef DEBUG_LEVEL_1
        static std::ofstream log_file("logical_phy.txt");
        static int step_counter{0};

        // set hex for the following
        log_file << std::dec;
        log_file << "===================== step " << step_counter << " =====================" << std::endl;
        log_file << std::hex;
        // rdi
        log_file << getPathName() << ": io.rdi.lp_data.isValid " << static_cast<bool>(io.rdi.lp_data.isValid()) << std::endl;
        log_file << getPathName() << ": io.rdi.lp_data.isReady " << static_cast<bool>(io.rdi.lp_data.isReady()) << std::endl;
        log_file << getPathName() << ": io.rdi.lp_data.bits " << io.rdi.lp_data.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.rdi.lp_data_irdy " << static_cast<bool>(io.rdi.lp_data_irdy()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_data.isValid " << static_cast<bool>(io.rdi.pl_data.isValid()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_data.bits " << io.rdi.pl_data.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.rdi.lp_retimer_crd " << static_cast<bool>(io.rdi.lp_retimer_crd()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_retimer_crd " << static_cast<bool>(io.rdi.pl_retimer_crd()) << std::endl;
        log_file << getPathName() << ": io.rdi.lp_state_req " << io.rdi.lp_state_req().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.rdi.lp_link_error " << static_cast<bool>(io.rdi.lp_link_error()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_state_status " << io.rdi.pl_state_status().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.rdi.pl_inband_pres " << static_cast<bool>(io.rdi.pl_inband_pres()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_error " << static_cast<bool>(io.rdi.pl_error()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_correctable_error " << static_cast<bool>(io.rdi.pl_correctable_error()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_non_fatal_error " << static_cast<bool>(io.rdi.pl_non_fatal_error()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_train_error " << static_cast<bool>(io.rdi.pl_train_error()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_phy_in_recenter " << static_cast<bool>(io.rdi.pl_phy_in_recenter()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_stall_req " << static_cast<bool>(io.rdi.pl_stall_req()) << std::endl;
        log_file << getPathName() << ": io.rdi.lp_stall_ack " << static_cast<bool>(io.rdi.lp_stall_ack()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_speed_mode " << io.rdi.pl_speed_mode().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.rdi.pl_link_width " << io.rdi.pl_link_width().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.rdi.pl_clk_req " << static_cast<bool>(io.rdi.pl_clk_req()) << std::endl;
        log_file << getPathName() << ": io.rdi.lp_clk_ack " << static_cast<bool>(io.rdi.lp_clk_ack()) << std::endl;
        log_file << getPathName() << ": io.rdi.lp_wake_req " << static_cast<bool>(io.rdi.lp_wake_req()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_wake_ack " << static_cast<bool>(io.rdi.pl_wake_ack()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_config.isValid " << static_cast<bool>(io.rdi.pl_config.isValid()) << std::endl;
        log_file << getPathName() << ": io.rdi.pl_config.bits " << io.rdi.pl_config.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.rdi.pl_config_credit " << static_cast<bool>(io.rdi.pl_config_credit()) << std::endl;
        log_file << getPathName() << ": io.rdi.lp_config.isValid " << static_cast<bool>(io.rdi.lp_config.isValid()) << std::endl;
        log_file << getPathName() << ": io.rdi.lp_config.bits " << io.rdi.lp_config.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.rdi.lp_config_credit " << static_cast<bool>(io.rdi.lp_config_credit()) << std::endl;


        // debug
        const auto packet_info = PacketInfoManager::GetInstance().peek();
        static std::ofstream rdi_keystone_log_file("logical_phy_keystone.txt");

        // lp
        static int rdi_lp_packet_count = 0;
        static int rdi_lp_tag_count = 0;
        if (io.rdi.lp_data.isValid() && io.rdi.lp_data.isReady()) {
            rdi_keystone_log_file << getPathName() << ": lp_data, clock_step " << step_counter
                << " packet_idx " << std::dec << rdi_lp_packet_count
                << " tag_idx " << std::dec << rdi_lp_tag_count
                << " bits 0x" << io.rdi.lp_data.bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase)
                << std::endl;
            if (packet_info.finished && rdi_lp_tag_count == packet_info.tag_index) {
                rdi_lp_packet_count++;
                rdi_lp_tag_count = 0;
            } else {
                rdi_lp_tag_count++;
            }
        }

        // pl
        static int rdi_pl_packet_count = 0;
        static int rdi_pl_tag_count = 0;
        if (io.rdi.pl_data.isValid()) {
            rdi_keystone_log_file << getPathName() << ": pl_data, clock_step " << step_counter
                << " packet_idx " << std::dec << rdi_pl_packet_count
                << " tag_idx " << std::dec << rdi_pl_tag_count
                << " bits 0x" << io.rdi.pl_data.bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase)
                << std::endl;
            if (packet_info.finished && rdi_pl_tag_count == packet_info.tag_index) {
                rdi_pl_packet_count++;
                rdi_pl_tag_count = 0;
            } else {
                rdi_pl_tag_count++;
            }
        }

        step_counter++;
#endif // DEBUG_LEVEL_1
    }

    bool LogicalPhy::propagateClock() {
        //std::cout << getPathName() << " propagateClock" << std::endl;
        bool success = true;

        if (io.sb_afe.rx_clock != nullptr) {
            _sideband_channel->io.to_lower_layer.rx.clock = io.sb_afe.rx_clock;
        } else {
            success = false;
        }

        if (io.mb_afe.fifo_params.clk != nullptr) {
            _lanes->io.mainband_io.fifo_params.clk = io.mb_afe.fifo_params.clk;
        } else {
            success = false;
        }

        success &= Module::propagateClock();

        if (_sideband_channel->io.to_lower_layer.tx.clock != nullptr) {
            io.sb_afe.tx_clock = _sideband_channel->io.to_lower_layer.tx.clock;
        } else {
            success = false;
        }
        return success;
    }
} // namespace CCPS