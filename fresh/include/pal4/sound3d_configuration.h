#pragma once

#include "pal4/sound3d_object.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ISound3DConfigurationRuntime {
public:
    virtual ~ISound3DConfigurationRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual std::string ReadString(
        std::uint32_t document, std::string_view key) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t document, std::string_view key,
        std::int32_t fallback) = 0;
    virtual void InitializeSoundPath(Sound3DObjectState& state,
                                     std::string_view prefix) = 0;
    virtual void AppendSoundFile(Sound3DObjectState& state,
                                 std::string_view filename) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            const std::array<float, 3>& scale,
                            std::uint32_t mode) = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             const std::array<float, 3>& axis,
                             float degrees, std::uint32_t mode) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const std::array<float, 3>& position,
                                std::uint32_t mode) = 0;
};

class Sound3DConfiguration final {
public:
    // IDA 0x617260.
    [[nodiscard]] static std::uint8_t Load(
        Sound3DObjectState& state, std::uint32_t document,
        ISound3DConfigurationRuntime& runtime);
};

}  // namespace pal4
