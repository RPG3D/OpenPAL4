#pragma once
#include "pal4/native_common_dialog_layout.h"
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class DialogManagerState;
class IDialogManagerSettingsWriteRuntime;
class NativeEventHandlerArray;
// Borrows the original Load controls; owns numbered rows and confirmation
// callbacks. Retire before the borrowed UI, settings and database.
class NativeLoadWindow final {
public:
    using DialogFactory = std::function<std::shared_ptr<NativeCommonDialogLayout>()>;
    NativeLoadWindow(NativeCeguiUiSession&, const std::filesystem::path&,
        const NativeGameDatabase&, DialogManagerState&, IDialogManagerSettingsWriteRuntime&,
        NativeEventHandlerArray&, std::uint32_t frame_manager,
        DialogFactory, NativeCommonDialogLayout::SoundPlayer,
        std::function<void(std::int32_t)>, std::function<void()> close);
    ~NativeLoadWindow();
    void Populate(std::int32_t page, bool refresh);
    void SetVisible(bool);
    [[nodiscard]] bool OwnsActivity(std::uint32_t) const;
    [[nodiscard]] std::uint32_t Callback(std::uint32_t slot);
    [[nodiscard]] std::uint8_t Predicate(std::uint32_t slot,std::uint32_t event);
    [[nodiscard]] std::int32_t HoveredSlot() const;
    void Click();
    void Cancel();
    [[nodiscard]] bool ConfirmationVisible() const;
    void RethrowEventError();
private:
    class Runtime;
    std::shared_ptr<Runtime> runtime_;
};
}
