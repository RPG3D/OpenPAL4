#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatSystemColdRuntime {
public:
    virtual ~ICombatSystemColdRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void LogInitialization(bool beginning) = 0;
    [[nodiscard]] virtual std::uint32_t LogInitialized() = 0;
    virtual void GetSkillSystem() = 0;
    virtual void LoadTranslator(std::uint32_t translator,
                                std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t GetConsole() = 0;
    virtual void SubscribeConsoleEvents(std::uint32_t console) = 0;
    virtual void InitializeMatFx(std::uint32_t object) = 0;
};

class ICombatSystemSessionRuntime : public ICombatSystemColdRuntime {
public:
    ~ICombatSystemSessionRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void EndCombat(std::uint32_t object) = 0;
    virtual void EndCombatSequence(std::uint32_t object,
                                   std::uint32_t mode) = 0;
    virtual void SetRenderObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetActorManager() = 0;
    virtual void ResetTaskFlags(std::uint32_t queue) = 0;
    virtual void CleanupTaskQueue(std::uint32_t queue) = 0;
    virtual void DeinitializeGameObject(std::uint32_t object) = 0;
    virtual void DestructActorManager(std::uint32_t manager) = 0;
    virtual void DestroyCombatDataRange(std::uint32_t begin,
                                        std::uint32_t end) = 0;
    virtual void DestroyConsole(std::uint32_t console) = 0;
    virtual void CleanupWorld(std::uint32_t object) = 0;
    virtual void DestructTranslator(std::uint32_t translator) = 0;
    virtual void InitializeGameStats(std::uint32_t handler) = 0;
};

class CombatSystemSession final {
public:
    // IDA 0x5754c0, 0x575570 and 0x5756b0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, ICombatSystemColdRuntime& runtime);
    static void End(std::uint32_t object,
                    ICombatSystemSessionRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetEndState(
        std::uint32_t object, std::uint32_t end_value,
        ICombatSystemSessionRuntime& runtime);
};

}  // namespace pal4
