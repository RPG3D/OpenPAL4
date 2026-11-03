#pragma once

#include <cstdint>

namespace pal4 {

enum class CombatActionKind : std::uint32_t {
    Magic = 0u,
    Stunt = 1u,
    Property = 2u,
    Flee = 3u,
    Attack = 4u,
    Defend = 5u,
};

class ICombatActionDispatchRuntime {
public:
    virtual ~ICombatActionDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorFromFacet(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t FindMagic(
        std::uint32_t skill_table, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindStunt(
        std::uint32_t skill_table, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindProperty(
        std::uint32_t database, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateTask(
        CombatActionKind kind, std::uint32_t resource,
        std::uint32_t actor, std::uint32_t target,
        std::uint32_t auxiliary) = 0;
    virtual void RegisterTask(std::uint32_t queue,
                              std::uint32_t task) = 0;
    virtual void AssignOwnedChild(std::uint32_t actor,
                                  std::uint32_t task) = 0;
    virtual void ResetActionComponent(std::uint32_t component) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEventRouter() = 0;
    virtual void DispatchStopSelect(std::uint32_t router,
                                    std::uint32_t descriptor,
                                    std::uint32_t event,
                                    std::uint32_t mode) = 0;
};

class CombatActionDispatcher final {
public:
    // IDA 0x57c4b0.
    [[nodiscard]] static std::uint32_t Handle(
        std::uint32_t owner, std::uint32_t event,
        ICombatActionDispatchRuntime& runtime);

private:
    static void CreateAndAssign(CombatActionKind kind,
                                std::uint32_t resource,
                                std::uint32_t actor,
                                std::uint32_t event,
                                bool has_target,
                                ICombatActionDispatchRuntime& runtime);
};

}  // namespace pal4
