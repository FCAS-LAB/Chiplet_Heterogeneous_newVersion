#include "logphy/rdi_data_mapper.hpp"

namespace CCPS {

    RdiDataMapperIO::RdiDataMapperIO(bool do_flip) {
        if (do_flip) {
            flip();
        }
    }

    void RdiDataMapperIO::flip() {
        lp_data.flip();
        pl_data.flip();
    }

    RdiDataMapper::RdiDataMapper(const RdiParams &rdi_params, const AfeParams &afe_params) {
        assert(afe_params.mb_serializer_ratio * afe_params.mb_lanes < rdi_params.width * 8);

        _afe_bits = (afe_params.mb_serializer_ratio * afe_params.mb_lanes);
        _ratio = (rdi_params.width * 8) / _afe_bits;

        // Instantiate
        _rx_slice_counter = createReg<UInt>(UInt(log2Ceil(_ratio), 0));
        _rx_data.resize(_ratio);
        for (auto &data : _rx_data) {
            data = createReg<UInt>(UInt(_afe_bits, 0));
        }
        _has_rx_data = createReg<Bool>(Bool(false));
        _tx_width_coupler = createSubmodule<DataWidthCoupler>("tx_width_coupler",
            DataWidthCouplerParams(rdi_params.width * 8, _afe_bits));

        // connect
        io.rdi.pl_data.assignValid(
            [this]() -> Bool {
                return _has_rx_data->read();
            }
        );
        io.rdi.pl_data.assignBits(
            [this]() -> UInt {
                UInt res;
                for (size_t i = 0; i < _rx_data.size(); i++) {
                    res.append(_rx_data[_ratio - 1 - i]->read());
                }
                return res;
            }
        );

        io.mainband_lane_io.tx_data.connect(_tx_width_coupler->io.out);

        io.rdi.lp_data.assignReady(
            [this]() -> Bool {
                return _tx_width_coupler->io.in.isReady();
            }
        );
        _tx_width_coupler->io.in.assignValid(
            [this]() -> Bool {
                return io.rdi.lp_data.isValid() && io.rdi.lp_data_irdy();
            }
        );
        _tx_width_coupler->io.in.assignBits(
            [this]() -> UInt {
                return io.rdi.lp_data.bits();
            }
        );
    }

    void RdiDataMapper::calcNextState() {
        *_has_rx_data = Bool(false);

        if (io.mainband_lane_io.rx_data.isValid()) {
            // chunk
            int rx_slice_counter = static_cast<int>(_rx_slice_counter->read().toBigUInt());
            *(_rx_data[_ratio - 1 - rx_slice_counter]) = io.mainband_lane_io.rx_data.bits();
            *_rx_slice_counter = UInt(log2Ceil(_ratio), rx_slice_counter + 1);
            if (rx_slice_counter == (_ratio - 1)) {
                *_has_rx_data = Bool(true);
                *_rx_slice_counter = UInt(log2Ceil(_ratio), 0);
            }
        }

        //std::cout << getPathName() << ": io.mainband_lane_io.rx_data.valid " << static_cast<bool>(io.mainband_lane_io.rx_data.isValid()) << std::endl;
        //std::cout << getPathName() << ": io.mainband_lane_io.rx_data.data " << std::hex << io.mainband_lane_io.rx_data.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": rx_slice_counter " << _rx_slice_counter->read().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.rdi.pl_data.valid " << static_cast<bool>(io.rdi.pl_data.isValid()) << std::endl;
        ////std::cout << getPathName() << ": io.rdi.pl_data.ready " << static_cast<bool>(io.rdi.pl_data.isReady()) << std::endl;
        //std::cout << getPathName() << ": io.rdi.pl_data.bits " << std::hex << io.rdi.pl_data.bits().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": has_rx_data " << static_cast<bool>(_has_rx_data->read()) << std::endl;
        //std::cout << getPathName() << ": ratio " << _ratio << std::endl;

    }

} // namespace CCPS