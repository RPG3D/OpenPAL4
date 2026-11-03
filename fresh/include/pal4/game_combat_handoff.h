#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct MonsterBehaviorManagerState;

class IGameCombatHandoffRuntime {
public:
    virtual ~IGameCombatHandoffRuntime() = default;
    [[nodiscard]] virtual std::uint32_t StateController() = 0;
    [[nodiscard]] virtual std::int32_t ControllerReady(
        std::uint32_t controller) = 0;
    virtual void CreateDocument(std::string_view root) = 0;
    [[nodiscard]] virtual std::string ArenaName() = 0;
    [[nodiscard]] virtual std::string ResolveArenaPath(
        std::string_view arena_name, std::uint8_t mode_byte) = 0;
    virtual void ClearStateText() = 0;
    virtual void AddStateText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextAddress() = 0;
    virtual void FinalizeManagerText(MonsterBehaviorManagerState& manager,
                                     std::uint32_t begin,
                                     std::uint32_t end) = 0;
    [[nodiscard]] virtual std::uint32_t StateMonsterBegin() = 0;
    [[nodiscard]] virtual std::uint32_t StateMonsterEnd() = 0;
    virtual void SetCombatWorld(std::string_view node,
                                std::string_view path,
                                std::uint32_t mode,
                                std::int32_t count) = 0;
    virtual void SetCombatMonster(std::uint32_t id,
                                  std::uint32_t value,
                                  std::uint32_t group) = 0;
    virtual void InitializeCombatState(std::uint32_t state_id) = 0;
    [[nodiscard]] virtual std::int32_t DestroyDocument() = 0;
};

class GameCombatHandoffRuntimeStub : public IGameCombatHandoffRuntime {
public:
    std::uint32_t StateController() override;
    std::int32_t ControllerReady(std::uint32_t controller) override;
    void CreateDocument(std::string_view root) override;
    std::string ArenaName() override;
    std::string ResolveArenaPath(std::string_view arena_name,
                                 std::uint8_t mode_byte) override;
    void ClearStateText() override;
    void AddStateText(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t EmptyTextAddress() override;
    void FinalizeManagerText(MonsterBehaviorManagerState& manager,
                             std::uint32_t begin,
                             std::uint32_t end) override;
    std::uint32_t StateMonsterBegin() override;
    std::uint32_t StateMonsterEnd() override;
    void SetCombatWorld(std::string_view node, std::string_view path,
                        std::uint32_t mode, std::int32_t count) override;
    void SetCombatMonster(std::uint32_t id, std::uint32_t value,
                          std::uint32_t group) override;
    void InitializeCombatState(std::uint32_t state_id) override;
    std::int32_t DestroyDocument() override;
};

class GameCombatHandoff final {
public:
    static constexpr std::uint32_t kCombatState = 3u;

    // IDA 0x434e30: move pending manager data into the combat state.
    [[nodiscard]] static std::int32_t Process(
        MonsterBehaviorManagerState& manager, std::int32_t requested_mode,
        IGameCombatHandoffRuntime& runtime);
};

}  // namespace pal4
