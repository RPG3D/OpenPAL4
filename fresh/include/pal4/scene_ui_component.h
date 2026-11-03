#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct SceneUiComponentState {
    std::array<std::uint32_t, 16> words{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        return words[offset / 4u];
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        words[offset / 4u] = value;
    }
};

class ISceneUiComponentRuntime {
public:
    virtual ~ISceneUiComponentRuntime() = default;
    virtual void LoadLayout(std::uint32_t address,
                            std::string_view layout,
                            std::uint32_t parent,
                            std::uint32_t flags) = 0;
    virtual void DestroyBase(std::uint32_t address) = 0;
    virtual void InitializeNameBuffer() = 0;
    virtual void AssignPrefix(std::uint32_t source_address,
                              std::uint32_t offset,
                              std::uint32_t count) = 0;
    virtual void AppendSuffix(std::string_view suffix,
                              bool inline_append) = 0;
    [[nodiscard]] virtual std::string_view CurrentName() = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t component, std::string_view name) = 0;
    virtual void SetHorizontalFormatting(std::uint32_t window,
                                         std::uint32_t format) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupNameBuffer() = 0;
    virtual void SetParent(std::uint32_t address, std::uint8_t enabled,
                           std::uint32_t parent) = 0;
};

class SceneUiComponent final {
public:
    static constexpr std::uint32_t kVtable = 0x843ad0u;
    static constexpr std::uint32_t kNameBufferOffset = 0x14u;

    [[nodiscard]] static std::uint32_t Construct(
        SceneUiComponentState& state, std::uint32_t address,
        std::string_view layout, ISceneUiComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeControls(
        SceneUiComponentState& state, std::uint32_t address,
        ISceneUiComponentRuntime& runtime);
    static void SetParent(std::uint32_t address, std::uint8_t enabled,
                          std::uint32_t parent,
                          ISceneUiComponentRuntime& runtime);
};

}  // namespace pal4
