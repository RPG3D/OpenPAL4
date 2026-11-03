#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>

namespace pal4 {
class NativeCombatActorOwner;
class NativeCombatPlayerActorOwner;
class NativeCombatActorSkillRuntime;
class NativeCameraManagerOwner;
class NativeCeguiUiSession;
class NativeUiTextImageOverlay;

// Native presentation for verified callbacks540020/540270/5408B0/5404B0 and Monster540170/5401F0.
// All dependencies are borrowed. ensure_system must perform the original cold
// CombatSystem acquisition/preflight; no success callback is supplied by default.
// Unbind the console dispatch, then Close, before retiring actor/skill/UI owners.
class NativeCombatConsoleVisualEvents final {
public:
    NativeCombatConsoleVisualEvents(NativeCameraManagerOwner&,NativeCeguiUiSession&,
        NativeUiTextImageOverlay&,std::function<void()> ensure_system,
        std::function<void(std::uint32_t actor,bool failed)> monster_flee_hint);
    ~NativeCombatConsoleVisualEvents();
    NativeCombatConsoleVisualEvents(const NativeCombatConsoleVisualEvents&)=delete;
    NativeCombatConsoleVisualEvents& operator=(const NativeCombatConsoleVisualEvents&)=delete;
    void PrepareResources(const std::filesystem::path& root);
    void BindActor(NativeCombatActorOwner&,NativeCombatActorSkillRuntime&);
    void BindActor(NativeCombatPlayerActorOwner&,NativeCombatActorSkillRuntime&);
    void UnbindActor(std::uint32_t actor);
    [[nodiscard]] static bool Handles(std::uint32_t entry) noexcept;
    void Dispatch(std::uint32_t entry,std::uint32_t payload);
    void Close();
    [[nodiscard]] std::size_t BoundActors() const noexcept;
    [[nodiscard]] std::size_t PendingAllocations() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
