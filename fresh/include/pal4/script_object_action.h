#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

// The 44-byte script handler installed by giObjectDoAction at 0x5e6b60.
struct ObjectActionWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 8> base{};
    std::uint32_t phase{};
    std::array<std::byte, 16> name_buffer{};
    std::int32_t completed_loops{};
    std::int32_t target_loops{};
    std::uint32_t object{};
};
static_assert(sizeof(ObjectActionWaitState) == 0x2cu);
static_assert(offsetof(ObjectActionWaitState, completed_loops) == 0x20u);
static_assert(offsetof(ObjectActionWaitState, target_loops) == 0x24u);
static_assert(offsetof(ObjectActionWaitState, object) == 0x28u);

// Native game-object, animation, string-buffer and script-queue operations.
class IScriptObjectActionRuntime {
public:
    virtual ~IScriptObjectActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectType(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::string_view AnimationName(
        std::uint32_t object) = 0;
    virtual void SelectAnimation(std::uint32_t object,
                                 std::string_view name) = 0;
    virtual void SetAnimationTime(std::uint32_t object, float time) = 0;
    virtual void ClearAnimationFlag(std::uint32_t object,
                                    std::uint32_t flag) = 0;
    virtual void SetAnimationFlag(std::uint32_t object,
                                  std::uint32_t flag) = 0;
    [[nodiscard]] virtual bool AnimationFlag(std::uint32_t object) = 0;
    [[nodiscard]] virtual float AnimationLength(std::uint32_t object) = 0;
    [[nodiscard]] virtual ObjectActionWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(ObjectActionWaitState& state,
                               std::uint8_t mode, std::uint32_t queue) = 0;
    virtual void AssignName(ObjectActionWaitState& state,
                            std::string_view name) = 0;
    virtual void FlushWait(ObjectActionWaitState& state) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            ObjectActionWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName() = 0;
    virtual void CloseWait(ObjectActionWaitState& state) = 0;
};

// Rebuilds the object animation script handler at 0x5e6b60..0x5e6e30.
class ScriptObjectAction final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x8495f4u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr std::uint32_t kObjectType = 6u;

    explicit ScriptObjectAction(IScriptObjectActionRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e6b60: selects animation and queues a named loop-wait handler.
    // A missing object was dereferenced in the binary; this boundary throws.
    [[nodiscard]] std::uint32_t Create(std::string_view object_name,
                                       std::string_view animation_name,
                                       std::int32_t target_loops,
                                       std::uint8_t wait_mode);
    // 0x5e6d90: counts animation flags or finishes the special -1 wait.
    void Update(ObjectActionWaitState& state);
    // 0x5e6e30: resets the counter and closes the handler.
    void Cleanup(ObjectActionWaitState& state);

private:
    IScriptObjectActionRuntime& runtime_;
};

}  // namespace pal4
