#include "pal4/debug_window.h"

namespace pal4 {

std::uint32_t DebugWindowAccess::Get(
    DebugWindowAccessState& state, IDebugWindowAccessRuntime& runtime) {
    if ((state.guard & 1u) != 0u) {
        return state.instance;
    }
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.Register(
        runtime.GetSingletonManager(), "palDebugWindow", 1021u,
        kFactoryAddress, kDestroyAddress);
    return state.instance;
}

std::uint32_t DebugWindowState::ReadDword(const std::uint32_t offset) const {
    return static_cast<std::uint32_t>(bytes_.at(offset)) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 1u)) << 8u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 2u)) << 16u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 3u)) << 24u);
}

void DebugWindowState::WriteDword(const std::uint32_t offset,
                                  const std::uint32_t value) {
    for (std::uint32_t index = 0; index < 4u; ++index) {
        bytes_.at(offset + index) =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

std::uint32_t DebugWindowObject::Create(IDebugWindowRuntime& runtime) {
    const auto address = runtime.Allocate(DebugWindowState::kBytes);
    if (address == 0u) {
        return 0u;
    }
    try {
        return Construct(address, runtime);
    } catch (...) {
        runtime.Free(address);
        throw;
    }
}

std::uint32_t DebugWindowObject::Construct(
    const std::uint32_t address, IDebugWindowRuntime& runtime) {
    constexpr std::uint32_t kFirstString = 0x14u;
    constexpr std::uint32_t kSecondString = 0x20u;
    runtime.ConstructString(address + kFirstString);
    try {
        runtime.ConstructString(address + kSecondString);
    } catch (...) {
        runtime.CleanupString(address + kFirstString);
        throw;
    }
    try {
        auto& state = runtime.Access(address);
        state.WriteDword(0u, 0u);
        state.WriteDword(4u, 0u);
        state.WriteDword(0x10u, 0x1000u);
        runtime.ReallocateString(address + kFirstString, 0x1000u);
        runtime.ReallocateString(address + kSecondString,
                                 state.ReadDword(0x10u));
    } catch (...) {
        runtime.CleanupString(address + kSecondString);
        runtime.CleanupString(address + kFirstString);
        throw;
    }
    return address;
}

void DebugWindowObject::Destruct(const std::uint32_t address,
                                 IDebugWindowRuntime& runtime) {
    try {
        runtime.CleanupString(address + 0x20u);
    } catch (...) {
        runtime.CleanupString(address + 0x14u);
        throw;
    }
    runtime.CleanupString(address + 0x14u);
}

void DebugWindowObject::Destroy(const std::uint32_t address,
                                IDebugWindowRuntime& runtime) {
    if (address != 0u) {
        Destruct(address, runtime);
        runtime.Free(address);
    }
}

std::uint32_t DebugWindowObject::DestroyWindows(
    const std::uint32_t address, IDebugWindowRuntime& runtime) {
    auto& state = runtime.Access(address);
    const auto first = state.ReadDword(0u);
    if (first != 0u) {
        static_cast<void>(runtime.DestroyWindow(first));
        state.WriteDword(0u, 0u);
    }
    const auto second = state.ReadDword(4u);
    if (second == 0u) {
        return second;
    }
    runtime.KillTimer(second, 1u);
    const auto result = runtime.DestroyWindow(state.ReadDword(4u));
    state.WriteDword(4u, 0u);
    return result;
}

}  // namespace pal4
