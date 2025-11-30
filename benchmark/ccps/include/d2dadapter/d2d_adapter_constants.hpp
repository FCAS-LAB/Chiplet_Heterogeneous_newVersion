#ifndef __D2D_ADAPTER_CONSTANTS_HPP__
#define __D2D_ADAPTER_CONSTANTS_HPP__

namespace CCPS {

    // 3bits
    enum LinkInitState{
        INIT_START = 0x0,
        RDI_BRINGUP = 0x1,
        PARAM_EXCH = 0x2,
        FDI_BRINGUP = 0x3,
        INIT_DONE = 0x4
    };

    struct D2DAdapterSignalSize {
        int SIDEBAND_MESSAGE_OP_WIDTH = 6;
    };

    // 6bits
    enum SideBandMessage{
        // start with 01: RES
        // start with 00: REQ
        // start with 1: others
        NOP = 0b000000,
        REQ_ACTIVE = 0b000001,
        REQ_L1 = 0b000100,
        REQ_L2 = 0b001000,
        REQ_LINKRESET = 0b001001,
        REQ_DISABLED = 0b001100,
        RSP_ACTIVE = 0b010001,
        RSP_PMNAK = 0b010011,
        RSP_L1 = 0b010100,
        RSP_L2 = 0b011000,
        RSP_LINKRESET = 0b011001,
        RSP_DISABLED = 0b011100,
        PARITY_FEATURE_REQ = 0b100001,
        PARITY_FEATURE_ACK = 0b110001,
        PARITY_FEATURE_NAK = 0b110010,
        ADV_CAP = 0b100100,
        REGISTER_ACCESS = 0b101000
    };

    const unsigned STATE_WIDTH = 2;
    // 2bits
    enum StallHandshakeState {
        IDLE = 0x0,
        REQSNT = 0x1,
        REQFALL = 0x2,
        COMPLETE = 0x3
    };

    // Parity module constants
    // 3bits
    enum ParityGeneratorWidth{
        PARITY_N_WIDTH = 3
    };

    enum ParityAmount{
        BASESIZE = 64,
        PARITY_DATA_NBYTE_1 = 64,
        DATA_NBYTE_1 = 256 * 256 * 1,
        PARITY_DATA_NBYTE_2 = 64 * 2,
        DATA_NBYTE_2 = 256 * 256 * 2,
        PARITY_DATA_NBYTE_4 = 64 * 4,
        DATA_NBYTE_4 = 256 * 256 * 4,
        CORRECT_REG_WIDTH = 256 // 4 * 64 for maximum four 64Bytes parity
    };

    enum ParityN{
        ONE = 0b000,
        TWO = 0b001,
        FOUR = 0b010
    };
} // namespace CCPS

#endif //__D2D_ADAPTER_CONSTANTS_HPP__
