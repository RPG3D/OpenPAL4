#pragma once

#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

namespace pal4 {
class NativeCeguiUiSession;

// Owns original CommonDialog.xml or CommonDialog2.xml and host subscribers.
// Destroy before the borrowed UI session/DLL. Loading leaves the root hidden.
// Show composes native text, modal navigation and copied selection callbacks;
// the caller supplies audio policy and the resulting game action.
class NativeCommonDialogLayout final {
public:
    enum class Kind { common, yes_no };
    using EventHandler = std::function<bool(bool key_down, std::uint32_t event)>;
    using SelectionHandler = std::function<void(std::uint32_t)>;
    using SoundPlayer = std::function<void(std::string_view, std::uint32_t)>;
    NativeCommonDialogLayout(HMODULE module, NativeCeguiUiSession& ui,
                             const std::filesystem::path& game_root, Kind kind = Kind::common);
    ~NativeCommonDialogLayout();
    NativeCommonDialogLayout(const NativeCommonDialogLayout&) = delete;
    NativeCommonDialogLayout& operator=(const NativeCommonDialogLayout&) = delete;
    [[nodiscard]] std::uint32_t RootAddress() const;
    [[nodiscard]] std::uint32_t Window(std::string_view suffix) const;
    void SetEventHandler(EventHandler handler);
    // Independent original CommonDialog2/YesNoDialog; results are Yes1/No0.
    void ShowYesNo(std::string_view game_text, SoundPlayer sound);
    [[nodiscard]] std::uint32_t FrameAddress() const;
    [[nodiscard]] bool Visible() const noexcept;
    void RethrowEventError();
    // Copies the selection callback. Audio policy belongs to the game caller.
    // Replaces a previous prompt like CreateCommonDialog/ShowWithFlags.
    void Show(std::u32string_view text, std::uint8_t flags,
              SelectionHandler callback, SoundPlayer sound);
    void Close();
    void ReleaseSelectionCallback();
    // A prompt owner may retire only its own callback when sharing this dialog.
    [[nodiscard]] std::uint64_t SelectionGeneration() const noexcept;
    [[nodiscard]] std::uint32_t LastSelection() const;
private:
    struct State;
    class Runtime;
    std::shared_ptr<State> state_;
};
} // namespace pal4
