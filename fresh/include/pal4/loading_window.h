#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct LoadingWindowState {
    std::uint32_t vtable{};
    std::string layout_prefix;
    std::uint32_t poser{};
    std::uint32_t mask{};
    float mask_x{};
    float mask_width{};
    float poser_x{};
};

struct LoadingWindowGeometry {
    float mask_width{};
    float mask_x{};
    float poser_x{};
};

class ILoadingWindowRuntime {
public:
    virtual ~ILoadingWindowRuntime() = default;
    virtual void LoadLayout(LoadingWindowState& state,
                            std::string_view layout,
                            std::uint32_t first,
                            std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::string_view name) = 0;
    [[nodiscard]] virtual float GetXPosition(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual float GetWidth(
        std::uint32_t window) = 0;
    virtual void SetWidth(std::uint32_t window, float width) = 0;
    virtual void SetXPosition(std::uint32_t window, float position) = 0;
    virtual void SetParent(LoadingWindowState& state, bool visible,
                           std::uint32_t parent) = 0;
};

class LoadingWindowRuntimeStub : public ILoadingWindowRuntime {
public:
    void LoadLayout(LoadingWindowState&, std::string_view,
                    std::uint32_t, std::uint32_t) override {}
    std::uint32_t GetWindow(std::string_view) override { return 0; }
    float GetXPosition(std::uint32_t) override { return 0.0f; }
    float GetWidth(std::uint32_t) override { return 0.0f; }
    void SetWidth(std::uint32_t, float) override {}
    void SetXPosition(std::uint32_t, float) override {}
    void SetParent(LoadingWindowState&, bool, std::uint32_t) override {}
};

class LoadingWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x8438f8u;

    [[nodiscard]] static LoadingWindowState& Construct(
        LoadingWindowState& state, ILoadingWindowRuntime& runtime);
    static void SetVisible(LoadingWindowState& state, bool visible,
                           std::uint32_t parent,
                           ILoadingWindowRuntime& runtime);
    static void UpdateProgress(LoadingWindowState& state, float progress,
                               ILoadingWindowRuntime& runtime);
    [[nodiscard]] static LoadingWindowGeometry ProgressGeometry(
        const LoadingWindowState& state, float progress);
};

}  // namespace pal4
