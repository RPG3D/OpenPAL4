#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

// The 44-byte object fade handler used by 0x5e72e0..0x5e7620.
struct ObjectBlendWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
    std::uint32_t object{};
    float duration{};
    float elapsed{};
};
static_assert(sizeof(ObjectBlendWaitState) == 0x2cu);
static_assert(offsetof(ObjectBlendWaitState, object) == 0x20u);
static_assert(offsetof(ObjectBlendWaitState, duration) == 0x24u);
static_assert(offsetof(ObjectBlendWaitState, elapsed) == 0x28u);

// Native object, RenderWare, arena and script operations for fades.
class IScriptObjectBlendRuntime {
public:
    virtual ~IScriptObjectBlendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual ObjectBlendWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(ObjectBlendWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(ObjectBlendWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, ObjectBlendWaitState* state) = 0;
    virtual void SetMaterialFlags(std::uint32_t object,
                                  std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool HasAnimatedObject(std::uint32_t object) = 0;
    virtual void RemoveAnimatedObject(std::uint32_t object) = 0;
    virtual void SetVisible(std::uint32_t object, bool visible) = 0;
    virtual void AddAnimatedObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint8_t InterpolateAlpha(
        float start, float end, float ratio) = 0;
    virtual void CloseWait(ObjectBlendWaitState& state) = 0;
};

// Reconstructs the object-specific fade handler lifecycle.
class ScriptObjectBlend final {
public:
    static constexpr std::uint32_t kOutVtable = 0x849618u;
    static constexpr std::uint32_t kInVtable = 0x849624u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptObjectBlend(IScriptObjectBlendRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e72e0: create fade-out handler and restore full material flags.
    [[nodiscard]] std::uint32_t BlendOut(std::string_view object_name,
                                         float duration,
                                         std::uint8_t wait_mode);
    // 0x5e73d0: advance fade-out and remove object at completion.
    void UpdateOut(ObjectBlendWaitState& state, float delta);
    // 0x5e74a0: finalize fade-out immediately.
    void FinishOut(ObjectBlendWaitState& state);
    // 0x5e7500: create fade-in handler and re-add visible object.
    [[nodiscard]] std::uint32_t BlendIn(std::string_view object_name,
                                        float duration,
                                        std::uint8_t wait_mode);
    // 0x5e7620: advance fade-in and restore full material flags.
    void UpdateIn(ObjectBlendWaitState& state, float delta);

private:
    [[nodiscard]] std::uint32_t Create(std::string_view object_name,
                                       float duration,
                                       std::uint8_t wait_mode, bool fade_in);
    void SetAlpha(std::uint32_t object, float start, float end,
                  float ratio);

    IScriptObjectBlendRuntime& runtime_;
};

}  // namespace pal4
