#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeCombatActorModelOwner;
class NativeEffectSceneSession;
class NativeScriptEventRouter;

class INativeCombatFinalHitMessage {
public:
    virtual ~INativeCombatFinalHitMessage() = default;
    virtual void Start(std::uint32_t task) = 0;
    [[nodiscard]] virtual bool Complete(std::uint32_t task) = 0;
    virtual void Stop(std::uint32_t task) = 0;
};

// The actor, effects and router outlive this owner and all its tasks.
class NativeCombatFinalHitMessage final : public INativeCombatFinalHitMessage {
public:
    NativeCombatFinalHitMessage(NativeCombatActorModelOwner&, NativeEffectSceneSession&,
                                NativeScriptEventRouter&);
    ~NativeCombatFinalHitMessage();
    NativeCombatFinalHitMessage(const NativeCombatFinalHitMessage&) = delete;
    NativeCombatFinalHitMessage& operator=(const NativeCombatFinalHitMessage&) = delete;
    void Start(std::uint32_t task) override;
    [[nodiscard]] bool Complete(std::uint32_t task) override;
    void Stop(std::uint32_t task) override;
    [[nodiscard]] std::size_t Count() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
