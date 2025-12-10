#include "d2dadapter/d2d_adapter.hpp"
#include <fstream>

namespace CCPS {

    D2DAdapter::D2DAdapter(const FdiParams &fdi_params, const RdiParams &rdi_params, const SidebandParams &sb_params):
        _fdi_params(fdi_params)
    {
        // Instantiate
        _link_manager = createSubmodule<LinkManagementController>("link_manager", fdi_params, rdi_params, sb_params);
        _fdi_stall_handler = createSubmodule<FDIStallHandler>("fdi_stall_handler");
        _rdi_stall_handler = createSubmodule<RDIStallHandler>("rdi_stall_handler");
        _d2d_sideband = createSubmodule<D2DSidebandModule>("d2d_sideband", fdi_params, sb_params);
        _d2d_mainband = createSubmodule<D2DMainbandModule>("d2d_mainband", fdi_params, rdi_params, sb_params);
        _parity_generator = createSubmodule<ParityGenerator>("parity_generator", fdi_params);

        // connect
        io.fdi.pl_protocol_valid.capture(true);
        io.fdi.pl_protocol_flit_format.capture(4, FlitFormat::raw);
        io.fdi.pl_protocol.capture(3, Protocol::streaming);
        io.fdi.pl_speed_mode = io.rdi.pl_speed_mode;
        io.fdi.pl_link_width = io.rdi.pl_link_width;
        io.fdi.pl_flit_cancel.capture(false);

        io.fdi.pl_nf_error.capture(false);
        io.fdi.pl_train_error.capture(false);
        io.fdi.pl_error.capture(false);
        io.fdi.pl_cerror.capture(false);

        io.fdi.pl_phy_in_recenter.capture(false);
        io.fdi.pl_phy_in_l1.capture(false);
        io.fdi.pl_phy_in_l2.capture(false);
        io.fdi.pl_dllp.assignBits(fdi_params.dllp_width, BigUInt(0));
        io.fdi.pl_dllp.assignValid(false);
        io.fdi.pl_dllp_ofc.capture(false);

        io.fdi.pl_clk_req.capture(true);
        io.rdi.lp_clk_ack.capture(true);
        io.fdi.pl_wake_ack.capture(true);

        io.fdi.pl_retimer_crd.capture(false);

        io.rdi.lp_retimer_crd.capture(false);
        io.rdi.lp_wake_req.capture(true);

        // link management controller
        // FDI interface
        _link_manager->io.fdi_lp_state_req = io.fdi.lp_state_req;
        _link_manager->io.fdi_lp_linkerror = io.fdi.lp_link_error;
        _link_manager->io.fdi_lp_rx_active_sts = io.fdi.lp_rx_active_status;
        io.fdi.pl_state_status = _link_manager->io.fdi_pl_state_sts;
        io.fdi.pl_rx_active_req = _link_manager->io.fdi_pl_rx_active_req;
        io.fdi.pl_inband_pres = _link_manager->io.fdi_pl_inband_pres;
        // RDI interface
        io.rdi.lp_link_error = _link_manager->io.rdi_lp_linkerror;
        io.rdi.lp_state_req = _link_manager->io.rdi_lp_state_req;
        _link_manager->io.rdi_pl_state_sts = io.rdi.pl_state_status;
        _link_manager->io.rdi_pl_inband_pres = io.rdi.pl_inband_pres;

        // link manager <-> D2D sideband
        _d2d_sideband->io.sideband_snt = _link_manager->io.sb_snd;
        _link_manager->io.sb_rcv = _d2d_sideband->io.sideband_rcv;
        _link_manager->io.sb_rdy = _d2d_sideband->io.sideband_rdy;

        // stall handler <-> LinkManagementController
        _link_manager->io.linkmgmt_stalldone = _fdi_stall_handler->io.linkmgmt_stalldone;
        _fdi_stall_handler->io.linkmgmt_stallreq = _link_manager->io.linkmgmt_stallreq;

        // TODO: should move this to a MMIO register
        _link_manager->io.cycles_1us.capture(10, BigUInt(1000));

        // parity generator <-> link manager
        _link_manager->io.parity_tx_sw_en.capture(false);
        _link_manager->io.parity_rx_sw_en.capture(false);
        _parity_generator->io.parity_rx_enable = _link_manager->io.parity_rx_enable;
        _parity_generator->io.parity_tx_enable = _link_manager->io.parity_tx_enable;

        // Sideband
        io.fdi.pl_config.assignBits(_d2d_sideband->io.fdi_pl_cfg);
        io.fdi.pl_config.assignValid(_d2d_sideband->io.fdi_pl_cfg_vld);
        _d2d_sideband->io.fdi_pl_cfg_crd = io.fdi.pl_config_credit;
        _d2d_sideband->io.fdi_lp_cfg = [this]() -> UInt {
            return io.fdi.lp_config.bits();
        };
        _d2d_sideband->io.fdi_lp_cfg_vld = [this]() -> Bool {
            return io.fdi.lp_config.isValid();
        };
        io.fdi.lp_config_credit = _d2d_sideband->io.fdi_lp_cfg_crd;

        _d2d_sideband->io.rdi_pl_cfg = [this]() -> UInt {
            return io.rdi.pl_config.bits();
        };
        _d2d_sideband->io.rdi_pl_cfg_vld = [this]() -> Bool {
            return io.rdi.pl_config.isValid();
        };
        io.rdi.pl_config_credit = _d2d_sideband->io.rdi_pl_cfg_crd;
        io.rdi.lp_config.assignBits(
            [this]() -> UInt {
                return _d2d_sideband->io.rdi_lp_cfg();
            }
        );
        io.rdi.lp_config.assignValid(
            [this]() -> Bool {
                return _d2d_sideband->io.rdi_lp_cfg_vld();
            }
        );
        _d2d_sideband->io.rdi_lp_cfg_crd = io.rdi.lp_config_credit;

        // stall handler
        io.fdi.pl_stall_req = _fdi_stall_handler->io.fdi_pl_stallreq;
        _fdi_stall_handler->io.fdi_lp_stallack = io.fdi.lp_stall_ack;

        _rdi_stall_handler->io.rdi_pl_stallreq = io.rdi.pl_stall_req;
        io.rdi.lp_stall_ack = _rdi_stall_handler->io.rdi_lp_stallack;

        // mainband module
        // FDI
        _d2d_mainband->io.fdi_lp_irdy = io.fdi.lp_data_irdy;
        _d2d_mainband->io.fdi_lp_valid = [this] () -> Bool {
            return io.fdi.lp_data.isValid();
        };
        _d2d_mainband->io.fdi_lp_data = [this] () -> UInt {
            return io.fdi.lp_data.bits();
        };
        _d2d_mainband->io.fdi_lp_stream.proto_stack = io.fdi.lp_stream.proto_stack;
        _d2d_mainband->io.fdi_lp_stream.proto_type = io.fdi.lp_stream.proto_type;
        io.fdi.lp_data.assignReady(
            [this]() -> Bool {
                return _d2d_mainband->io.fdi_pl_trdy();
            }
        );
        io.fdi.pl_data.assignValid(
            [this]() -> Bool {
                return _d2d_mainband->io.fdi_pl_valid();
            }
        );
        io.fdi.pl_data.assignBits(
            [this]() -> UInt {
                return _d2d_mainband->io.fdi_pl_data();
            }
        );
        io.fdi.pl_stream.proto_stack = _d2d_mainband->io.fdi_pl_stream.proto_stack;
        io.fdi.pl_stream.proto_type = _d2d_mainband->io.fdi_pl_stream.proto_type;
        // RDI
        io.rdi.lp_data_irdy = _d2d_mainband->io.rdi_lp_irdy;
        io.rdi.lp_data.assignValid(
            [this]() -> Bool {
                return _d2d_mainband->io.rdi_lp_valid();
            }
        );
        io.rdi.lp_data.assignBits(
            [this]() -> UInt {
                return _d2d_mainband->io.rdi_lp_data();
            }
        );
        _d2d_mainband->io.rdi_pl_trdy = [this]() -> Bool {
            return io.rdi.lp_data.isReady();
        };
        _d2d_mainband->io.rdi_pl_valid = [this]() -> Bool {
            return io.rdi.pl_data.isValid();
        };
        _d2d_mainband->io.rdi_pl_data = [this]() -> UInt {
            return io.rdi.pl_data.bits();
        };

        _d2d_mainband->io.d2d_state = _link_manager->io.fdi_pl_state_sts;

        // stall handler <-> mainband
        _d2d_mainband->io.mainband_stallreq = _rdi_stall_handler->io.mainband_stallreq;
        _rdi_stall_handler->io.mainband_stalldone = _d2d_mainband->io.mainband_stalldone;

        // parity generator <-> mainband
        //(Bits((8 * fdiParams.width).W))
        for (size_t i = 0; i < _parity_generator->io.snd_data.size(); i++) {
            _parity_generator->io.snd_data[i] = [this, i]() -> UInt {
                return _d2d_mainband->io.snd_data()(_fdi_params.width * i + 7, _fdi_params.width * i);
            };
        }
        _parity_generator->io.snd_data_vld = _d2d_mainband->io.snd_data_vld;
        for (size_t i = 0; i < _parity_generator->io.rcv_data.size(); i++) {
            _parity_generator->io.rcv_data[i] = [this, i]() -> UInt {
                return _d2d_mainband->io.rcv_data()(_fdi_params.width * i + 7, _fdi_params.width * i);
            };
        }
        _parity_generator->io.rcv_data_vld = _d2d_mainband->io.rcv_data_vld;
        _d2d_mainband->io.parity_insert = _parity_generator->io.parity_insert;
        _d2d_mainband->io.parity_data = [this]() -> UInt {
            UInt res;
            for (size_t i = 0; i < _fdi_params.width; i++) {
                res.append(
                    _parity_generator->io.parity_data[_fdi_params.width - i - 1]()
                );
            }
            return res;
        };
        _parity_generator->io.parity_rdy = _d2d_mainband->io.parity_rdy;
        _d2d_mainband->io.parity_check = _parity_generator->io.parity_check;

        // Parity generator submodule other IOs
        _parity_generator->io.parity_n.capture(ParityGeneratorWidth::PARITY_N_WIDTH, ParityN::ONE);
        _parity_generator->io.rdi_state = io.rdi.pl_state_status;
    }

