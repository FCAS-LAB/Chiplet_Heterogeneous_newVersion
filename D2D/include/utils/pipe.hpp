#ifndef __PIPE_HPP__
#define __PIPE_HPP__

#include "module.hpp"
#include <vector>

namespace CCPS{
    // =============================== BaseClock ==========================
    // For now, only support UInt
    class Pipe: RegModule {
    public:
        struct {
            struct {
                Wire<Bool> valid;
                Wire<Bool> ready;
                Wire<UInt> bits;
            } enq;
            struct {
                Wire<Bool> valid;
                Wire<Bool> ready;
                Wire<UInt> bits;
            } deq;
        } io;

        // latency must be > 0
        Pipe(int latency, int width);
        void calcNextState();

    private:
        std::vector<RegPtr<UInt>> _v_bits;
        std::vector<RegPtr<Bool>> _v_valid;
        RegPtr<UInt> _counter;
        int _latency;
        int _width;
        std::vector<bool> _temp_valids;
        std::vector<bool> _temp_readys;
    };
}

#endif // __PIPE_HPP__