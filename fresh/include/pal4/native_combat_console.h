#pragma once
#include <cstdint>
#include <functional>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeScriptEventRouter;
class IAnimationEventQueueActivityRuntime;

// Owns the original252-byte cbCombatConsole image/queue and subscriptions.
// These borrowed services and dispatched callback owners must outlive it.
class NativeCombatConsole final {
public:
    using Dispatch = std::function<void(std::uint32_t console, std::uint32_t entry,
                                       std::uint32_t payload)>;
    NativeCombatConsole(NativeBaseObjectMemory&, NativeScriptEventRouter&);
    ~NativeCombatConsole();
    NativeCombatConsole(const NativeCombatConsole&) = delete;
    NativeCombatConsole& operator=(const NativeCombatConsole&) = delete;
    [[nodiscard]] std::uint32_t Construct();
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept;
    void Subscribe();
    void Unsubscribe() noexcept;
    // Clear owned/borrowed animation queues, preserving all event subscriptions.
    // Start separately unsubscribes after camera defaults, as in5756D0.
    void Reset();
    void BindDispatch(Dispatch);
    void BindAnimationActivity(IAnimationEventQueueActivityRuntime*);
    void AddAnimation(std::uint32_t animation, bool owned, bool delay);
    void Update(float delta);
    //Original4C3210, called after UI rendering in576220.
    void Render(std::uint32_t context);
    bool AnimationActivitiesBound() const noexcept;
    [[nodiscard]] std::uint32_t SubscriptionCount() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
