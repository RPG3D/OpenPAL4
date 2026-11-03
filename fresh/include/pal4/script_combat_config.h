#pragma once

#include "pal4/combat_system_property_copy.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

enum class CombatConfigTextSlot { bgm, camera, ground_camera };
class IScriptCombatTextRuntime {
public:
    virtual ~IScriptCombatTextRuntime() = default;
    virtual void SetText(CombatConfigTextSlot slot, std::string_view text) = 0;
};

struct CombatConfigText {
    std::uint8_t marker{};
    std::string value;
};

struct CombatConfigSnapshot {
    std::array<std::uint32_t, 5> words{};
    std::array<CombatConfigText, 3> texts{};
    std::array<std::uint8_t, 3> flags{};
};

class IScriptCombatConfigRuntime {
public:
    virtual ~IScriptCombatConfigRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual CombatConfigText ReadTextProperty(
        std::uint32_t address) = 0;
    virtual void ApplyConfiguration(const CombatConfigSnapshot& config) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseText(
        CombatConfigText& text, std::uint32_t index) noexcept = 0;
};

class ICombatConfigurationState : public IScriptCombatConfigRuntime,public IScriptCombatTextRuntime {
public:
    virtual CombatConfigSnapshot Snapshot()const=0;
};
class ScriptCombatConfig final {
public:
    // IDA 0x5d42c0, giConfigCombatParam callback.
    [[nodiscard]] static std::uint32_t Configure(
        std::uint32_t enabled_byte, std::uint32_t slot,
        std::uint32_t value, std::uint32_t type,
        IScriptCombatConfigRuntime& runtime);
};

class ScriptCombatTextThunks final {
public:
    // IDA 0x5d4480 and 0x5d4490.
    [[nodiscard]] static std::uint8_t SetBgm(
        std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetCamera(
        std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime);
};

}  // namespace pal4
