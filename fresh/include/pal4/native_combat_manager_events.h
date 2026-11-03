#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
namespace pal4 {
class NativeEffectSceneSession;
class NativeScriptEventRouter;
// Borrowed original child172/160 vectors; resolver validates live actor/model
// ownership before returning. Only actual Player owners provide selection1192.
struct NativeCombatEventActor {
    std::uint32_t kind{},position{},rotation{};
    std::optional<std::uint32_t> selection;
};
// Owns ten original ActorManager subscriptions and tail304..399 in an already
// bound400-byte Combat effect group. Game-object260..303 is a separate owner.
// Session/router/combat520 and actor views must outlive this service. Close
// before group unbind. Uses shared prepare/selection/preview algorithms.
class NativeCombatManagerEvents final {
public:
    using ResolveActor=std::function<NativeCombatEventActor(std::uint32_t)>;
    NativeCombatManagerEvents(std::uint32_t manager,std::uint32_t combat,
        NativeEffectSceneSession&,NativeScriptEventRouter&,ResolveActor);
    ~NativeCombatManagerEvents();
    NativeCombatManagerEvents(const NativeCombatManagerEvents&)=delete;
    NativeCombatManagerEvents& operator=(const NativeCombatManagerEvents&)=delete;
    void Subscribe();
    void Close();
    [[nodiscard]] std::uint32_t SubscriptionCount() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
