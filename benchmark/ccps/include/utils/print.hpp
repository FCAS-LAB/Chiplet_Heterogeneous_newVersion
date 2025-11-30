#ifndef __PRINT_HPP__
#define __PRINT_HPP__

#include "utils/base_types.hpp"
#include "utils/wire.hpp"
#include <string>

namespace CCPS {
void print_io(const std::string& path_name, const Wire<Bool> &io);
void print_io(const std::string& path_name, const Wire<UInt> &io);
void print_io(const std::string& path_name, const RegPtr<Bool> &io);
void print_io(const std::string& path_name, const RegPtr<UInt> &io);

//#define PRINT(io) print_io(getPathName() + ": " + #io, io)
#define PRINT(io) {}
}

//#define ENTER_MODULE_FUNC() (std::cout << getPathName() << ": do function " << __func__) << std::endl
//#define ENTER_FUNC() (std::cout << "do function " << __func__) << std::endl
#define ENTER_MODULE_FUNC() {}
#define ENTER_FUNC() {}

#endif // __PRINT_HPP__
