#pragma once

#include "pal4/pal_console.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct PalConsoleCommandSpec {
    std::uint32_t callback;
    const char* name;
    const char* description;
};

class PalConsoleCommands final {
public:
    static constexpr std::size_t kCount = 48u;

    // IDA 0x5bae30: initialize geometry and register the native command list.
    [[nodiscard]] static std::uint32_t Initialize(
        PalConsoleAccessState& access_state,
        IPalConsoleAccessRuntime& access_runtime,
        IPalConsoleObjectRuntime& object_runtime,
        std::int32_t width, std::int32_t height);
    [[nodiscard]] static const std::array<PalConsoleCommandSpec, kCount>&
    Catalog() noexcept;
};

}  // namespace pal4
