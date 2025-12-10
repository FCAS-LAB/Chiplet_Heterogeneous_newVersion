#include "d2dadapter/parity_generator.hpp"
#include "d2dadapter/d2d_adapter_constants.hpp"
#include "interfaces/types.hpp"

namespace CCPS {
    ParityGenerator::ParityGenerator(const FdiParams &fdi_params): _fdi_params(fdi_params) {
        // Instantiate
        io.snd_data.resize(fdi_params.width);
        io.rcv_data.resize(fdi_params.width);
        _parity_data_snd_reg.resize(ParityAmount::PARITY_DATA_NBYTE_4);
        for (auto &reg : _parity_data_snd_reg) {
            reg = createReg<Bool>(Bool(false));
        }
        _parity_data_rcv_reg.resize(ParityAmount::PARITY_DATA_NBYTE_4);
        for (auto &reg : _parity_data_rcv_reg) {
            reg = createReg<Bool>(Bool(false));
        }
        _parity_dcount_snd_reg = createReg<UInt>(UInt(19, 0));
        _parity_pcount_snd_reg = createReg<UInt>(UInt(9, 0));
        _parity_dcount_rcv_reg = createReg<UInt>(UInt(19, 0));
        _parity_pcount_rcv_reg = createReg<UInt>(UInt(9, 0));
        _parity_check_result_valid_reg = createReg<Bool>(Bool(false));
        _parity_check_bits_reg.resize(ParityAmount::PARITY_DATA_NBYTE_4);
        for (auto &reg : _parity_check_bits_reg) {
            reg = createReg<Bool>(Bool(false));
        }

        // Connect
        _io_parity_data.resize(fdi_params.width);
        io.parity_data.resize(fdi_params.width);
        for (int i = 0; i < fdi_params.width; i++) {
            _io_parity_data[i] = [this, i] () -> Bool {
                return _parity_data_snd_reg[i]->read();
            };
            io.parity_data[i] = _io_parity_data[i];
        }

        _io_parity_insert = [this] () -> Bool {
            return Bool(_parity_dcount_snd_reg->read() == _n_256_256());
        };
        io.parity_insert = _io_parity_insert;

        _io_parity_check = [this] () -> Bool {
            return Bool(_parity_dcount_rcv_reg->read() == _n_256_256());
        };
        io.parity_check = _io_parity_check;

        _io_parity_check_result.resize(ParityAmount::PARITY_DATA_NBYTE_4);
        io.parity_check_result.resize(ParityAmount::PARITY_DATA_NBYTE_4);
        for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
            _io_parity_check_result[i] = [this, i] () -> Bool {
                return _parity_check_bits_reg[i]->read();
            };
            io.parity_check_result[i] = _io_parity_check_result[i];
        }

        _io_parity_check_result_valid = [this] () -> Bool {
            return _parity_check_result_valid_reg->read();
        };
        io.parity_check_result_valid = _io_parity_check_result_valid;

        _n_64 = [this] () -> UInt {
            BigUInt res = 0;
            BigUInt parity_n = io.parity_n().toBigUInt();
            if (parity_n == ParityN::ONE) {
                res = ParityAmount::PARITY_DATA_NBYTE_1;
            } else if (parity_n == ParityN::TWO) {
                res = ParityAmount::PARITY_DATA_NBYTE_2;
            } else if (parity_n == ParityN::FOUR) {
                res = ParityAmount::PARITY_DATA_NBYTE_4;
            } else {
                res = ParityAmount::PARITY_DATA_NBYTE_1;
            }
            return UInt(9, res);
        };

