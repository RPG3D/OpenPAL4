#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ScreenEffectDispatcherState {
    std::uint8_t enabled{};
    std::uint8_t reserved[3]{};
    std::int32_t active{};
    std::int32_t current_key{};
    std::uint32_t current_handler{};
    std::uint32_t frame_state{};
    std::uint32_t tree_field{};
    std::uint32_t tree_sentinel{};
};
static_assert(sizeof(ScreenEffectDispatcherState) == 28u);
static_assert(offsetof(ScreenEffectDispatcherState, frame_state) == 16u);
static_assert(offsetof(ScreenEffectDispatcherState, tree_field) == 20u);
static_assert(offsetof(ScreenEffectDispatcherState, tree_sentinel) == 24u);

class IScreenEffectDispatchRuntime {
public:
    virtual ~IScreenEffectDispatchRuntime() = default;
    virtual void PrepareSearch(ScreenEffectDispatcherState& state) = 0;
    [[nodiscard]] virtual std::uint32_t SearchNode(
        const ScreenEffectDispatcherState& state,
        std::int32_t key) = 0;
    [[nodiscard]] virtual std::int32_t NodeKey(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t NodeHandler(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual bool TestHandler(
        std::uint32_t handler, std::uint32_t slot_offset) = 0;
    virtual void ActivateHandler(std::uint32_t handler) = 0;
};

class ScreenEffectDispatchRuntimeStub final
    : public IScreenEffectDispatchRuntime {
public:
    void PrepareSearch(ScreenEffectDispatcherState& state) override;
    [[nodiscard]] std::uint32_t SearchNode(
        const ScreenEffectDispatcherState& state,
        std::int32_t key) override;
    [[nodiscard]] std::int32_t NodeKey(std::uint32_t node) override;
    [[nodiscard]] std::uint32_t NodeHandler(
        std::uint32_t node) override;
    [[nodiscard]] bool TestHandler(
        std::uint32_t handler, std::uint32_t slot_offset) override;
    void ActivateHandler(std::uint32_t handler) override;
};

class IScreenEffectFrameRuntime {
public:
    virtual ~IScreenEffectFrameRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    virtual void FrameSetIdentity(std::uint32_t frame) = 0;
    virtual void ApplyTransform(std::uint32_t frame,
                                std::uint32_t transform,
                                std::uint32_t flags) = 0;
    virtual void FrameUpdateObjects(std::uint32_t frame) = 0;
    virtual void CameraClear(std::uint32_t camera,
                             std::uint32_t color_address,
                             std::uint32_t flags) = 0;
    virtual void CameraBeginUpdate(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t CameraEndUpdate(
        std::uint32_t camera) = 0;
};

class ScreenEffectFrameRuntimeStub final
    : public IScreenEffectFrameRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t GetObjectTransform(
        std::uint32_t object) override;
    void FrameSetIdentity(std::uint32_t frame) override;
    void ApplyTransform(std::uint32_t frame,
                        std::uint32_t transform,
                        std::uint32_t flags) override;
    void FrameUpdateObjects(std::uint32_t frame) override;
    void CameraClear(std::uint32_t camera,
                     std::uint32_t color_address,
                     std::uint32_t flags) override;
    void CameraBeginUpdate(std::uint32_t camera) override;
    [[nodiscard]] std::uint32_t CameraEndUpdate(
        std::uint32_t camera) override;
};

class IScreenEffectSceneRuntime {
public:
    virtual ~IScreenEffectSceneRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Render(
        std::uint32_t handler, std::uint32_t dispatcher,
        std::uint32_t frame_state) = 0;
};

class ScreenEffectSceneRuntimeStub final
    : public IScreenEffectSceneRuntime {
public:
    [[nodiscard]] std::uint32_t Render(
        std::uint32_t handler, std::uint32_t dispatcher,
        std::uint32_t frame_state) override;
};

class ScreenEffectDispatcher final {
public:
    static void HandleScript(
        ScreenEffectDispatcherState& state, std::int32_t key,
        IScreenEffectDispatchRuntime& runtime);
    static void PrepareRenderFrame(
        const ScreenEffectDispatcherState& state,
        std::uint32_t input_record,
        IScreenEffectFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t EndRenderFrame(
        const ScreenEffectDispatcherState& state,
        std::uint32_t incoming_eax,
        IScreenEffectFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t RenderScene(
        std::uint32_t object_address,
        const ScreenEffectDispatcherState& state,
        IScreenEffectSceneRuntime& runtime);
};

}  // namespace pal4