    void D2DAdapter::calcNextState() {
#ifdef DEBUG_LEVEL_1
        static std::ofstream log_file("d2d_adapter.txt");
        static int step_counter{0};

        // set hex for the following
        log_file << std::dec;
        log_file << "===================== step " << step_counter << " =====================" << std::endl;
        log_file << std::hex;

        // fdi
        log_file << getPathName() << ": io.fdi.lp_data.isValid " << static_cast<bool>(io.fdi.lp_data.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_data.isReady " << static_cast<bool>(io.fdi.lp_data.isReady()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_data.bits " << io.fdi.lp_data.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.lp_data_irdy " << static_cast<bool>(io.fdi.lp_data_irdy()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_data.isValid " << static_cast<bool>(io.fdi.pl_data.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_data.bits " << io.fdi.pl_data.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.lp_retimer_crd " << static_cast<bool>(io.fdi.lp_retimer_crd()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_corrupt_crc " << static_cast<bool>(io.fdi.lp_corrupt_crc()) << std::endl;
        //log_file << getPathName() << ": io.fdi.lp_dllp.isValid " << static_cast<bool>(io.fdi.lp_dllp.isValid()) << std::endl;
        //log_file << getPathName() << ": io.fdi.lp_dllp.bits " << io.fdi.lp_dllp.bits().toBigUInt() << std::endl;
        //log_file << getPathName() << ": io.fdi.lp_dllp_ofc " << static_cast<bool>(io.fdi.lp_dllp_ofc()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_retimer_crd " << static_cast<bool>(io.fdi.pl_retimer_crd()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_dllp.isValid " << static_cast<bool>(io.fdi.pl_dllp.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_dllp.bits " << io.fdi.pl_dllp.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_dllp_ofc " << static_cast<bool>(io.fdi.pl_dllp_ofc()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_stream.proto_stack " << io.fdi.pl_stream.proto_stack().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_stream.proto_type " << io.fdi.pl_stream.proto_type().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_flit_cancel " << static_cast<bool>(io.fdi.pl_flit_cancel()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_state_req " << io.fdi.lp_state_req().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.lp_link_error " << static_cast<bool>(io.fdi.lp_link_error()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_state_status " << io.fdi.pl_state_status().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_inband_pres " << static_cast<bool>(io.fdi.pl_inband_pres()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_error " << static_cast<bool>(io.fdi.pl_error()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_cerror " << static_cast<bool>(io.fdi.pl_cerror()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_nf_error " << static_cast<bool>(io.fdi.pl_nf_error()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_train_error " << static_cast<bool>(io.fdi.pl_train_error()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_rx_active_req " << static_cast<bool>(io.fdi.pl_rx_active_req()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_rx_active_status " << static_cast<bool>(io.fdi.lp_rx_active_status()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_protocol " << io.fdi.pl_protocol().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_protocol_flit_format " << io.fdi.pl_protocol_flit_format().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_protocol_valid " << static_cast<bool>(io.fdi.pl_protocol_valid()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_stall_req " << static_cast<bool>(io.fdi.pl_stall_req()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_stall_ack " << static_cast<bool>(io.fdi.lp_stall_ack()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_phy_in_recenter " << static_cast<bool>(io.fdi.pl_phy_in_recenter()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_phy_in_l1 " << static_cast<bool>(io.fdi.pl_phy_in_l1()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_phy_in_l2 " << static_cast<bool>(io.fdi.pl_phy_in_l2()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_speed_mode " << io.fdi.pl_speed_mode().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_link_width " << io.fdi.pl_link_width().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_clk_req " << static_cast<bool>(io.fdi.pl_clk_req()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_clk_ack " << static_cast<bool>(io.fdi.lp_clk_ack()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_wake_req " << static_cast<bool>(io.fdi.lp_wake_req()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_wake_ack " << static_cast<bool>(io.fdi.pl_wake_ack()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_config.isValid " << static_cast<bool>(io.fdi.pl_config.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.pl_config.bits " << io.fdi.pl_config.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.pl_config_credit " << static_cast<bool>(io.fdi.pl_config_credit()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_config.isValid " << static_cast<bool>(io.fdi.lp_config.isValid()) << std::endl;
        log_file << getPathName() << ": io.fdi.lp_config.bits " << io.fdi.lp_config.bits().toBigUInt() << std::endl;
        log_file << getPathName() << ": io.fdi.lp_config_credit " << static_cast<bool>(io.fdi.lp_config_credit()) << std::endl;

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

        //PRINT(io.rdi.pl_config_credit);
        log_file << std::endl;

        // debug fdi
        const auto packet_info = PacketInfoManager::GetInstance().peek();
        static std::ofstream keystone_log_file("d2d_adapter_fdi_keystone.txt");

        // lp
        static int lp_packet_count = 0;
        static int lp_tag_count = 0;
        if (io.fdi.lp_data.isValid() && io.fdi.lp_data.isReady()) {
            keystone_log_file << getPathName() << ": lp_data, clock_step " << step_counter
                << " packet_idx " << std::dec << lp_packet_count
                << " tag_idx " << std::dec << lp_tag_count
                << " bits 0x" << io.fdi.lp_data.bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase)
                << std::endl;
            if (packet_info.finished && lp_tag_count == packet_info.tag_index) {
                lp_packet_count++;
                lp_tag_count = 0;
            } else {
                lp_tag_count++;
            }
        }

        // pl
        static int pl_packet_count = 0;
        static int pl_tag_count = 0;
        if (io.fdi.pl_data.isValid()) {
            keystone_log_file << getPathName() << ": pl_data, clock_step " << step_counter
                << " packet_idx " << std::dec << pl_packet_count
                << " tag_idx " << std::dec << pl_tag_count
                << " bits 0x" << io.fdi.pl_data.bits().toBigUInt().str(0, std::ios_base::hex | std::ios_base::uppercase)
                << std::endl;
            if (packet_info.finished && pl_tag_count == packet_info.tag_index) {
                pl_packet_count++;
                pl_tag_count = 0;
            } else {
                pl_tag_count++;
            }
        }


        // debug rdi
        static std::ofstream rdi_keystone_log_file("d2d_adapter_rdi_keystone.txt");

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
} // namespace CCPS