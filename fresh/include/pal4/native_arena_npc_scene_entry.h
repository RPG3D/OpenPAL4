#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

namespace pal4 {

class NativeArenaCollisionTree;
class NativeMonsterManagerOwner;
class NativeNpcManagerOwner;
class NativeSceneMovementAsset;
class IBaseObjectNamedWeaponRuntime;
class IBaseObjectAttachedEffectRuntime;

class NativeArenaNpcSceneEntry final {
public:
    ~NativeArenaNpcSceneEntry();
    NativeArenaNpcSceneEntry();
    NativeArenaNpcSceneEntry(const NativeArenaNpcSceneEntry&) = delete;
    NativeArenaNpcSceneEntry& operator=(const NativeArenaNpcSceneEntry&) = delete;

    [[nodiscard]] std::uint32_t BindAll(
        NativeNpcManagerOwner& manager, std::uint32_t arena,
        NativeSceneMovementAsset& movement,
        NativeArenaCollisionTree& collision);
    [[nodiscard]] std::uint32_t BindMonsters(
        NativeMonsterManagerOwner& manager, std::uint32_t arena,
        NativeSceneMovementAsset& movement,
        NativeArenaCollisionTree& collision);
    [[nodiscard]] std::uint32_t LeaveScriptNpc(std::uint32_t npc);
    [[nodiscard]] std::uint32_t EnterScriptNpc(std::uint32_t npc);
    //5FF210 only: remove movement registration while retaining scene/world
    // entry. Keep the creating entry's teardown bookkeeping in agreement.
    void UnregisterMovement(std::uint32_t npc);
    void Clear();
    // Borrowed setup/cleanup runtime; bind before scripts attach hand resources.
    void BindWeaponRuntime(IBaseObjectNamedWeaponRuntime*);
    void BindAttachedEffectRuntime(IBaseObjectAttachedEffectRuntime*);
    [[nodiscard]] std::size_t Count() const noexcept;

private:
    struct EntryState;
    std::vector<std::unique_ptr<EntryState>> entries_;
    IBaseObjectNamedWeaponRuntime* weapons_{};
    IBaseObjectAttachedEffectRuntime* attached_{};
};

}  // namespace pal4
