#pragma once

#include <cstdint>

namespace pal4 {

class IWorldManagerRenderRuntime {
public:
    virtual ~IWorldManagerRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void RenderWorld(std::uint32_t world,
                             std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    virtual void ProcessActorUpdates(std::uint32_t actor_manager,
                                     std::uint32_t context) = 0;
    virtual void RenderClump(std::uint32_t clump) = 0;
    virtual void SortAtomicGeometries(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateActorComponents(
        std::uint32_t actor_manager, std::uint32_t context) = 0;
};

class WorldManagerRenderer final {
public:
    [[nodiscard]] static std::uint32_t Render(
        std::uint32_t manager, std::uint32_t context_pointer,
        IWorldManagerRenderRuntime& runtime);
    // Same algorithm for an independently owned embedded scene (+116).
    [[nodiscard]] static std::uint32_t RenderScene(
        std::uint32_t scene, std::uint32_t context_pointer,
        IWorldManagerRenderRuntime& runtime);
};

}  // namespace pal4
