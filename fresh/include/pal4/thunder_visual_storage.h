#pragma once

#include "pal4/thunder_object.h"

#include <cstdint>

namespace pal4 {

class IThunderVisualStorageRuntime {
public:
    virtual ~IThunderVisualStorageRuntime() = default;
    virtual void ResetProperties(ThunderObjectState& state,
                                 std::uint32_t visual_offset) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class ThunderVisualStorage final {
public:
    // IDA 0x64a420. Returns the native x87 comparison mask.
    [[nodiscard]] static std::uint16_t AdvanceTimer(
        ThunderObjectState& state, double frame_time);

    // IDA 0x64a310.
    static void ResetProperties(
        ThunderObjectState& state,
        class IThunderVisualResetRuntime& runtime);

    // IDA 0x64a290.
    [[nodiscard]] static std::uint8_t InitializeWithTexture(
        ThunderObjectState& state, std::uint32_t texture, float fps,
        std::uint8_t mode, class IThunderVisualInitializationRuntime& runtime);

    // IDA 0x61cd20; this points to owner +60.
    [[nodiscard]] static std::uint32_t Cleanup(
        ThunderObjectState& state, IThunderVisualStorageRuntime& runtime);
};

class IThunderVisualInitializationRuntime {
public:
    virtual ~IThunderVisualInitializationRuntime() = default;
    virtual void ResetProperties(ThunderObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void SetupAndUpdateFrame(ThunderObjectState& state) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
};

class IThunderVisualResetRuntime {
public:
    virtual ~IThunderVisualResetRuntime() = default;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
};

}  // namespace pal4
