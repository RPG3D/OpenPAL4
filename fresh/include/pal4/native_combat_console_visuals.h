#pragma once
#include "pal4/combat_result_icon_creator.h"
#include <cstddef>
#include <memory>
namespace pal4 {
class NativeCombatConsole;
class NativeUiTextImageOverlay;
class NativeMilesSound2dRuntime;struct NativeCombatSystemImage;
class NativeCombatEndingWindow;
// Sole animation allocator/activity binding for ready portraits, Ending and defeat.
// Console, overlay and sound owners outlive this object. Close before retiring UI.
class NativeCombatConsoleVisuals final : public ICombatResultIconRuntime {
public:
    NativeCombatConsoleVisuals(NativeCombatConsole&,NativeUiTextImageOverlay&,NativeMilesSound2dRuntime&,NativeCombatSystemImage&);
    ~NativeCombatConsoleVisuals();
    void ShowVictory();
    //57A920 original delayed72-byte defeat image on the same owned console queue.
    void ShowDefeat(std::string_view imageset,std::string_view image,float scale,float x,float y);
    void ShowRank(std::uint32_t child,NativeCombatEndingWindow&);
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t ConstructFontElement(std::uint32_t) override;
    std::uint32_t ConstructPulse(std::uint32_t);
    void SetDimensions(std::uint32_t,float,float) override;
    void SetImageset(std::uint32_t,std::string_view,std::string_view,float) override;
    void SetFont(std::uint32_t,std::string_view,std::string_view,float) override;
    void SetFontProperties(std::uint32_t,float,float) override;
    void SetPulseTiming(std::uint32_t,float,float,float,float);
    std::uint32_t ElementList() override;
    void AddElement(std::uint32_t,bool,std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    void PlaySound(std::string_view,std::uint32_t) override;
    void DiscardPending();
    void Close();
    std::size_t OutstandingAllocations() const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
