#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISystemSettingDisplayRuntime {
public:
    virtual ~ISystemSettingDisplayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::string_view imageset,
                          std::string_view image) = 0;
    [[nodiscard]] virtual std::uint32_t BeginName(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ResetName(
        std::uint32_t buffer, std::uint32_t frame,
        std::string_view suffix) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t EndName(std::uint32_t buffer) = 0;
};

// Options-page display requires activity routing, but no save-list services.
class ISystemSettingOptionsRuntime : public ISystemSettingDisplayRuntime {
public:
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void RemoveActive(std::uint32_t collection,
                              std::uint32_t member) = 0;
    virtual void InsertActive(std::uint32_t collection,
                              std::uint32_t member) = 0;
};

class ISystemSettingFrameModeRuntime : public ISystemSettingOptionsRuntime {
public:
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    virtual void PopulateSaveList(std::uint32_t frame,
                                  std::int32_t page,
                                  std::uint32_t mode) = 0;
};

class SystemSettingFrameMode final {
public:
    [[nodiscard]] static std::uint32_t ShowSave(
        std::uint32_t frame, ISystemSettingOptionsRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowDataMode(
        std::uint32_t frame, ISystemSettingFrameModeRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowSystem(
        std::uint32_t frame, ISystemSettingDisplayRuntime& runtime);
};

}  // namespace pal4
