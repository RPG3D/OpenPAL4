#pragma once

#include "pal4/game_render_system.h"
#include "pal4/window_message_processor.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct ApplicationWindowIdentity {
    std::string_view class_name = "rwc";
    std::string_view title = "PAL4-Application";
};

struct ApplicationMessage {
    std::uint32_t message = 0u;
    std::uint32_t wparam = 0u;
    std::uint32_t lparam = 0u;
    std::uint32_t window = 0u;
    std::uint32_t time = 0u;
    std::int32_t x = 0;
    std::int32_t y = 0;
};

class IApplicationGameServices {
public:
    virtual ~IApplicationGameServices() = default;
    virtual void SetExceptionLogPath(std::string_view path) = 0;
    virtual void SetExceptionHandlerOptions(std::uint32_t first,
                                         std::uint32_t second,
                                         std::uint32_t third,
                                         std::uint32_t fourth,
                                         std::uint32_t fifth) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeLogger() = 0;
    virtual void SetLogFilePath(std::uint32_t logger,
                                std::string_view path) = 0;
    [[nodiscard]] virtual std::string FormatStartupMessage() = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void BindAllocatedGlobals(std::uint32_t config_address,
                                      std::uint32_t game_address) = 0;
    [[nodiscard]] virtual bool InitializeGameConfig(
        std::uint32_t instance) = 0;
    [[nodiscard]] virtual std::uint32_t ScanDecimal(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t GetRenderSystem() = 0;
    virtual void InitializeGameSystem() = 0;
    virtual void LogStartupMessage(std::uint32_t logger,
                                   std::string_view message) = 0;
    virtual void SetViewportCenter(float x, float y) = 0;
    [[nodiscard]] virtual std::uint32_t GetFrameCallback() = 0;
    virtual void UpdateGameFrame(std::uint32_t frame_callback) = 0;
    virtual void CleanupGame() = 0;
    virtual void ReleaseAllocatorRegions() = 0;
};

class IApplicationRuntime : public IApplicationGameServices {
public:
    ~IApplicationRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view class_name, std::string_view title) = 0;
    [[nodiscard]] virtual bool IsWindow(std::uint32_t window) = 0;
    virtual void ShowWindow(std::uint32_t window, std::int32_t command) = 0;
    [[nodiscard]] virtual std::uint32_t Time() = 0;
    virtual void SeedRandom(std::uint32_t seed) = 0;
    [[nodiscard]] virtual std::uint32_t Random() = 0;
    virtual void CreateCounterThread() = 0;
    // Keep the referenced parameter alive until the worker's final access.
    virtual void CreateRandomizedThread(std::uint32_t& parameter) = 0;
    virtual void StopStartupThreads() = 0;
    virtual void Sleep(std::uint32_t milliseconds) = 0;
    [[nodiscard]] virtual std::uint32_t LoadIcon(
        std::uint32_t instance, std::uint32_t resource) = 0;
    [[nodiscard]] virtual bool CreateGameWindow(
        std::uint32_t render_system, std::string_view class_name,
        std::string_view title, bool windowed, std::uint32_t icon) = 0;
    virtual void UpdateWindow(std::uint32_t window) = 0;
    [[nodiscard]] virtual bool PeekMessage(ApplicationMessage& message,
                                           std::uint32_t flags) = 0;
    virtual void TranslateMessage(const ApplicationMessage& message) = 0;
    virtual void DispatchMessage(const ApplicationMessage& message) = 0;
    virtual void WaitMessage() = 0;
    virtual void DestroyGameWindow(std::uint32_t render_system) = 0;
};

class ApplicationEntry final {
public:
    ApplicationEntry(GameRenderState& game, WindowMessageConfig& config,
                     std::uint8_t& threads_initialized,
                     std::uint32_t& thread_count)
        : game_(game), config_(config), threads_initialized_(threads_initialized),
          thread_count_(thread_count) {}

    [[nodiscard]] std::uint32_t Run(std::uint32_t instance,
                                    std::string_view command_line,
                                    std::int32_t show_command,
                                    IApplicationRuntime& runtime,
                                    ApplicationWindowIdentity identity = {});
    [[nodiscard]] std::uint32_t ConfigAddress() const { return config_address_; }
    [[nodiscard]] std::uint32_t GameAddress() const { return game_address_; }

private:
    void ApplyCommandLine(std::string_view command_line,
                          IApplicationRuntime& runtime);

    GameRenderState& game_;
    WindowMessageConfig& config_;
    std::uint8_t& threads_initialized_;
    std::uint32_t& thread_count_;
    std::uint32_t config_address_ = 0u;
    std::uint32_t game_address_ = 0u;
};

}  // namespace pal4
