#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct GameInitWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
    std::uint32_t state_identity{};
};
static_assert(sizeof(GameInitWaitState) == 0x24u);
static_assert(offsetof(GameInitWaitState, state_identity) == 0x20u);

class IScriptGameInitRuntime {
public:
    virtual ~IScriptGameInitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    virtual void InitializeStateAndUi(std::uint32_t game,
                                      std::uint32_t first,
                                      std::uint32_t second) = 0;
    [[nodiscard]] virtual GameInitWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(GameInitWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(GameInitWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, GameInitWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentState(
        std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t StateIdentity(
        std::uint32_t state) = 0;
    virtual void CloseWait(GameInitWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyWait(
        GameInitWaitState& state) = 0;
};

class ScriptGameInitWait final {
public:
    static constexpr std::uint32_t kVtable = 0x84969cu;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptGameInitWait(IScriptGameInitRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e8c10: initialize state/UI, then queue a blocking wait.
    [[nodiscard]] std::uint32_t Start();
    // 0x5e8cb0: close when the current state is absent or matches the marker.
    void Update(GameInitWaitState& state);
    // 0x5e8ce0 and 0x5e8cf0: original vtable forwarding thunks.
    void Close(GameInitWaitState& state);
    [[nodiscard]] std::uint32_t Destroy(GameInitWaitState& state);

private:
    IScriptGameInitRuntime& runtime_;
};

}  // namespace pal4
