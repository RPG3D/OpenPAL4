#pragma once

#include "pal4/minimap_scene_ui.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMinimapSceneUiButtonRuntime {
public:
    virtual ~IMinimapSceneUiButtonRuntime() = default;
    virtual void InitializeNameBuffer() = 0;
    virtual void AssignPrefix(std::uint32_t source_address,
                              std::uint32_t offset,
                              std::uint32_t count) = 0;
    virtual void AppendSuffix(std::string_view suffix,
                              bool inline_append) = 0;
    [[nodiscard]] virtual std::string_view CurrentName() = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t owner, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructCallback(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t argument, std::uint32_t owner) = 0;
    virtual void Subscribe(std::uint32_t window, MinimapUiEvent event,
                           std::uint32_t callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupNameBuffer() = 0;
};

class MinimapSceneUiButtons final {
public:
    static constexpr std::uint32_t kNameBufferOffset = 0x14u;

    [[nodiscard]] static std::uint32_t Initialize(
        MinimapSceneUiState& state, std::uint32_t address,
        IMinimapSceneUiButtonRuntime& runtime);
};

}  // namespace pal4
