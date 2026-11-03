#include "pal4/gob_state_apply.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

struct Runtime final : pal4::IGobStateApplyRuntime {
    std::uint32_t flag = 0u;
    std::uint32_t result = 3u;
    std::uint32_t read_address = 0u;
    std::uint32_t invoked_snapshot = 0u;
    std::uint32_t invokes = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        read_address = address;
        return flag;
    }
    std::uint32_t InvokeSnapshotTypeId(std::uint32_t snapshot) override {
        invoked_snapshot = snapshot;
        ++invokes;
        return result;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        if (pal4::GobStateApply::InvokeTypeIfStored(
                0x1000u, 0x2000u, runtime) != 0u ||
            runtime.read_address != 0x11a4u || runtime.invokes != 0u)
            throw std::runtime_error("disabled store flag");
        runtime.flag = 7u;
        if (pal4::GobStateApply::InvokeTypeIfStored(
                0x1000u, 0x2000u, runtime) != 3u ||
            runtime.invoked_snapshot != 0x2000u || runtime.invokes != 1u)
            throw std::runtime_error("snapshot type dispatch");
        runtime.result = 9u;
        if (pal4::GobStateApply::InvokeTypeIfStored(
                0x1000u, 0x3000u, runtime) != 9u ||
            runtime.invoked_snapshot != 0x3000u || runtime.invokes != 2u)
            throw std::runtime_error("shared Marker type dispatch");
        std::cout << "gob_state_apply_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
