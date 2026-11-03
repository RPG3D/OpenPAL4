#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IFrameToolbarRuntime {
public:
    virtual ~IFrameToolbarRuntime() = default;
    virtual void LoadLayout(std::uint32_t object,
                            std::string_view layout_name) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view WindowName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::uint32_t object, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeClicked(
        std::uint32_t button, std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseNameBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t object) = 0;
    virtual void SetParent(std::uint32_t object, bool visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t SetNestedVisible(
        std::uint32_t object, bool visible,
        std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessUiEvent(std::uint32_t tree,
                                std::uint32_t key,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
};

class FrameToolbar final {
public:
    static constexpr std::uint32_t kVtable = 0x843f28u;
    static constexpr std::uint32_t kCloseCallbackVtable = 0x843f40u;
    static constexpr std::uint32_t kCloseEventKey = 0x8a80b4u;

    // IDA 0x4a6130.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::string_view layout_name,
        IFrameToolbarRuntime& runtime);
    // IDA 0x4a61c0.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IFrameToolbarRuntime& runtime);
    // IDA 0x4a6210.
    [[nodiscard]] static std::uint32_t InitializeControls(
        std::uint32_t object, IFrameToolbarRuntime& runtime);
    // IDA 0x4a6390.
    [[nodiscard]] static std::uint32_t SetVisible(
        std::uint32_t object, bool visible, std::uint32_t parent,
        IFrameToolbarRuntime& runtime);
    // IDA 0x4a63c0.
    [[nodiscard]] static std::uint8_t OnCloseButtonClick(
        IFrameToolbarRuntime& runtime);
};

}  // namespace pal4
