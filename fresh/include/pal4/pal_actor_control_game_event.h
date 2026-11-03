#pragma once

#include "pal4/pal_actor_control.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class IActorControlGameEventRuntime {
public:
    virtual ~IActorControlGameEventRuntime() = default;
    virtual std::int32_t EventCode(std::uint32_t dialog);
    virtual std::uint32_t CreateHintWindow();
    virtual std::optional<std::string> TriggerText(std::uint32_t id);
    virtual std::string ConvertToUtf8(std::string_view text);
    virtual std::string AssignHintText(std::string_view utf8);
    virtual std::uint32_t EncodedSize(std::string_view utf8);
    virtual std::string EncodeHintText(
        std::string_view utf8, std::uint32_t encoded_size);
    virtual void InitializeHintWindow(
        std::uint32_t window, std::string_view text, float x,
        float y, float duration, std::uint32_t zero,
        std::uint32_t one);
    virtual void PlayHintSound(std::string_view path,
                               std::uint32_t repeat);
};

class PalActorControlGameEvent final {
public:
    static constexpr std::string_view kSoundPath =
        "gamedata\\PALSound\\WA05.wav";

    // IDA 0x425d60. True means normal dialog processing may continue.
    [[nodiscard]] static bool Process(
        PalActorControlState& state, std::uint32_t dialog,
        IActorControlGameEventRuntime& runtime);
};

}  // namespace pal4
