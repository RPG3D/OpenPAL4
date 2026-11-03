#pragma once

#include "pal4/clump_object.h"

#include <cstdint>

namespace pal4 {

class IClumpObjectColorRuntime {
public:
    virtual ~IClumpObjectColorRuntime() = default;
    [[nodiscard]] virtual std::uintptr_t SetColor(
        std::uint32_t manager, std::uint32_t rgba) = 0;
};

class ClumpObjectColorRuntimeStub final : public IClumpObjectColorRuntime {
public:
    [[nodiscard]] std::uintptr_t SetColor(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

}  // namespace pal4
