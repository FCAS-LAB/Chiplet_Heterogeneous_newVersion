#ifndef __RDI_HPP__
#define __RDI_HPP__

namespace CCPS {

    struct RdiParams {
        int width = 64;
        int sb_width = 32;

        RdiParams(const int w = 64, const int sb_w = 32):
            width(w), sb_width(sb_w) {}
    };
    struct Rdi{
        Decoupled<UInt> lp_data;            // 8 * params.width
        Wire<Bool> lp_data_irdy;    // O
        Valid<UInt> pl_data{true};          // 8 * params.width
        Wire<Bool> lp_retimer_crd;  // O
        Wire<Bool> pl_retimer_crd;  // I
        Wire<UInt> lp_state_req; // O       // 4
        Wire<Bool> lp_link_error;   // O
        Wire<UInt> pl_state_status;   // I   // 4
        Wire<Bool> pl_inband_pres; // I
        Wire<Bool> pl_error;       // I
        Wire<Bool> pl_correctable_error; // I
        Wire<Bool> pl_non_fatal_error; // I
        Wire<Bool> pl_train_error; // I
        Wire<Bool> pl_phy_in_recenter; // I
        Wire<Bool> pl_stall_req;  // I
        Wire<Bool> lp_stall_ack;  // O
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

        void connect(Rdi &other) {
            lp_data.connect(other.lp_data);
            lp_data_irdy = other.lp_data_irdy;
            other.pl_data.connect(pl_data);
            lp_retimer_crd = other.lp_retimer_crd;
            other.pl_retimer_crd = pl_retimer_crd;
            lp_state_req = other.lp_state_req;
            lp_link_error = other.lp_link_error;
            other.pl_state_status = pl_state_status;
            other.pl_inband_pres = pl_inband_pres;
            other.pl_error = pl_error;
            other.pl_correctable_error = pl_correctable_error;
            other.pl_non_fatal_error = pl_non_fatal_error;
            other.pl_train_error = pl_train_error;
            other.pl_phy_in_recenter = pl_phy_in_recenter;
            other.pl_stall_req = pl_stall_req;
            lp_stall_ack = other.lp_stall_ack;
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


} // namespace CCPS

#endif // __RDI_HPP__