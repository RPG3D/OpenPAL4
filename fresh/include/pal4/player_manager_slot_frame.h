#pragma once

#include <cstdint>

namespace pal4 {

class IPlayerManagerSlotFrameRuntime {
public:
    virtual ~IPlayerManagerSlotFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void UpdatePosition(float elapsed, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateAnimationList(
        std::uint32_t list, float elapsed) = 0;
};

class PlayerManagerSlotFrameRuntimeStub :
    public IPlayerManagerSlotFrameRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void UpdatePosition(float, std::uint32_t) override;
    std::uint32_t UpdateAnimationList(std::uint32_t, float) override;
};

class PlayerManagerSlotFrame final {
public:
    // IDA 0x43f480: optional inherited position update, then animation.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object, float elapsed, std::uint32_t mode,
        IPlayerManagerSlotFrameRuntime& runtime);
};

}  // namespace pal4
