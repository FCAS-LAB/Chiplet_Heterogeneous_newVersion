#include "e2e/ccps_top.hpp"

namespace CCPS {

    CCPSTop::CCPSTop(const FdiParams &fdi_params, const RdiParams &rdi_params,
                   const SidebandParams &sb_params, BigUInt my_id,
                   const LinkTrainingParams &link_training_params, const AfeParams &afe_params,
                   const AsyncQueueParams &lane_async_queue_params) {
        // Instantiate
        _protocol = createSubmodule<ProtocolLayer>("protocol", fdi_params);
        _d2dadapter = createSubmodule<D2DAdapter>("d2dadapter", fdi_params, rdi_params, sb_params);
        _logphy = createSubmodule<LogicalPhy>("logphy", static_cast<int>(my_id), link_training_params,
        afe_params, rdi_params, fdi_params, sb_params, lane_async_queue_params);

        // Connect
        // Connect the FDI interface of Protocol layer to D2D adapter
        _d2dadapter->io.fdi.lp_data.connect(_protocol->io.fdi.lp_data);
        _d2dadapter->io.fdi.lp_data_irdy = _protocol->io.fdi.lp_data_irdy;
        _protocol->io.fdi.pl_data.connect(_d2dadapter->io.fdi.pl_data);
        _d2dadapter->io.fdi.lp_retimer_crd = _protocol->io.fdi.lp_retimer_crd;
        _d2dadapter->io.fdi.lp_corrupt_crc = _protocol->io.fdi.lp_corrupt_crc;
        _d2dadapter->io.fdi.lp_stream.connect(_protocol->io.fdi.lp_stream);
        _protocol->io.fdi.pl_retimer_crd = _d2dadapter->io.fdi.pl_retimer_crd;
        _protocol->io.fdi.pl_dllp.connect(_d2dadapter->io.fdi.pl_dllp);
        _protocol->io.fdi.pl_dllp_ofc = _d2dadapter->io.fdi.pl_dllp_ofc;
        _protocol->io.fdi.pl_stream.connect(_d2dadapter->io.fdi.pl_stream);
        _protocol->io.fdi.pl_flit_cancel = _d2dadapter->io.fdi.pl_flit_cancel;
        _d2dadapter->io.fdi.lp_state_req = _protocol->io.fdi.lp_state_req;
        _d2dadapter->io.fdi.lp_link_error = _protocol->io.fdi.lp_link_error;
        _protocol->io.fdi.pl_state_status = _d2dadapter->io.fdi.pl_state_status;
        _protocol->io.fdi.pl_inband_pres = _d2dadapter->io.fdi.pl_inband_pres;
        _protocol->io.fdi.pl_error = _d2dadapter->io.fdi.pl_error;
        _protocol->io.fdi.pl_cerror = _d2dadapter->io.fdi.pl_cerror;
        _protocol->io.fdi.pl_nf_error = _d2dadapter->io.fdi.pl_nf_error;
        _protocol->io.fdi.pl_train_error = _d2dadapter->io.fdi.pl_train_error;
        _protocol->io.fdi.pl_rx_active_req = _d2dadapter->io.fdi.pl_rx_active_req;
        _d2dadapter->io.fdi.lp_rx_active_status = _protocol->io.fdi.lp_rx_active_status;
        _protocol->io.fdi.pl_protocol = _d2dadapter->io.fdi.pl_protocol;
        _protocol->io.fdi.pl_protocol_flit_format = _d2dadapter->io.fdi.pl_protocol_flit_format;
        _protocol->io.fdi.pl_protocol_valid = _d2dadapter->io.fdi.pl_protocol_valid;
        _protocol->io.fdi.pl_stall_req = _d2dadapter->io.fdi.pl_stall_req;
        _d2dadapter->io.fdi.lp_stall_ack = _protocol->io.fdi.lp_stall_ack;
        _protocol->io.fdi.pl_phy_in_recenter = _d2dadapter->io.fdi.pl_phy_in_recenter;
        _protocol->io.fdi.pl_phy_in_l1 = _d2dadapter->io.fdi.pl_phy_in_l1;
        _protocol->io.fdi.pl_phy_in_l2 = _d2dadapter->io.fdi.pl_phy_in_l2;
        _protocol->io.fdi.pl_speed_mode = _d2dadapter->io.fdi.pl_speed_mode;
        _protocol->io.fdi.pl_link_width = _d2dadapter->io.fdi.pl_link_width;
        _protocol->io.fdi.pl_clk_req = _d2dadapter->io.fdi.pl_clk_req;
        _d2dadapter->io.fdi.lp_clk_ack = _protocol->io.fdi.lp_clk_ack;
        _d2dadapter->io.fdi.lp_wake_req = _protocol->io.fdi.lp_wake_req;
        _protocol->io.fdi.pl_wake_ack = _d2dadapter->io.fdi.pl_wake_ack;
        //_protocol->io.fdi.pl_config.connect(_d2dadapter->io.fdi.pl_config);
        _d2dadapter->io.fdi.pl_config_credit = _protocol->io.fdi.pl_config_credit;
        _d2dadapter->io.fdi.lp_config.connect(_protocol->io.fdi.lp_config);
        //_protocol->io.fdi.lp_config_credit = _d2dadapter->io.fdi.lp_config_credit;

        // Connect the RDI interface of D2D adapter to logPhy
        _logphy->io.rdi.connect(_d2dadapter->io.rdi);

        // Connect the AFE interface from logPhy to the top
        io.mb_afe.connect(_logphy->io.mb_afe);
        io.sb_afe.connect(_logphy->io.sb_afe);

        // Connect the protocol IOs to the top for connections to the tilelink interface
        io.fdi_lp_config.connect(_protocol->io.fdi.lp_config);
        _protocol->io.fdi.lp_config_credit = io.fdi_lp_config_credit;
        _protocol->io.fdi.pl_config.connect(io.fdi_pl_config);
        io.fdi_pl_config_credit = _protocol->io.fdi.pl_config_credit;
        io.fdi_lp_stall_ack = _protocol->io.fdi.lp_stall_ack;
        io.tl_pl_state_status = _protocol->io.tl_pl_state_status;

        _protocol->io.tl_lp_data_valid = io.tl_lp_data_valid;
        _protocol->io.tl_lp_data_bits = io.tl_lp_data_bits;
        _protocol->io.tl_lp_data_irdy = io.tl_lp_data_irdy;
        io.tl_lp_data_ready = _protocol->io.tl_lp_data_ready;
        io.tl_pl_data_bits = _protocol->io.tl_pl_data_bits;
        io.tl_pl_data_valid = _protocol->io.tl_pl_data_valid;
        _protocol->io.tl_ready_to_rcv = io.tl_ready_to_rcv;
        _protocol->io.fault = io.fault;
        _protocol->io.soft_reset = io.soft_reset;

        _logphy->setReset(getReset());
    }

