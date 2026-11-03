#pragma once

#include <cstdint>

namespace pal4 {

class IStripAtomicRenderRuntime {
public:
    virtual ~IStripAtomicRenderRuntime() = default;
    virtual void GetRenderState(std::uint32_t state,
                                std::uint32_t& value) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void GetDeviceRenderState(std::uint32_t state,
                                      std::uint32_t& value) = 0;
    virtual void SetDeviceRenderState(std::uint32_t state,
                                      std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RenderOriginalAtomic(
        std::uint32_t atomic) = 0;
};

class StripAtomicRenderer final {
public:
    // IDA 0x6489c0.
    [[nodiscard]] static std::uint32_t RenderBasic(
        std::uint32_t atomic, IStripAtomicRenderRuntime& runtime);

    // IDA 0x648a50.
    [[nodiscard]] static std::uint32_t RenderBlended(
        std::uint32_t atomic, IStripAtomicRenderRuntime& runtime);
};

}  // namespace pal4
