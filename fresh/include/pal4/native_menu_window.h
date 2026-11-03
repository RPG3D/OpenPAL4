#pragma once
#include "pal4/menu_window_visibility.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;
class NativeEventHandlerArray;
// Shares the UI session's original menuWindow.xml/DLL popup, with a separate
// command owner per facade. UI/activity outlive Close; commands may reopen it.
class NativeMenuWindow final {
public:
    using Command=std::function<void(std::string_view,std::uint32_t,std::uint32_t)>;
    NativeMenuWindow(NativeCeguiUiSession&,NativeEventHandlerArray&,Command);
    ~NativeMenuWindow();
    NativeMenuWindow(const NativeMenuWindow&)=delete;
    NativeMenuWindow& operator=(const NativeMenuWindow&)=delete;
    void AddButton(std::string_view command,std::uint32_t first,std::uint32_t second,
                   std::string_view utf8_label,bool enabled);
    void Show(MenuWindowPoint);
    void HideAndCleanup();
    // Host command-tree owners route the registered original command here.
    void DispatchCommand(std::string_view name);
    void Close();
    void RethrowEventError();
    std::uint32_t FrameAddress()const noexcept;
    std::uint32_t Button(std::uint32_t index)const;
    std::uint32_t ButtonCount()const noexcept;
    bool Visible()const noexcept;
private:
    class Impl;
    struct Client;
    std::shared_ptr<Impl> impl_;
    std::shared_ptr<Client> client_;
    void Prepare();
    bool OwnsPopup()const noexcept;
};
}
