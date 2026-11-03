#pragma once

#include <cstdint>

namespace pal4 {

class ISystemSettingVisibilityRuntime {
public:
    virtual ~ISystemSettingVisibilityRuntime() = default;
    virtual void SetParent(std::uint32_t frame,
                           std::uint8_t visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessUiEvent(std::uint32_t tree,
                                std::uint32_t key,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void ShowSystemFrame(std::uint32_t frame) = 0;
    virtual void UpdateStats(std::uint32_t frame) = 0;
    virtual void InsertActive(std::uint32_t collection,
                              std::uint32_t member) = 0;
    virtual void RemoveActive(std::uint32_t collection,
                              std::uint32_t member) = 0;
    virtual void SetEnabled(std::uint32_t window,
                            bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateEventArgs() = 0;
    virtual void DestroyEventArgs(std::uint32_t event) = 0;
    virtual void InvokeLoadData(std::uint32_t frame,
                                std::uint32_t event) = 0;
    virtual void InvokeStartNewGame(std::uint32_t frame,
                                    std::uint32_t event) = 0;
    virtual void UpdateSystemDialog(std::uint32_t manager,
                                    std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeGlobalData() = 0;
    virtual void SaveSettings(std::uint32_t global_data) = 0;
};

class SystemSettingVisibility final {
public:
    static constexpr std::uint32_t kShowFrameTool = 0x8b1394u;
    static constexpr std::uint32_t kShowGameInfo = 0x8b13a0u;
    static constexpr std::uint32_t kHelpTips = 0x8b13a4u;

    static void SetVisible(std::uint32_t frame,
                           std::uint8_t visible,
                           std::uint32_t parent,
                           ISystemSettingVisibilityRuntime& runtime);
};

}  // namespace pal4
