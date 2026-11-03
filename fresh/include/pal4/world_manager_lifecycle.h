#pragma once

#include "pal4/world_manager_event_subscriptions.h"
#include "pal4/world_manager_scene_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IWorldManagerLifecycleRuntime : public IWorldManagerSceneRuntime,
                                     public IWorldManagerEventRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) override = 0;
    virtual void ConstructBaseManager(std::uint32_t manager) = 0;
    virtual void DestroyBaseManager(std::uint32_t manager) = 0;
    virtual void CleanupManager(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void FreeTextStorage(std::uint32_t address) = 0;
};

class WorldManagerLifecycle final {
public:
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t manager, IWorldManagerLifecycleRuntime& runtime);
    static void Destroy(std::uint32_t manager,
                        IWorldManagerLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t ActorData(std::uint32_t manager);

private:
    static void ReleaseText(std::uint32_t descriptor,
                            IWorldManagerLifecycleRuntime& runtime);
};

}  // namespace pal4
