#include "utils/print.hpp"

namespace CCPS {

void print_io(const std::string& path_name, const Wire<Bool> &io) {
    std::cout << path_name << ": " << static_cast<bool>(io()) << std::endl;
}

void print_io(const std::string& path_name, const Wire<UInt> &io) {
    std::cout << path_name << ": " << io().toBigUInt() << std::endl;
}

void print_io(const std::string& path_name, const RegPtr<Bool> &io) {
    std::cout << path_name << ": " << static_cast<bool>(io->read()) << std::endl;
}

void print_io(const std::string& path_name, const RegPtr<UInt> &io) {
    std::cout << path_name << ": " << io->read().toBigUInt() << std::endl;
}


} // CCPS