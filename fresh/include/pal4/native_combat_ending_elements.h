#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

namespace pal4 {
class NativeCeguiUiSession;
class NativeCombatEndingElementsRuntime;
// Original543F80 on a borrowed720-byte game frame. The caller owns its layout,
// report, hints and task scheduler; this adapter owns only controls/callback.
// Close before the borrowed image, UI session or DLL is retired.
class NativeCombatEndingElements final {
public:
    using MouseHandler=std::function<bool(std::uint32_t)>;
    NativeCombatEndingElements(NativeCeguiUiSession&,std::span<std::uint32_t,180> frame);
    ~NativeCombatEndingElements();
    NativeCombatEndingElements(const NativeCombatEndingElements&)=delete;
    NativeCombatEndingElements& operator=(const NativeCombatEndingElements&)=delete;
    void BindMouseHandler(MouseHandler);
    void RethrowMouseError();
    std::size_t ControlCount() const noexcept;
    std::size_t TemporaryAllocations() const noexcept;
    bool Subscribed() const noexcept;
    void Close();
private:
    friend class NativeCombatEndingElementsRuntime;
    class State;
    std::shared_ptr<State> state_;
};
}
