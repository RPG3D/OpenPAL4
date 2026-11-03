#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;
struct NativeCombatSequence;
struct NativeCombatTooltipData;
struct NativeCombatSystemImage;
// Borrows the session's original Main layout. Reuses54B4E0/54C810 for game
// construction, with modern text/vector ownership and a real DLL root click.
// Close before UI/DLL retirement. This does not install frame game events.
class NativeCombatMainWindow final {
public:
    using ClickHandler=std::function<bool(std::uint32_t)>;
    NativeCombatMainWindow(NativeCeguiUiSession&,const std::filesystem::path& root);
    ~NativeCombatMainWindow();
    NativeCombatMainWindow(const NativeCombatMainWindow&)=delete;
    NativeCombatMainWindow& operator=(const NativeCombatMainWindow&)=delete;
    std::uint32_t Address() const noexcept;
    std::uint32_t Root() const noexcept;
    std::array<float,4> SequenceBounds() const;
    std::string_view ResetIconTimer() const;
    std::string_view StatusMarkup(std::size_t index) const;
    std::size_t StatusMarkupCount() const noexcept;
    void BindClickHandler(ClickHandler);
    // Scoped consumers can restore the previous real route after diagnostics.
    // Same idle/closed guard as BindClickHandler; never replace during a click.
    [[nodiscard]] ClickHandler ExchangeClickHandler(ClickHandler);
    void RethrowClickError();
    // Shared54CAC0 refresh with actual staged actor data; does not show root.
    void RefreshSequence(const NativeCombatSequence&,std::uint32_t game_state);
    void HandleSequenceEvent(std::uint32_t callback,const NativeCombatSequence&,
        std::uint32_t game_state,std::uint32_t actor=0u);
    void HandlePanelEvent(std::uint32_t callback,std::uint32_t actor=0u,
        std::uint32_t slot=0u,bool player=false);
    bool DisplayTooltip(const NativeCombatTooltipData&,std::uint32_t combat_state,
        const std::array<float,2>& mouse_screen);
    //54CC60 presentation only; actor identity is validated by the event owner.
    void ShowJointAttackHint(std::uint32_t actor,std::uint32_t target,NativeCombatSystemImage&);
    void Close();
private:
    friend class NativeCombatMainFrame;
    void AcquireFrame();
    void ReleaseFrame() noexcept;
    class State;
    class Runtime;
    std::shared_ptr<State> state_;
};
}
