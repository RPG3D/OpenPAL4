#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct MainWindowEventWords {
    std::uint32_t first{};
    std::uint32_t second{};
};

class IMainWindowCommandRuntime {
public:
    virtual ~IMainWindowCommandRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void SetFrameVisibility(std::uint32_t frame,
                                    std::uint32_t showing,
                                    std::uint32_t parent) = 0;
    virtual void ProcessEvent(std::uint32_t tree,
                              std::string_view command,
                              MainWindowEventWords& words) = 0;
    virtual void InitializeGame() = 0;
    virtual void HideCurrentWindow(std::uint32_t showing,
                                   std::uint32_t parent) = 0;
};

class MainWindowCommandRuntimeStub : public IMainWindowCommandRuntime {
public:
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t FindFrame(
        std::uint32_t, std::string_view) override { return 0u; }
    void SetFrameVisibility(std::uint32_t, std::uint32_t,
                            std::uint32_t) override {}
    void ProcessEvent(std::uint32_t, std::string_view,
                      MainWindowEventWords&) override {}
    void InitializeGame() override {}
    void HideCurrentWindow(std::uint32_t, std::uint32_t) override {}
};

class MainWindowCommands final {
public:
    [[nodiscard]] static std::uint8_t DispatchPrimaryButton(
        std::uint32_t button, IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t ShowMain(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t CloseMain(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t StartNewGame(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t ShowMoviePreview(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t ShowPicturePreview(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t ShowAboutGame(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t ShowGameInfo(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t ExitButton(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t ProcessExit(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t ShowLoadGame(
        IMainWindowCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t ShowQuest(
        IMainWindowCommandRuntime& runtime);
};

}  // namespace pal4
