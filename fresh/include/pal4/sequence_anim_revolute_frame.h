#pragma once

#include "pal4/sequence_anim_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ISequenceAnimRevoluteRuntime {
public:
    virtual ~ISequenceAnimRevoluteRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        SequenceAnimObjectState& state) = 0;
    virtual void TranslateMatrix(std::uint32_t matrix,
                                 const std::array<float, 3>& vector,
                                 std::uint32_t mode) = 0;
    virtual void RotateMatrix(std::uint32_t matrix,
                              const std::array<float, 3>& axis,
                              float angle, std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        SequenceAnimObjectState& state,
        const std::array<float, 3>& position,
        std::uint32_t mode) = 0;
};

class SequenceAnimRevoluteRuntimeStub final
    : public ISequenceAnimRevoluteRuntime {
public:
    [[nodiscard]] std::uint32_t GetFrame(
        SequenceAnimObjectState&) override { return 0u; }
    void TranslateMatrix(std::uint32_t, const std::array<float, 3>&,
                         std::uint32_t) override {}
    void RotateMatrix(std::uint32_t, const std::array<float, 3>&,
                      float, std::uint32_t) override {}
    void UpdateFrameObjects(std::uint32_t) override {}
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t SetPosition(
        SequenceAnimObjectState&, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
};

class SequenceAnimRevoluteFrame final {
public:
    // IDA 0x616620, owner +28 facet; dispatches to owner +24 position facet.
    [[nodiscard]] static std::uint32_t Apply(
        SequenceAnimObjectState& state,
        const std::array<float, 3>& pivot,
        const std::array<float, 3>& rotation,
        const std::array<float, 3>& step,
        std::uint32_t entry_eax,
        ISequenceAnimRevoluteRuntime& runtime);
};

}  // namespace pal4
