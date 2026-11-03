#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlayerBlendWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
    std::uint32_t actor{};
    float duration{};
    float elapsed{};
};
static_assert(sizeof(PlayerBlendWaitState) == 0x2cu);
static_assert(offsetof(PlayerBlendWaitState, actor) == 0x20u);

class IScriptPlayerBlendRuntime {
public:
    virtual ~IScriptPlayerBlendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t NpcByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::string_view ActorName(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t NpcInfoByName(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void ProcessNpcEvents(std::uint32_t actor,
                                  std::uint32_t info,
                                  std::uint32_t mode) = 0;
    [[nodiscard]] virtual PlayerBlendWaitState* Allocate(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void Construct(PlayerBlendWaitState& state,
                           std::uint8_t wait_mode,
                           std::uint32_t queue) = 0;
    virtual void Flush(PlayerBlendWaitState& state) = 0;
    virtual void SetMaterialFlags(std::uint32_t actor,
                                  std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void RemoveFromArena(std::uint32_t arena,
                                 std::uint32_t actor) = 0;
    virtual void AddToArena(std::uint32_t arena,
                            std::uint32_t actor) = 0;
    virtual void SetStateFlag(std::uint32_t actor,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual float Interpolate(float from, float to,
                                            float fraction,
                                            std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t ConvertAlpha(float value) = 0;
    virtual void Close(PlayerBlendWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, PlayerBlendWaitState* state) = 0;
};

class ScriptPlayerBlend final {
public:
    static constexpr std::uint32_t kOutVtable = 0x849588u;
    static constexpr std::uint32_t kInVtable = 0x849594u;
    static constexpr std::uint32_t kNpcOutVtable = 0x8495a0u;
    static constexpr std::uint32_t kNpcInVtable = 0x8495acu;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptPlayerBlend(IScriptPlayerBlendRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5e3010..0x5e3300 and 0x5e4330..0x5e4520.
    [[nodiscard]] std::uint32_t BlendOut(std::int32_t index,
                                         float duration,
                                         std::uint8_t wait_mode);
    void UpdateOut(PlayerBlendWaitState& state, double frame_time);
    // IDA 0x5e7620: fade to opaque without removing the actor.
    void UpdateIn(PlayerBlendWaitState& state, double frame_time);
    void FinishOut(PlayerBlendWaitState& state);
    [[nodiscard]] std::uint32_t BlendIn(std::int32_t index,
                                        float duration,
                                        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t BlendNpcOut(std::string_view name,
                                            float duration,
                                            std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t BlendNpcIn(std::string_view name,
                                           float duration,
                                           std::uint8_t wait_mode);
    void Cleanup(PlayerBlendWaitState& state);

private:
    [[nodiscard]] std::uint32_t Start(std::int32_t index,
                                      float duration,
                                      std::uint8_t wait_mode, bool in);
    [[nodiscard]] std::uint32_t StartActor(std::uint32_t actor,
                                           float duration,
                                           std::uint8_t wait_mode,
                                           bool in, bool npc);
    IScriptPlayerBlendRuntime& runtime_;
};

}  // namespace pal4
