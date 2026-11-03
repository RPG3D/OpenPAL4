#pragma once

#include "pal4/clump_object.h"

#include <cstdint>

namespace pal4 {

class IClumpObjectRenderRuntime {
public:
    virtual ~IClumpObjectRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetRenderState(
        std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint8_t SetRenderState(
        std::uint32_t state, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetTextureStageState(
        std::uint32_t stage) = 0;
    virtual void SetTextureStageState(std::uint32_t stage,
                                     std::uint32_t value) = 0;
    virtual void Render(std::uint32_t clump) = 0;
};

class ClumpObjectRenderRuntimeStub final : public IClumpObjectRenderRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetRenderState(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint8_t SetRenderState(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetTextureStageState(
        std::uint32_t) override { return 0u; }
    void SetTextureStageState(std::uint32_t, std::uint32_t) override {}
    void Render(std::uint32_t) override {}
};

}  // namespace pal4
