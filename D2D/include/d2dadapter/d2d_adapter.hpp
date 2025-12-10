#ifndef __D2D_ADAPTER_HPP__
#define __D2D_ADAPTER_HPP__

#include "interfaces/fdi.hpp"
#include "interfaces/rdi.hpp"
#include "utils/wire.hpp"
#include "utils/module.hpp"
#include "d2dadapter/link_management_controller.hpp"
#include "d2dadapter/stall_handler.hpp"
#include "d2dadapter/d2d_sideband_module.hpp"
#include "d2dadapter/d2d_mainband_module.hpp"
#include "d2dadapter/parity_generator.hpp"


namespace CCPS {

    struct D2DAdapterIO
    {
        Fdi fdi{true};
        Rdi rdi;
    };

    class D2DAdapter : public RegModule
    {
    public:
        D2DAdapterIO io;
        D2DAdapter(const FdiParams &fdi_params, const RdiParams &rdi_params, const SidebandParams &sb_params);
        void calcNextState() override;

    private:
        // ================ chisel signals =================
        ModulePtr<LinkManagementController> _link_manager;
        ModulePtr<FDIStallHandler> _fdi_stall_handler;
        ModulePtr<RDIStallHandler> _rdi_stall_handler;
        ModulePtr<D2DSidebandModule> _d2d_sideband;
        ModulePtr<D2DMainbandModule> _d2d_mainband;
        ModulePtr<ParityGenerator> _parity_generator;

        // ================ helper signals =================
        FdiParams _fdi_params;
    };


} // namespace CCPS


#endif // __D2D_ADAPTER_HPP__