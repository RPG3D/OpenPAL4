#pragma once

#include <cstdint>

namespace pal4 {

class IScriptEventVolumeVisibilityRuntime {
public:
    virtual ~IScriptEventVolumeVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t EventVolumeManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindVolume(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t Deactivate(
        std::uint32_t arena, std::uint32_t volume) = 0;
    [[nodiscard]] virtual std::uint32_t Activate(
        std::uint32_t arena, std::uint32_t volume) = 0;
};

class ScriptEventVolumeVisibility final {
public:
    // IDA 0x5d6670, giEventVolumeVisible.
    [[nodiscard]] static std::uint32_t Set(
        std::uint32_t name_payload, std::uint8_t visible,
        IScriptEventVolumeVisibilityRuntime& runtime);
};

}  // namespace pal4
