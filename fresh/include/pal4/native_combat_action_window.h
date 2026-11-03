#pragma once
#include "pal4/native_combat_selector_layout.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
class NativeEventHandlerArray;
// Original Property window80/84/88 survives for the UI session. Host battle
// controllers retire separately; only these value IDs cross that boundary.
struct NativeCombatPropertySelectionHistory {
    std::array<std::uint32_t,3> preferred_properties{};
};
// Owns the recovered Action console image, embedded selectors and DLL events.
// Controllers and actual roster availability are bound separately.
class NativeCombatActionWindow final {
public:
    using Handler=std::function<bool(std::uint32_t,std::uint32_t)>;
    using Property=std::function<std::uint8_t(std::uint32_t,std::uint32_t)>;
    // Optional history is borrowed and must outlive the window. Standalone
    // probes may omit it to retain original cold-constructor defaults.
    NativeCombatActionWindow(NativeCeguiUiSession&,const std::filesystem::path&,NativeEventHandlerArray&,
        NativeCombatPropertySelectionHistory* history=nullptr);
    ~NativeCombatActionWindow();
    NativeCombatActionWindow(const NativeCombatActionWindow&)=delete;
    NativeCombatActionWindow& operator=(const NativeCombatActionWindow&)=delete;
    std::uint32_t Address()const noexcept;
    std::uint32_t Root()const noexcept;
    std::uint32_t Button(unsigned)const;
    std::uint32_t& Word(std::uint32_t);
    NativeCombatSelectorLayout& Selector(NativeCombatSelectorKind);
    std::size_t SubscriptionCount()const noexcept;
    std::size_t TemporaryAllocations()const noexcept;
    void BindHandler(Handler);
    void BindAvailability(Property,std::function<bool()> has_magic);
    void SetVisible(std::uint32_t context,bool);
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t InvokeVirtual(std::uint32_t object,std::uint32_t slot);
    std::uint8_t InvokePredicate(std::uint32_t object,std::uint32_t slot,std::uint32_t event);
    void RethrowEventError();
    void Close();
private:
    friend class NativeCombatActionFrame;
    friend class NativeCombatActionController;
    void AcquireController();
    void ReleaseController()noexcept;
    void BindControlHandler(Handler);
    void AcquireFrame();
    void ReleaseFrame()noexcept;
    void BindFrameParent(bool,std::uint32_t);
    class State;class Runtime;
    std::shared_ptr<State> state_;
};
}
