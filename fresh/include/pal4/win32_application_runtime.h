#pragma once

#include "pal4/application_entry.h"
#include "pal4/game_window_lifecycle.h"
#include "pal4/win32_startup_threads.h"

namespace pal4 {

// Connects WinMain's OS calls, worker threads and game window to the reviewed
// ApplicationEntry. Game subsystems remain supplied by IApplicationGameServices.
class Win32ApplicationRuntime final : public IApplicationRuntime {
public:
    Win32ApplicationRuntime(WindowMessageConfig& config,
                            GameConfigWords& window_config,
                            GameWindowLifecycle& window,
                            IGameWindowRuntime& window_runtime,
                            Win32StartupThreads& threads,
                            IApplicationGameServices& game_services,
                            std::uint32_t window_procedure,
                            std::uint32_t counter_stack_word,
                            std::uint8_t randomized_stack_byte)
        : config_(config), window_config_(window_config), window_(window),
          window_runtime_(window_runtime), threads_(threads),
          game_services_(game_services), window_procedure_(window_procedure),
          counter_stack_word_(counter_stack_word),
          randomized_stack_byte_(randomized_stack_byte) {}

    void SetExceptionLogPath(std::string_view path) override;
    void SetExceptionHandlerOptions(std::uint32_t first,
                                    std::uint32_t second,
                                    std::uint32_t third,
                                    std::uint32_t fourth,
                                    std::uint32_t fifth) override;
    [[nodiscard]] std::uint32_t InitializeLogger() override;
    void SetLogFilePath(std::uint32_t logger,
                        std::string_view path) override;
    [[nodiscard]] std::uint32_t FindWindow(
        std::string_view class_name, std::string_view title) override;
    [[nodiscard]] bool IsWindow(std::uint32_t window) override;
    void ShowWindow(std::uint32_t window, std::int32_t command) override;
    [[nodiscard]] std::string FormatStartupMessage() override;
    [[nodiscard]] std::uint32_t Time() override;
    void SeedRandom(std::uint32_t seed) override;
    [[nodiscard]] std::uint32_t Random() override;
    void CreateCounterThread() override;
    void CreateRandomizedThread(std::uint32_t& parameter) override;
    void StopStartupThreads() override;
    void Sleep(std::uint32_t milliseconds) override;
    [[nodiscard]] std::uint32_t AllocateMemory(
        std::uint32_t bytes) override;
    void BindAllocatedGlobals(std::uint32_t config_address,
                              std::uint32_t game_address) override;
    [[nodiscard]] bool InitializeGameConfig(
        std::uint32_t instance) override;
    [[nodiscard]] std::uint32_t ScanDecimal(
        std::string_view text) override;
    [[nodiscard]] std::uint32_t GetRenderSystem() override;
    [[nodiscard]] std::uint32_t LoadIcon(
        std::uint32_t instance, std::uint32_t resource) override;
    [[nodiscard]] bool CreateGameWindow(
        std::uint32_t render_system, std::string_view class_name,
        std::string_view title, bool windowed,
        std::uint32_t icon) override;
    void InitializeGameSystem() override;
    void LogStartupMessage(std::uint32_t logger,
                           std::string_view message) override;
    void UpdateWindow(std::uint32_t window) override;
    void SetViewportCenter(float x, float y) override;
    [[nodiscard]] bool PeekMessage(ApplicationMessage& message,
                                   std::uint32_t flags) override;
    void TranslateMessage(const ApplicationMessage& message) override;
    void DispatchMessage(const ApplicationMessage& message) override;
    [[nodiscard]] std::uint32_t GetFrameCallback() override;
    void UpdateGameFrame(std::uint32_t frame_callback) override;
    void WaitMessage() override;
    void CleanupGame() override;
    void DestroyGameWindow(std::uint32_t render_system) override;
    void ReleaseAllocatorRegions() override;

private:
    WindowMessageConfig& config_;
    GameConfigWords& window_config_;
    GameWindowLifecycle& window_;
    IGameWindowRuntime& window_runtime_;
    Win32StartupThreads& threads_;
    IApplicationGameServices& game_services_;
    std::uint32_t window_procedure_;
    std::uint32_t counter_stack_word_;
    std::uint8_t randomized_stack_byte_;
    std::uint32_t render_system_ = 0u;
};

}  // namespace pal4
