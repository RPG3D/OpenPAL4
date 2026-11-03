#pragma once

#include "pal4/sequence_anim_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ISequenceAnimPropertiesRuntime {
public:
    virtual ~ISequenceAnimPropertiesRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeTransform(
        std::uint32_t resource) = 0;
    [[nodiscard]] virtual std::uint8_t SetGeometryColor(
        std::uint32_t geometry, std::uint32_t rgba) = 0;
};

class SequenceAnimPropertiesRuntimeStub final
    : public ISequenceAnimPropertiesRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t InitializeTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t SetGeometryColor(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class SequenceAnimProperties final {
public:
    // IDA 0x616450, +44 facet. Third output pointer low byte is returned.
    [[nodiscard]] static std::uint8_t GetRotation(
        const SequenceAnimObjectState& state,
        std::array<std::uint32_t, 3>& output,
        std::uint32_t third_output_address,
        ISequenceAnimPropertiesRuntime& runtime);
    // IDA 0x616480, +40 facet.
    [[nodiscard]] static std::uint8_t GetScale(
        const SequenceAnimObjectState& state,
        std::array<std::uint32_t, 3>& output,
        ISequenceAnimPropertiesRuntime& runtime);
    // IDA 0x6164b0, +24 facet.
    [[nodiscard]] static std::uint8_t GetPosition(
        const SequenceAnimObjectState& state,
        std::array<std::uint32_t, 3>& output,
        ISequenceAnimPropertiesRuntime& runtime);
    // IDA 0x6164e0, +44 facet. Only the first float is used.
    [[nodiscard]] static std::uint32_t SetRotation(
        const SequenceAnimObjectState& state, float rotation,
        std::uint8_t relative, std::uint32_t entry_eax,
        ISequenceAnimPropertiesRuntime& runtime);
    // IDA 0x616510, +40 facet. Only the first two floats are used.
    [[nodiscard]] static std::uint32_t SetScale(
        const SequenceAnimObjectState& state, float width, float height,
        std::uint8_t relative, std::uint32_t entry_eax,
        ISequenceAnimPropertiesRuntime& runtime);
    // IDA 0x616580, +24 facet.
    [[nodiscard]] static std::uint32_t SetPosition(
        const SequenceAnimObjectState& state,
        const std::array<float, 3>& position,
        std::uint8_t relative, std::uint32_t entry_eax,
        ISequenceAnimPropertiesRuntime& runtime);
    // IDA 0x616870, +32 facet.
    [[nodiscard]] static std::uint8_t SetPositionWords(
        const SequenceAnimObjectState& state,
        const std::array<std::uint32_t, 3>& position,
        ISequenceAnimPropertiesRuntime& runtime);
    // IDA 0x6168c0 / 0x616900, +36 facet.
    [[nodiscard]] static std::uint8_t GetColor(
        const SequenceAnimObjectState& state,
        std::array<std::uint8_t, 4>& output,
        ISequenceAnimPropertiesRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetColor(
        const SequenceAnimObjectState& state,
        const std::array<std::uint8_t, 4>& channels,
        ISequenceAnimPropertiesRuntime& runtime);
};

}  // namespace pal4
