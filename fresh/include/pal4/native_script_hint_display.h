#pragma once
#include "pal4/script_hint_display.h"

namespace pal4 {
class NativeCeguiUiSession;
// Borrows the shared HintWindow owner and uses the installed CEGUI string codec.
class NativeScriptHintDisplay final : public IScriptHintDisplayRuntime {
public:
    explicit NativeScriptHintDisplay(NativeCeguiUiSession& ui) : ui_(ui) {}
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t EmptyStringAddress() override;
    std::string ConvertToUtf8(std::uint32_t) override;
    std::uint32_t EncodedSize(std::string_view) override;
    std::u32string Encode(std::string_view, std::uint32_t) override;
    std::uint32_t UiFrameManager() override;
    std::uint32_t CreateHintWindow(std::uint32_t) override;
    void ShowHint(std::uint32_t, std::u32string_view, float, float, float,
                  std::uint32_t, std::uint32_t) override;
private:
    NativeCeguiUiSession& ui_;
};
}
