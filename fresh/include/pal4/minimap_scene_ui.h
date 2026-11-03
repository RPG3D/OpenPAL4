#pragma once

#include "pal4/scene_ui_component.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IMinimapSceneUiButtonRuntime;

struct MinimapSceneUiState {
    std::array<std::uint8_t, 0xf8> bytes{};
    SceneUiComponentState component;

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t offset) const noexcept {
        return static_cast<std::uint32_t>(bytes[offset]) |
               (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
               (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
               (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
    }
    void WriteWord(std::uint32_t offset, std::uint32_t value) noexcept {
        for (std::uint32_t index = 0; index < 4u; ++index) {
            bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8u));
        }
    }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t offset) const noexcept {
        return bytes[offset];
    }
    void WriteByte(std::uint32_t offset, std::uint8_t value) noexcept {
        bytes[offset] = value;
    }
    void SyncComponent() noexcept {
        for (std::uint32_t offset = 0u; offset < 0x40u; offset += 4u) {
            WriteWord(0x2cu + offset, component.Read(offset));
        }
    }
};

enum class MinimapUiEvent { Clicked, MouseDown, MouseUp, MouseMove };

class IMinimapSceneUiRuntime {
public:
    virtual ~IMinimapSceneUiRuntime() = default;
    virtual void LoadLayout(MinimapSceneUiState& state,
                            std::uint32_t address,
                            std::uint32_t name,
                            std::uint32_t parent,
                            std::uint32_t flags) = 0;
    virtual void ProcessUtf8(std::uint32_t address) = 0;
    virtual void ResetText(std::uint32_t address, std::uint8_t error) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t address,
                                             std::string_view text,
                                             std::uint8_t mode) = 0;
    virtual void SetTextSize(std::uint32_t address,
                             std::uint32_t size) = 0;
    virtual void SetClippedByParent(std::uint32_t window, bool clipped) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteCallbackWord(std::uint32_t address,
                                    std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructCallback(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t argument, std::uint32_t owner,
        bool command_callback) = 0;
    virtual void Subscribe(std::uint32_t window, MinimapUiEvent event,
                           std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertCommand(std::uint32_t tree,
                               std::uint32_t key_pointer_cell,
                               std::uint32_t callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback) = 0;
    virtual void UnwindConstruction(std::uint32_t address,
                                     std::uint32_t stage) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyUtf8(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(std::uint32_t address) = 0;
};

class MinimapSceneUi final {
public:
    static constexpr std::uint32_t kVtable = 0x843b08u;
    static constexpr std::uint32_t kBaseComponentVtable = 0x8435a4u;
    static constexpr std::uint32_t kComponentVtable = 0x843af0u;
    static constexpr std::uint32_t kInlineCallbackVtable = 0x843ae8u;
    static constexpr std::uint32_t kUpdateSceneKeyCell = 0x8a80c4u;
    static constexpr std::uint32_t kJumpHintKeyCell = 0x8a80a8u;

    [[nodiscard]] static std::uint32_t Construct(
        MinimapSceneUiState& state, std::uint32_t address,
        std::uint32_t layout_name,
        ISceneUiComponentRuntime& component_runtime,
        IMinimapSceneUiButtonRuntime& button_runtime,
        IMinimapSceneUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyUtf8Thunk(
        std::uint32_t address, IMinimapSceneUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        MinimapSceneUiState& state, std::uint32_t address,
        IMinimapSceneUiRuntime& runtime);
};

}  // namespace pal4
