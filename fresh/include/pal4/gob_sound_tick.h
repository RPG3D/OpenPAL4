#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct GobSoundVector {
    float x;
    float y;
    float z;
};

class IGobSoundTickRuntime {
public:
    virtual ~IGobSoundTickRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSoundConfigNode(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsAudioPlaying(std::uint32_t handle) = 0;
    [[nodiscard]] virtual GobSoundVector ActiveCameraPosition() = 0;
    [[nodiscard]] virtual float CalculateVectorMagnitude(
        const GobSoundVector& vector) = 0;
    [[nodiscard]] virtual float AudibleRadius(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t TimeMilliseconds() = 0;
    [[nodiscard]] virtual float RandomDelay(float minimum, float maximum,
                                            float seed) = 0;
    [[nodiscard]] virtual std::string BuildSoundPath(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t Play3DSound(
        const GobSoundVector& listener_relative_source,
        std::string_view path, const GobSoundVector& emitter_position,
        std::uint32_t use_count, float radius,
        float near_radius) = 0;
    virtual void ApplyMovement(std::uint32_t object,
                               const GobSoundVector& movement,
                               std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateClumpIfVisible(
        std::uint32_t object, float elapsed) = 0;
};

}  // namespace pal4
