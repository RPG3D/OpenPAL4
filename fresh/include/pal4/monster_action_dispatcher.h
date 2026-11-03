#pragma once

#include <cstdint>

namespace pal4 {

class IMonsterActionDispatchRuntime {
public:
    virtual ~IMonsterActionDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ManagerInstance() = 0;
    [[nodiscard]] virtual std::uint32_t IterateAndCallback(
        std::uint32_t instance_address,
        std::uint32_t callback_address,
        std::uint32_t context_address) = 0;
};

class MonsterActionDispatchRuntimeStub final
    : public IMonsterActionDispatchRuntime {
public:
    std::uint32_t ManagerInstance() override;
    std::uint32_t IterateAndCallback(std::uint32_t instance_address,
        std::uint32_t callback_address,
        std::uint32_t context_address) override;
};

class MonsterActionDispatcher final {
public:
    static constexpr std::uint32_t kObjectInteractionCallback = 0x434600u;
    // IDA 0x4345e0: dispatch the current manager to the monster collection.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t manager_address,
        IMonsterActionDispatchRuntime& runtime);
};

}  // namespace pal4
