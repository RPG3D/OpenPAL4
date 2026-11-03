#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

namespace pal4 {
class NativeCeguiUiSession;
// Borrows the original Role layout and524-byte surrounding frame storage.
// Implements54F940 only. Nine script subscriptions, timers and actor/stat
// refresh must be supplied by a real game frame before showing this layout.
class NativeCombatRoleStateElements final {
public:
    using Handler=std::function<bool(std::uint32_t method,std::uint32_t event)>;
    NativeCombatRoleStateElements(NativeCeguiUiSession&,std::span<std::uint32_t,131>);
    ~NativeCombatRoleStateElements();
    NativeCombatRoleStateElements(const NativeCombatRoleStateElements&)=delete;
    NativeCombatRoleStateElements& operator=(const NativeCombatRoleStateElements&)=delete;
    void BindHandler(Handler);
    void RethrowError();
    std::size_t ControlCount()const noexcept;
    std::size_t SubscriptionCount()const noexcept;
    std::size_t TemporaryAllocations()const noexcept;
    void Close();
private:
    friend class NativeCombatRoleStateElementsRuntime;
    class State;std::shared_ptr<State> state_;
};
}
