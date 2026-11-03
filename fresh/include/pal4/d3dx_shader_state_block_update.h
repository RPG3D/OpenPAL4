#pragma once

#include <cstdint>

namespace pal4 {

class ID3DXShaderStateBlockRuntime {
public:
    virtual ~ID3DXShaderStateBlockRuntime() = default;
    [[nodiscard]] virtual std::int32_t BeginStateBlock(
        std::uint32_t device) = 0;
    [[nodiscard]] virtual std::int32_t EndStateBlock(
        std::uint32_t device, std::uint32_t& state_block) = 0;
    virtual void SetDebugMute(bool muted) = 0;
    [[nodiscard]] virtual std::int32_t CaptureStateBlock(
        std::uint32_t state_block) = 0;
    [[nodiscard]] virtual std::int32_t ApplyStateBlock(
        std::uint32_t state_block) = 0;
    virtual void ReleaseStateBlock(std::uint32_t state_block) = 0;
};

class ID3DXShaderStateBlockOperation {
public:
    virtual ~ID3DXShaderStateBlockOperation() = default;
    [[nodiscard]] virtual std::int32_t Submit() = 0;
};

class D3DXShaderStateBlockUpdate final {
public:
    [[nodiscard]] static std::int32_t Run(
        std::uint32_t device,
        std::uint8_t owner_flags,
        std::uint32_t& saved_state_block,
        ID3DXShaderStateBlockRuntime& runtime,
        ID3DXShaderStateBlockOperation& operation);
};

}  // namespace pal4
