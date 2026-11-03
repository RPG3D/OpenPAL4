#pragma once
#include "pal4/native_combat_manager_events.h"
#include <array>
#include <cstdint>
#include <memory>
namespace pal4 {
struct NativeCombatSystemImage;
// Stable original400-byte ActorManager storage. Borrowed by the session's
// EffectSystem/game-object layer; it must outlive that session on failure.
struct NativeCombatActorManagerImage final {
    std::array<std::uint32_t,100> words{};
    NativeCombatActorManagerImage()=default;
    NativeCombatActorManagerImage(const NativeCombatActorManagerImage&)=delete;
    NativeCombatActorManagerImage& operator=(const NativeCombatActorManagerImage&)=delete;
};
static_assert(sizeof(NativeCombatActorManagerImage)==400u);
// Composes the existing native EffectSystem, game-object and ten event owners.
// CloseEvents before actors retire; Close after Scene/UI effect payloads drain,
// before world/camera/SDK retirement. Close failure retains storage/publication
// for retry. Construction does not initialize the whole Combat state.
class NativeCombatActorManagerOwner final {
public:
    NativeCombatActorManagerOwner(NativeCombatActorManagerImage&,NativeCombatSystemImage&,
        NativeEffectSceneSession&,NativeScriptEventRouter&,NativeCombatManagerEvents::ResolveActor);
    ~NativeCombatActorManagerOwner();
    NativeCombatActorManagerOwner(const NativeCombatActorManagerOwner&)=delete;
    NativeCombatActorManagerOwner& operator=(const NativeCombatActorManagerOwner&)=delete;
    void Initialize(std::uint32_t game,std::uint32_t camera);
    void CloseEvents();
    void Close();
    std::uint32_t Address() const noexcept;
    std::uint32_t SubscriptionCount() const;
private:
    NativeCombatActorManagerImage& image_;
    NativeCombatSystemImage& combat_;
    NativeEffectSceneSession& effects_;
    std::unique_ptr<NativeCombatManagerEvents> events_;
    bool bound_{};
};
}