        _n_256_256 = [this] () -> UInt {
            BigUInt res = 0;
            BigUInt parity_n = io.parity_n().toBigUInt();
            if (parity_n == ParityN::ONE) {
                res = ParityAmount::DATA_NBYTE_1;
            } else if (parity_n == ParityN::TWO) {
                res = ParityAmount::DATA_NBYTE_2;
            } else if (parity_n == ParityN::FOUR) {
                res = ParityAmount::DATA_NBYTE_4;
            } else {
                res = ParityAmount::DATA_NBYTE_1;
            }
            return UInt(19, res);
        };
    }

    void ParityGenerator::calcNextState() {
        // snd data add parity
        if (io.rdi_state().toBigUInt() != PhyState::active) {
            for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                *_parity_data_snd_reg[i] = Bool(false);
            }
            *_parity_dcount_snd_reg = UInt(19, 0);
            *_parity_pcount_snd_reg = UInt(9, 0);
        } else if (_parity_pcount_snd_reg->read().toBigUInt() + _fdi_params.width == _n_64().toBigUInt() && static_cast<bool>(io.parity_rdy())) {
            for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                *_parity_data_snd_reg[i] = Bool(false);
            }
            *_parity_dcount_snd_reg = UInt(19, 0);
            *_parity_pcount_snd_reg = UInt(9, 0);
        } else if (static_cast<bool>(io.snd_data_vld()) && static_cast<bool>(io.parity_tx_enable()) && _parity_dcount_snd_reg->read().toBigUInt() != _n_256_256().toBigUInt()) {
            if (io.parity_n().toBigUInt() == ParityN::ONE) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_1 - _fdi_params.width; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_1 - _fdi_params.width; i < ParityAmount::PARITY_DATA_NBYTE_1; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]->read() ^ io.snd_data[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]().xorR();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_1; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::TWO) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_2 - _fdi_params.width; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2 - _fdi_params.width; i < ParityAmount::PARITY_DATA_NBYTE_2; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read() ^ io.snd_data[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]().xorR();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::FOUR) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4 - _fdi_params.width; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_4]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_4 - _fdi_params.width; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_4]->read() ^ io.snd_data[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]().xorR();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_4; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[i]->read();
                }
            }
            *_parity_dcount_snd_reg = _parity_dcount_snd_reg->read() + UInt(19, _fdi_params.width);
            *_parity_pcount_snd_reg = UInt(9, 0);
        } else if (_parity_dcount_snd_reg->read().toBigUInt() == _n_256_256().toBigUInt() && static_cast<bool>(io.parity_rdy())) {
            if (io.parity_n().toBigUInt() == ParityN::ONE) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_1; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_1; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::TWO) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_2; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::FOUR) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_snd_reg[i] = _parity_data_snd_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_4]->read();
                }
            }
            *_parity_dcount_snd_reg = _n_256_256();
            *_parity_pcount_snd_reg = _parity_pcount_snd_reg->read() + UInt(9, _fdi_params.width);
        }

        if (io.rdi_state().toBigUInt() != PhyState::active) {
            for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                *_parity_data_rcv_reg[i] = Bool(false);
            }
            *_parity_dcount_rcv_reg = UInt(19, 0);
            *_parity_pcount_rcv_reg = UInt(9, 0);
            *_parity_check_result_valid_reg = Bool(false);
        } else if (_parity_pcount_rcv_reg->read().toBigUInt() + _fdi_params.width == _n_64().toBigUInt()
                && static_cast<bool>(io.rcv_data_vld())
                && _parity_dcount_rcv_reg->read().toBigUInt() == _n_256_256().toBigUInt()) {
            for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                *_parity_data_rcv_reg[i] = Bool(false);
            }
            *_parity_dcount_rcv_reg = UInt(19, 0);
            *_parity_pcount_rcv_reg = UInt(9, 0);
            *_parity_check_result_valid_reg = Bool(true);
        } else if (static_cast<bool>(io.rcv_data_vld()) && static_cast<bool>(io.parity_rx_enable()) && _parity_dcount_rcv_reg->read().toBigUInt() != _n_256_256().toBigUInt()) {
            if (io.parity_n().toBigUInt() == ParityN::ONE) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_1 - _fdi_params.width; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_1 - _fdi_params.width; i < ParityAmount::PARITY_DATA_NBYTE_1; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]->read() ^ io.rcv_data[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]().xorR();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_1; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::TWO) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_2 - _fdi_params.width; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2 - _fdi_params.width; i < ParityAmount::PARITY_DATA_NBYTE_2; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read() ^ io.rcv_data[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]().xorR();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::FOUR) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4 - _fdi_params.width; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_4]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_4 - _fdi_params.width; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_4]->read() ^ io.rcv_data[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]().xorR();
                }
            }
            *_parity_dcount_rcv_reg = _parity_dcount_rcv_reg->read() + UInt(19, _fdi_params.width);
            *_parity_pcount_rcv_reg = UInt(9, 0);
            *_parity_check_result_valid_reg = Bool(false);
        } else if (_parity_dcount_rcv_reg->read().toBigUInt() == _n_256_256().toBigUInt() && static_cast<bool>(io.rcv_data_vld())) {
            if (io.parity_n().toBigUInt() == ParityN::ONE) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_1; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_1; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::TWO) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_2; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::FOUR) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_4]->read();
                }
            }
            *_parity_dcount_rcv_reg = _n_256_256();
            *_parity_pcount_rcv_reg = _parity_pcount_rcv_reg->read() + UInt(9, _fdi_params.width);
            *_parity_check_result_valid_reg = Bool(false);
        } else {
            for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                *_parity_data_rcv_reg[i] = _parity_data_rcv_reg[i]->read();
            }
            *_parity_dcount_rcv_reg = _parity_dcount_rcv_reg->read();
            *_parity_pcount_rcv_reg = _parity_pcount_rcv_reg->read();
            *_parity_check_result_valid_reg = Bool(false);
        }

        if (_parity_dcount_rcv_reg->read().toBigUInt() == _n_256_256().toBigUInt() && static_cast<bool>(io.rcv_data_vld())) { // this data should be checked{
            if (io.parity_n().toBigUInt() == ParityN::ONE) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_1 - _fdi_params.width; i++) {
                    *_parity_check_bits_reg[i] = _parity_check_bits_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_1 - _fdi_params.width; i < ParityAmount::PARITY_DATA_NBYTE_1; i++) {
                    *_parity_check_bits_reg[i] = Bool(_parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]->read() != io.rcv_data[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]());
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_1; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_check_bits_reg[i] = _parity_check_bits_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::TWO) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_2 - _fdi_params.width; i++) {
                    *_parity_check_bits_reg[i] = _parity_check_bits_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2 - _fdi_params.width; i < ParityAmount::PARITY_DATA_NBYTE_2; i++) {
                    *_parity_check_bits_reg[i] = Bool(_parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read() != io.rcv_data[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]());
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_check_bits_reg[i] = _parity_check_bits_reg[i]->read();
                }
            } else if (io.parity_n().toBigUInt() == ParityN::FOUR) {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_2 - _fdi_params.width; i++) {
                    *_parity_check_bits_reg[i] = _parity_check_bits_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read();
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2 - _fdi_params.width; i < ParityAmount::PARITY_DATA_NBYTE_2; i++) {
                    *_parity_check_bits_reg[i] = Bool(_parity_data_rcv_reg[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_2]->read() != io.rcv_data[(i + _fdi_params.width) % ParityAmount::PARITY_DATA_NBYTE_1]());
                }
                for (int i = ParityAmount::PARITY_DATA_NBYTE_2; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_check_bits_reg[i] = _parity_check_bits_reg[i]->read();
                }
            } else {
                for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                    *_parity_check_bits_reg[i] = _parity_check_bits_reg[i]->read();
                }
            }
        } else {
            for (int i = 0; i < ParityAmount::PARITY_DATA_NBYTE_4; i++) {
                *_parity_check_bits_reg[i] = _parity_check_bits_reg[i]->read();
            }
        }

        //std::cout << getPathName() << ": io.parity_rx_enable    " << static_cast<bool>(io.parity_rx_enable()) << std::endl;
        //std::cout << getPathName() << ": io.parity_n            " << io.parity_n().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.rdi_state           " << io.rdi_state().toBigUInt() << std::endl;
        //std::cout << getPathName() << ": io.parity_rdy          " << static_cast<bool>(io.parity_rdy()) << std::endl;
        //std::cout << getPathName() << ": io.snd_data_vld        " << static_cast<bool>(io.snd_data_vld()) << std::endl;
        //std::cout << getPathName() << ": io.rcv_data_vld        " << static_cast<bool>(io.rcv_data_vld()) << std::endl;
        //std::cout << getPathName() << ": io.parity_check        " << static_cast<bool>(io.parity_check()) << std::endl;
        //std::cout << getPathName() << ": parity_dcount_rcv_reg  " << _parity_dcount_rcv_reg->read() << std::endl;

        //for (size_t i = 0; i < io.snd_data.size(); i++) {
        //    PRINT(io.snd_data[i]);
        //}
        PRINT(io.snd_data_vld);
        //for (size_t i = 0; i < io.rcv_data.size(); i++) {
        //    PRINT(io.rcv_data[i]);
        //}
        PRINT(io.rcv_data_vld);
        for (size_t i = 0; i < io.parity_data.size(); i++) {
            PRINT(io.parity_data[i]);
        }
        PRINT(io.parity_insert);
        PRINT(io.parity_check);
        PRINT(io.parity_rdy);
        for (size_t i = 0; i < io.parity_check_result.size(); i++) {
            PRINT(io.parity_check_result[i]);
        }
        PRINT(io.parity_check_result_valid);
        PRINT(io.rdi_state);
        PRINT(io.parity_rx_enable);
        PRINT(io.parity_tx_enable);
        PRINT(io.parity_n);

    }
} // namespace CCPS