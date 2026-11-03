#pragma once

#include <cstdint>
#include <vector>

namespace pal4 {

class IActorManagerPlayerMaintenanceRuntime {
public:
    virtual ~IActorManagerPlayerMaintenanceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool ProcessAIActionTask(std::uint32_t task) = 0;
    virtual void CleanupObjectLists(std::uint32_t actor) = 0;
    virtual void ProcessInventory(std::uint32_t slots,
                                  std::vector<std::uint32_t>& items) = 0;
    virtual void ProcessIndexedCollection(
        std::uint32_t collection,
        std::vector<std::uint32_t>& items) = 0;
    virtual void ProcessSortedValues(std::uint32_t collection,
                                     std::vector<std::uint32_t>& items) = 0;
    virtual void HandleGameObjectFade(std::uint32_t receiver,
                                      std::uint32_t item,
                                      std::uint32_t actor,
                                      std::vector<std::uint32_t>& items) = 0;
};

class ActorManagerPlayerMaintenance final {
public:
    [[nodiscard]] static bool ProcessTask(
        std::uint32_t actor, IActorManagerPlayerMaintenanceRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupData(
        std::uint32_t actor, IActorManagerPlayerMaintenanceRuntime& runtime);
};

}  // namespace pal4
