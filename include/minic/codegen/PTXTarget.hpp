#pragma once

#include <string>

namespace minic::codegen {

struct PTXTarget {
    std::string ptxVersion{"7.0"};
    std::string smArch{"sm_70"};
    int addressSize{64};
    bool isKernel{false}; // Treat entry functions as GPU kernels (.entry) vs device functions (.func)

    PTXTarget() = default;
    PTXTarget(std::string arch, bool kernel = false, std::string version = "7.0")
        : ptxVersion(std::move(version)), smArch(std::move(arch)), isKernel(kernel) {}
};

} // namespace minic::codegen