    bool CCPSTop::propagateClock() {
        bool success = true;

        if (io.mb_afe.fifo_params.clk != nullptr) {
            _logphy->io.mb_afe.fifo_params.clk = io.mb_afe.fifo_params.clk;
        } else {
            success = false;
        }

        if (io.sb_afe.fifo_params.clk != nullptr) {
            _logphy->io.sb_afe.fifo_params.clk = io.sb_afe.fifo_params.clk;
        } else {
            success = false;
        }

        if (io.sb_afe.rx_clock != nullptr) {
            _logphy->io.sb_afe.rx_clock = io.sb_afe.rx_clock;
        } else {
            success = false;
        }

        success &= Module::propagateClock();

        if (_logphy->io.sb_afe.tx_clock != nullptr) {
            io.sb_afe.tx_clock = _logphy->io.sb_afe.tx_clock;
        } else {
            success = false;
        }

        return success;
    }

    void CCPSTop::calcNextState() {
        //std::cout << "============== " << getPathName() << " =================" << std::endl;
        //std::cout << getPathName() << ": io.fdi_lp_stall_ack " << static_cast<bool>(io.fdi_lp_stall_ack()) << std::endl;
        //std::cout << getPathName() << ": io.tl_pl_state_status " << io.tl_pl_state_status().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.tl_lp_data_valid " << static_cast<bool>(io.tl_lp_data_valid()) << std::endl;
        //std::cout << getPathName() << ": io.tl_lp_data_bits 0x" << io.tl_lp_data_bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase) << std::endl;
        //std::cout << getPathName() << ": io.tl_lp_data_irdy " << static_cast<bool>(io.tl_lp_data_irdy()) << std::endl;
        //std::cout << getPathName() << ": io.tl_lp_data_ready " << static_cast<bool>(io.tl_lp_data_ready()) << std::endl;
        //std::cout << getPathName() << ": io.tl_pl_data_bits 0x" << io.tl_pl_data_bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase) << std::endl;
        //std::cout << getPathName() << ": io.tl_pl_data_valid " << static_cast<bool>(io.tl_pl_data_valid()) << std::endl;
        //std::cout << getPathName() << ": io.tl_ready_to_rcv " << static_cast<bool>(io.tl_ready_to_rcv()) << std::endl;
        //std::cout << getPathName() << ": io.fault " << static_cast<bool>(io.fault()) << std::endl;
        //std::cout << getPathName() << ": io.soft_reset " << static_cast<bool>(io.soft_reset()) << std::endl;
    }

} // namespace CCPS