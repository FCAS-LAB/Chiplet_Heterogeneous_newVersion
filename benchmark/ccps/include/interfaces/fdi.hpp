#ifndef __FDI_HPP__
#define __FDI_HPP__

#include "utils/decoupled.hpp"
#include "utils/base_types.hpp"
#include "interfaces/types.hpp"

namespace CCPS {

    class FdiParams {
    public:
        FdiParams(const int w=64, const int dw=128, const int sbw=128):
            width(w), dllp_width(dw), sb_width(sbw) {}
        ~FdiParams() = default;

        int width;
        int dllp_width;
        int sb_width;
    };

    struct Fdi {
        Fdi(bool do_flip = false) {
            if (do_flip) {
                flip();
            }
        }

        void flip() {
            lp_data.flip();
            pl_data.flip();
            pl_dllp.flip();
            pl_config.flip();
            lp_config.flip();
        }

        Decoupled<UInt> lp_data;            // 8 * params.width
        Wire<Bool> lp_data_irdy;        // O
        Valid<UInt> pl_data{true};          // 8 * params.width
        Wire<Bool> lp_retimer_crd;  // O
        Wire<Bool> lp_corrupt_crc;  // O
        Valid<UInt> lp_dllp;                // params.dllpWidth
        Wire<Bool> lp_dllp_ofc;     // O
        ProtoStream lp_stream; // O
        Wire<Bool> pl_retimer_crd;  // I
        Valid<UInt> pl_dllp{true};          // params.dllpWidth
        Wire<Bool> pl_dllp_ofc;     // I
        ProtoStream pl_stream; // I
        Wire<Bool> pl_flit_cancel;  // I
        Wire<UInt> lp_state_req; // O       // 4
        Wire<Bool> lp_link_error;   // O
        Wire<UInt> pl_state_status;   // I   // 4
        Wire<Bool> pl_inband_pres; // I
        Wire<Bool> pl_error;       // I
        Wire<Bool> pl_cerror;     // I
        Wire<Bool> pl_nf_error;    // I
        Wire<Bool> pl_train_error; // I
        Wire<Bool> pl_rx_active_req; // I
        Wire<Bool> lp_rx_active_status; // O
        Wire<UInt> pl_protocol;   // I          // 3
        Wire<UInt> pl_protocol_flit_format; // I    // 4
        Wire<Bool> pl_protocol_valid; // I
        Wire<Bool> pl_stall_req;  // I
        Wire<Bool> lp_stall_ack;  // O
        Wire<Bool> pl_phy_in_recenter; // I
        Wire<Bool> pl_phy_in_l1;  // I
        Wire<Bool> pl_phy_in_l2;  // I
        Wire<UInt> pl_speed_mode; // I      // 3
        Wire<UInt> pl_link_width; // I      // 3
        Wire<Bool> pl_clk_req;    // I
        Wire<Bool> lp_clk_ack;    // O
        Wire<Bool> lp_wake_req;   // O
        Wire<Bool> pl_wake_ack;   // I
        Valid<UInt> pl_config{true};    //  params.sbWidth
        Wire<Bool> pl_config_credit; // O
        Valid<UInt> lp_config;          // params.sbWidth
        Wire<Bool> lp_config_credit; // I

        void connect(Fdi &other) {
            lp_data.connect(other.lp_data);
            lp_data_irdy = other.lp_data_irdy;
            other.pl_data.connect(pl_data);
            lp_retimer_crd = other.lp_retimer_crd;
            lp_corrupt_crc = other.lp_corrupt_crc;
            lp_stream.connect(other.lp_stream);
            other.pl_retimer_crd = pl_retimer_crd;
            other.pl_dllp.connect(pl_dllp);
            other.pl_dllp_ofc = pl_dllp_ofc;
            other.pl_stream.connect(pl_stream);
            other.pl_flit_cancel = pl_flit_cancel;
            lp_state_req = other.lp_state_req;
            lp_link_error = other.lp_link_error;
            other.pl_state_status = pl_state_status;
            other.pl_inband_pres = pl_inband_pres;
            other.pl_error = pl_error;
            other.pl_cerror = pl_cerror;
            other.pl_nf_error = pl_nf_error;
            other.pl_train_error = pl_train_error;
            other.pl_rx_active_req = pl_rx_active_req;
            lp_rx_active_status = other.lp_rx_active_status;
            other.pl_protocol = pl_protocol;
            other.pl_protocol_flit_format = pl_protocol_flit_format;
            other.pl_protocol_valid = pl_protocol_valid;
            other.pl_stall_req = pl_stall_req;
            lp_stall_ack = other.lp_stall_ack;
            other.pl_phy_in_recenter = pl_phy_in_recenter;
            other.pl_phy_in_l1 = pl_phy_in_l1;
            other.pl_phy_in_l2 = pl_phy_in_l2;
            other.pl_speed_mode = pl_speed_mode;
            other.pl_link_width = pl_link_width;
            other.pl_clk_req = pl_clk_req;
            lp_clk_ack = other.lp_clk_ack;
            lp_wake_req = other.lp_wake_req;
            other.pl_wake_ack = pl_wake_ack;
            other.pl_config.connect(pl_config);
            pl_config_credit = other.pl_config_credit;
            lp_config.connect(other.lp_config);
            other.lp_config_credit = lp_config_credit;
        }
    };
}

#endif // __FDI_HPP__