#pragma once

#include "pal4/world_manager_scene_initializer.h"

#include <cstdint>

namespace pal4 {

class IWorldManagerCleanupRuntime : public IWorldManagerSceneInitializationRuntime {
public:
    virtual void CompactAtomicList(std::uint32_t list) = 0;
    virtual void CleanupWorld(std::uint32_t world,
                              std::uint32_t game_context) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeTextStorage(std::uint32_t address) = 0;
    virtual void ReleaseMatFxVm(std::uint32_t vm) = 0;
};

class WorldManagerCleanup final {
public:
    [[nodiscard]] static std::uint32_t Run(
        std::uint32_t manager, IWorldManagerCleanupRuntime& runtime);

private:
    static void ClearText(std::uint32_t descriptor,
                          IWorldManagerCleanupRuntime& runtime);
};

}  // namespace pal4
