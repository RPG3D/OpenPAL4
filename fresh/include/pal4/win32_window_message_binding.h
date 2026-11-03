#pragma once

#include "pal4/window_message_processor.h"

#include <cstdint>

namespace pal4 {

// Install on the window-owning thread before CreateWindowExA; keep it alive
// until every window using ProcedureAddress has been destroyed.
class Win32WindowMessageBinding final {
public:
    Win32WindowMessageBinding(WindowMessageProcessor& processor,
                              IWindowMessageRuntime& runtime);
    ~Win32WindowMessageBinding();
    Win32WindowMessageBinding(const Win32WindowMessageBinding&) = delete;
    Win32WindowMessageBinding& operator=(
        const Win32WindowMessageBinding&) = delete;

    [[nodiscard]] static std::uint32_t ProcedureAddress();
    [[nodiscard]] static std::uint32_t DispatchCurrent(
        std::uint32_t window, std::uint32_t message,
        std::uint32_t wparam, std::uint32_t lparam);

private:
    [[nodiscard]] std::uint32_t Dispatch(std::uint32_t window,
                                         std::uint32_t message,
                                         std::uint32_t wparam,
                                         std::uint32_t lparam);

    WindowMessageProcessor& processor_;
    IWindowMessageRuntime& runtime_;
    static thread_local Win32WindowMessageBinding* current_;
};

}  // namespace pal4
