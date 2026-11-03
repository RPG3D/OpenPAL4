#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;
class NativeEventHandlerArray;
// Owns the original SelectDialog layout and its reusable answer panels.
class NativeSelectDialogLayout final {
public:
    NativeSelectDialogLayout(NativeCeguiUiSession&, const std::filesystem::path&,
                            NativeEventHandlerArray&, std::uint32_t manager);
    ~NativeSelectDialogLayout();
    std::uint32_t FrameAddress() const noexcept;
    std::uint32_t RootAddress() const noexcept;
    std::uint32_t Window(std::uint32_t offset) const;
    void AddItem(std::string_view game_text);
    void SetDefaultSelection(std::int32_t);
    std::int32_t LastSelection() const;
    void Show(std::string_view question);
    bool Visible() const noexcept;
    bool OwnsActivity(std::uint32_t) const noexcept;
    std::uint32_t Callback(std::uint32_t, std::uint32_t);
    std::uint8_t Predicate(std::uint32_t, std::uint32_t, std::uint32_t);
    void RethrowEventError();
    void Close();
private:
    class Impl; std::unique_ptr<Impl> impl_;
};
}
