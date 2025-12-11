#ifndef __SIDEBAND_SWITCHER_TEST_HPP__
#define __SIDEBAND_SWITCHER_TEST_HPP__

#include "utils/module.hpp"
#include "utils/base_types.hpp"
#include "utils/wire.hpp"
#include "utils/decoupled.hpp"
#include "sideband/sideband_io.hpp"
#include "sideband/sideband_switcher.hpp"

namespace CCPS {
    class dummyfactory: public WireModule {
    public:
        struct {
            Wire<UInt> output_foryou;
            Wire<UInt> output_notforyou;
        } io;

        dummyfactory();
    };

    class switcher_wrapper: public WireModule {
    public:
        struct {
            SidebandSwitcherbundle inner{true};
            SidebandSwitcherbundle outer;
            Wire<UInt> dummy_foryou;
            Wire<UInt> dummy_notforyou;
        } io;

        SidebandSwitcher s{0x001, SidebandParams()};
        dummyfactory d;

        switcher_wrapper();
    };

    class racefactory: public WireModule {
    public:
        struct {
            Wire<UInt> output_complete;
            Wire<UInt> output_message;
        } io;

        racefactory();

    };

    class race_switcher_wrapper: public WireModule {
        public:
            struct {
                SidebandSwitcherbundle inner{true};
                SidebandSwitcherbundle outer;
                Wire<UInt> output_complete;
                Wire<UInt> output_message;
            } io;

            SidebandSwitcher s{0x001, SidebandParams()};
            racefactory d;

            race_switcher_wrapper();

    };

} // namespace CCPS

#endif // __SIDEBAND_SWITCHER_TEST_HPP__