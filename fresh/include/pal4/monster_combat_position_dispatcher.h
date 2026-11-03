#pragma once

#include "pal4/monster_action_dispatcher.h"

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

struct MonsterBehaviorManagerState;

class IMonsterCombatPositionRuntime {
public:
    virtual ~IMonsterCombatPositionRuntime() = default;
    [[nodiscard]] virtual std::int32_t PositionCount(
        std::uint32_t source_address) = 0;
    [[nodiscard]] virtual std::uint32_t PositionId(
        std::uint32_t source_address, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t PositionValue(
        std::uint32_t source_address, const std::string& key,
        std::int32_t fallback) = 0;
    virtual void AppendRecord(
        MonsterBehaviorManagerState& state,
        std::uint32_t array_address, std::uint32_t end_address,
        std::uint32_t count,
        const std::array<std::uint32_t, 3>& record) = 0;
    virtual void AddManagerText(std::uint32_t manager_address,
                                std::uint32_t text_address) = 0;
    [[nodiscard]] virtual IMonsterActionDispatchRuntime& ActionRuntime() = 0;
};

class MonsterCombatPositionRuntimeStub final
    : public IMonsterCombatPositionRuntime {
public:
    std::int32_t PositionCount(std::uint32_t source_address) override;
    std::uint32_t PositionId(std::uint32_t source_address,
                             std::uint32_t index) override;
    std::int32_t PositionValue(std::uint32_t source_address,
        const std::string& key, std::int32_t fallback) override;
    void AppendRecord(MonsterBehaviorManagerState& state,
        std::uint32_t array_address, std::uint32_t end_address,
        std::uint32_t count,
        const std::array<std::uint32_t, 3>& record) override;
    void AddManagerText(std::uint32_t manager_address,
                        std::uint32_t text_address) override;
    IMonsterActionDispatchRuntime& ActionRuntime() override;

private:
    MonsterActionDispatchRuntimeStub action_{};
};

class MonsterCombatPositionDispatcher final {
public:
    static constexpr std::uint32_t kCallback = 0x434810u;
    // IDA 0x434720: collect combat positions and iterate monster objects.
    [[nodiscard]] static std::uint32_t Process(
        MonsterBehaviorManagerState& state, std::uint32_t manager_address,
        IMonsterCombatPositionRuntime& runtime);
};

}  // namespace pal4
