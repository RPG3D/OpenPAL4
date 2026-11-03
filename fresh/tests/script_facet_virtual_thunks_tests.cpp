#include "pal4/script_facet_virtual_thunks.h"
#include "pal4/crt_noop_atexit_registration.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct VirtualRuntime final : pal4::IScriptFacetVirtualRuntime {
    std::uint32_t object = 0u;
    std::uint32_t offset = 0u;
    std::vector<std::uint32_t> arguments;
    std::uint32_t calls = 0u;

    std::uint32_t Invoke(std::uint32_t receiver,
                         std::uint32_t vtable_offset,
                         std::span<const std::uint32_t> args) override {
        object = receiver;
        offset = vtable_offset;
        arguments.assign(args.begin(), args.end());
        ++calls;
        return 0x12345678u;
    }
};

struct ExitRuntime final : pal4::ICrtNoopAtExitRuntime {
    std::vector<std::uint32_t> callbacks;
    int RegisterAtExit(std::uint32_t callback) override {
        callbacks.push_back(callback);
        return 41;
    }
};

void AllThunks() {
    constexpr std::array<std::uint32_t, 42> offsets{
        0x7cu, 0xbcu, 0x9cu, 0xdcu, 0x5cu, 0x3cu, 0x1cu,
        0x80u, 0xc0u, 0xa0u, 0xe0u, 0x40u, 0xc4u, 0xa4u,
        0xe4u, 0x64u, 0x44u, 0x08u, 0xc8u, 0xa8u, 0x88u,
        0x68u, 0x28u, 0xccu, 0xacu, 0x8cu, 0x2cu, 0xb0u,
        0x90u, 0xd0u, 0x70u, 0xb4u, 0x94u, 0xd4u, 0x74u,
        0x34u, 0xb8u, 0x98u, 0xd8u, 0x58u, 0x38u, 0x18u,
    };
    constexpr std::array<std::uint32_t, 3> arguments{7u, 0u, 99u};
    for (std::uint32_t i = 0u; i < offsets.size(); ++i) {
        VirtualRuntime runtime;
        Check(pal4::ScriptFacetVirtualThunks::Dispatch(
                  0x588c00u + i * 0x10u, 0x7000u, arguments,
                  runtime) == 0x12345678u &&
                  runtime.calls == 1u && runtime.object == 0x7000u &&
                  runtime.offset == offsets[i] &&
                  runtime.arguments == std::vector<std::uint32_t>{7u, 0u, 99u},
              "every virtual thunk forwards receiver, slot, arguments and result");
    }
    VirtualRuntime runtime;
    for (auto entry : {0x588bffu, 0x588c01u, 0x588e91u, 0x588ea0u}) {
        bool rejected = false;
        try {
            static_cast<void>(pal4::ScriptFacetVirtualThunks::Dispatch(
                entry, 0x7000u, arguments, runtime));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Check(rejected && runtime.calls == 0u,
              "invalid virtual thunk has no effects");
    }
}

void AtexitRegistration() {
    ExitRuntime runtime;
    std::uint8_t flags = 0xa4u;
    Check(pal4::CrtNoopAtExitRegistration::Run(flags, runtime) == 41 &&
              flags == 0xa5u &&
              runtime.callbacks == std::vector<std::uint32_t>{0x5e0560u},
          "first registration sets only bit zero and forwards CRT result");
    Check(pal4::CrtNoopAtExitRegistration::Run(flags, runtime) == 41 &&
              flags == 0xa5u && runtime.callbacks.size() == 2u &&
              runtime.callbacks[1] == 0x5e0560u,
          "registration still calls atexit when the flag was already set");
}

}  // namespace

int main() {
    try {
        AllThunks();
        AtexitRegistration();
        std::cout << "script facet virtual thunk tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
