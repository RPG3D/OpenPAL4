#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class MenuButtonVisualState {
    Normal,
    Highlight,
    Disabled,
    Pushed,
};

enum class MenuButtonMetric {
    X,
    Y,
    Width,
    Height,
};

struct MenuButtonColour {
    float red;
    float green;
    float blue;
    float alpha;
};

class IMenuWindowButtonRuntime {
public:
    virtual ~IMenuWindowButtonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void AppendButtonRecord(
        std::uint32_t records, std::uint32_t insertion,
        std::uint32_t count, std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t CreateWindow(
        std::string_view type, std::string_view name) = 0;
    virtual void SetText(std::uint32_t window,
                         std::uint32_t string_address) = 0;
    virtual void SetTextColour(
        std::uint32_t window, MenuButtonVisualState state,
        MenuButtonColour colour) = 0;
    virtual void CopyImage(std::uint32_t model,
                           std::uint32_t button,
                           MenuButtonVisualState state,
                           std::string_view image_scope) = 0;
    virtual void AddChild(std::uint32_t parent,
                          std::uint32_t child) = 0;
    [[nodiscard]] virtual float GetMetric(
        std::uint32_t window, MenuButtonMetric metric,
        std::uint32_t mode) = 0;
    virtual void SetMetric(std::uint32_t window,
                           MenuButtonMetric metric,
                           std::uint32_t mode, float value) = 0;
    virtual void SetClippedByParent(std::uint32_t window,
                                    bool clipped) = 0;
    virtual void SetEnabled(std::uint32_t window,
                            bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructClickCallback(
        std::uint32_t storage, std::uint32_t function,
        std::uint32_t argument, std::uint32_t owner) = 0;
    virtual void SubscribeClicked(std::uint32_t button,
                                  std::uint32_t callback) = 0;
    virtual void ReleaseClickCallback(std::uint32_t callback) = 0;
    virtual void AppendNavigationButton(
        std::uint32_t list, std::uint32_t insertion,
        std::uint32_t button) = 0;
};

class MenuWindowButton final {
public:
    // IDA 0x4b3ef0. The native return is an incidental StringCleanup value.
    static void Create(std::uint32_t menu, std::uint32_t record,
                       bool add_to_navigation, bool enabled,
                       IMenuWindowButtonRuntime& runtime);
};

}  // namespace pal4
