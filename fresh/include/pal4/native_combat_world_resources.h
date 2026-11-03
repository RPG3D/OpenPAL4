#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
#include "pal4/native_combat_world_renderer.h"
#include "pal4/game_world_object.h"

namespace pal4 {
class NativePkFileManagerRuntime;
class NativePalGameIVTextureStorage;
class NativeWorldAsset;
class INativeCombatWorldFrameServices;
// Typed scene subobject at WorldManager/CombatSystem+116. Never relocate a
// constructed image: its dictionaries and queues contain pointers into it.
struct NativeCombatWorldImage final {
    GameWorldObjectState base;
    std::array<std::uint32_t,9> tail{};
};
static_assert(sizeof(NativeCombatWorldImage)==232u);
// Owns WorldManager's embedded232-byte scene and original combat CPK resources.
// Loading prepares resources; explicit pipeline installation owns original draw
// callbacks. This does not enter Combat state3 or attach/activate a camera.
class NativeCombatWorldResources final {
public:
    NativeCombatWorldResources(NativePkFileManagerRuntime&, NativePalGameIVTextureStorage&);
    // Storage must outlive this owner. Resource/queue destruction is in-place;
    // the containing object is never freed. Only one owner may bind an image.
    NativeCombatWorldResources(NativePkFileManagerRuntime&, NativePalGameIVTextureStorage&,
                              NativeCombatWorldImage& storage);
    ~NativeCombatWorldResources();
    NativeCombatWorldResources(const NativeCombatWorldResources&) = delete;
    NativeCombatWorldResources& operator=(const NativeCombatWorldResources&) = delete;
    void Load(const std::filesystem::path& game_root, std::string_view region,
              std::string_view scene);
    void Reset();
    //53F3B0 tail: shared GameWorld camera removal/addition, real SDK membership.
    // Does not activate the global camera or borrow ownership of its lifetime.
    bool AttachCamera(std::uint32_t camera);
    void InstallRenderPipeline(std::uint32_t effect);
    void Render();
    //661890: actual six material schemas; preserves world-ready48 gate.
    bool UpdateRenderParameters(float elapsed);
    // Pins world/pipeline/queues across UV update and synchronous input events.
    class FrameLease final {
    public:
        FrameLease(const FrameLease&)=delete;
        FrameLease& operator=(const FrameLease&)=delete;
        ~FrameLease();
        bool UpdateRenderParameters(float elapsed);
    private:
        friend class NativeCombatWorldResources;
        explicit FrameLease(NativeCombatWorldResources&);
        NativeCombatWorldResources& owner_;
    };
    [[nodiscard]] FrameLease BorrowFrame();
    // Original53F740 order; caller begins this world's camera. Actor services
    // must provide both real phases. Render() remains the world-only boundary.
    std::uint32_t RenderFrame(std::uint32_t context,INativeCombatWorldFrameServices&);
    // WorldManager+328 == embedded scene+212; scenery queue is scene+196.
    // Restore borrowed model callbacks before destroying this owner.
    [[nodiscard]] std::uint32_t GeometryQueue() const;
    void ClearGeometryQueues();
    [[nodiscard]] std::size_t GeometryAllocations() const noexcept;
    [[nodiscard]] bool RenderPipelineReady() const noexcept;
    [[nodiscard]] NativeCombatWorldRenderStats RenderStats() const noexcept;
    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] bool Loaded() const noexcept;
    [[nodiscard]] bool LightmapConfigLoaded() const noexcept;
    // Same global layout as53ED30: brightness first, then RGB saturation.
    [[nodiscard]] std::array<float,4> LightmapParameters() const noexcept;
    [[nodiscard]] const NativeWorldAsset& Asset() const noexcept;
    [[nodiscard]] std::size_t TextAllocations() const noexcept;
private:
    friend class NativeCombatWorldSceneEvents;
    void BindSceneEvents();
    void UnbindSceneEvents() noexcept;
    void RequireSceneEventDispatch() const;
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
