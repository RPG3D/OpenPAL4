#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct MonsterBehaviorManagerState;

struct MonsterDistanceTriggerState {
    std::uint32_t active{};
    std::uint32_t kind{};
    std::array<float, 3> position{};
};
static_assert(sizeof(MonsterDistanceTriggerState) == 20u);

class IMonsterDistanceTriggerRuntime {
public:
    virtual ~IMonsterDistanceTriggerRuntime() = default;
    [[nodiscard]] virtual double Distance(
        const std::array<float, 3>& delta) = 0;
    virtual void NotifyAt564(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string AnimationName(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool NameEqualsIgnoreCase(
        std::string_view left, std::string_view right) = 0;
    [[nodiscard]] virtual bool SelectAnimation(
        std::uint32_t address, std::string_view name) = 0;
};

class MonsterDistanceTriggerRuntimeStub final
    : public IMonsterDistanceTriggerRuntime {
public:
    double Distance(const std::array<float, 3>& delta) override;
    void NotifyAt564(std::uint32_t address) override;
    std::string AnimationName(std::uint32_t address) override;
    bool NameEqualsIgnoreCase(std::string_view left,
                              std::string_view right) override;
    bool SelectAnimation(std::uint32_t address,
                         std::string_view name) override;
};

class MonsterDistanceTrigger final {
public:
    // IDA 0x435150: manager pursuit-distance callback.
    [[nodiscard]] static std::uint32_t Process(
        MonsterDistanceTriggerState& trigger,
        std::uint32_t trigger_address,
        MonsterBehaviorManagerState& manager,
        std::uint32_t manager_address,
        IMonsterDistanceTriggerRuntime& runtime);
};

}  // namespace pal4
