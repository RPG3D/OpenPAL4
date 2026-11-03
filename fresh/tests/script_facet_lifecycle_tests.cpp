#include "pal4/script_facet_lifecycle.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptFacetLifecycleRuntime {
    std::vector<std::string> calls;

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
};

void AllEntries() {
    constexpr std::array<std::uint32_t, 6> early_order{
        2u, 3u, 4u, 5u, 0u, 1u};
    for (std::uint32_t i = 0u; i < 26u; ++i) {
        const auto vtable = 0x848430u + i * 8u;
        const auto write = "write 28672 " + std::to_string(vtable);
        Runtime runtime;
        pal4::ScriptFacetLifecycle::RestoreVtable(
            0x5886f0u + i * 0x30u, 0x7000u, runtime);
        Check(runtime.calls == std::vector<std::string>{write},
              "non-deleting destructor restores its exact vtable");

        for (std::uint32_t flags = 0u; flags < 4u; ++flags) {
            runtime.calls.clear();
            Check(pal4::ScriptFacetLifecycle::Destroy(
                      0x588700u + i * 0x30u, 0x7000u, flags,
                      runtime) == 0x7000u,
                  "scalar deleting destructor returns its object");
            const std::vector<std::string> expected =
                (flags & 1u) != 0u
                    ? std::vector<std::string>{write, "free 28672"}
                    : std::vector<std::string>{write};
            Check(runtime.calls == expected,
                  "scalar deleting destructor writes before optional free");

            runtime.calls.clear();
            const auto ordinal = i < early_order.size()
                                     ? early_order[i]
                                     : i;
            const auto wrapper_vtable = 0x848430u + ordinal * 8u;
            Check(pal4::ScriptFacetLifecycle::Destroy(
                      0x5883b0u + i * 0x20u, 0x7000u, flags,
                      runtime) == 0x7000u,
                  "wrapper deleting destructor returns its object");
            const auto wrapper_write =
                "write 28672 " + std::to_string(wrapper_vtable);
            const std::vector<std::string> wrapper_expected =
                (flags & 1u) != 0u
                    ? std::vector<std::string>{wrapper_write, "free 28672"}
                    : std::vector<std::string>{wrapper_write};
            Check(runtime.calls == wrapper_expected,
                  "wrapper destructor follows its IDA call order");
        }
    }
}

void InvalidEntries() {
    Runtime runtime;
    for (const auto entry : {0x5883afu, 0x5883b1u, 0x5886efu,
                             0x5886f1u, 0x588701u, 0x588bb1u,
                             0x588bd0u}) {
        bool rejected = false;
        try {
            static_cast<void>(pal4::ScriptFacetLifecycle::Destroy(
                entry, 0x7000u, 1u, runtime));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Check(rejected && runtime.calls.empty(),
              "invalid destructor entry has no runtime effects");
    }
    for (const auto entry : {0x5886efu, 0x5886f1u, 0x588ba1u,
                             0x588bb0u}) {
        bool rejected = false;
        try {
            pal4::ScriptFacetLifecycle::RestoreVtable(
                entry, 0x7000u, runtime);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Check(rejected && runtime.calls.empty(),
              "invalid vtable initializer has no runtime effects");
    }
}

}  // namespace

int main() {
    try {
        AllEntries();
        InvalidEntries();
        std::cout << "script facet lifecycle tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
