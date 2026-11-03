#pragma once

#include <cstdint>

namespace pal4 {

class ICombatSystemEndRuntime {
public:
    virtual ~ICombatSystemEndRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void DispatchActorEnd(std::uint32_t object) = 0;
    virtual void DeinitializeGameObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetActorManager() = 0;
    virtual void DestructActorManager(std::uint32_t manager) = 0;
    virtual void ClearCombatRosters(){DestructActorManager(GetActorManager());}
    [[nodiscard]] virtual std::uint32_t GetConsole() = 0;
    virtual void ResetConsole(std::uint32_t console) = 0;
    virtual void SetRenderCallback(std::uint32_t callback) = 0;
    virtual void InitializeStrings(std::uint32_t block) = 0;
};

class CombatSystemEndCombat final {
public:
    // IDA 0x575ec0 forwards to 0x575ed0.
    [[nodiscard]] static std::uint32_t EndWrapper(
        std::uint32_t object, ICombatSystemEndRuntime& runtime);
    // IDA 0x575ed0.
    [[nodiscard]] static std::uint32_t End(
        std::uint32_t object, ICombatSystemEndRuntime& runtime);
};

}  // namespace pal4
