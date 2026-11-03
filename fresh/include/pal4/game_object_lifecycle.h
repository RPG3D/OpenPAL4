#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct GameObjectState {
    std::array<std::uint32_t, 0x6ecu / 4u> words{};
};
static_assert(sizeof(GameObjectState) == 0x6ecu);

class IGameObjectLifecycleRuntime {
public:
    virtual ~IGameObjectLifecycleRuntime() = default;
    virtual void CleanupEventsAndScripts(GameObjectState& state,
                                         std::uint32_t address,
                                         std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t Reset(GameObjectState& state,
                                              std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateState(GameObjectState& state,
                                                    std::uint32_t address,
                                                    std::uint32_t argument) = 0;
    virtual void HandleEventProcessing(std::uint32_t processor,
                                       float delta_time,
                                       std::uint32_t script,
                                       std::uint32_t tree,
                                       std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t UpdatePosition(
        GameObjectState& state, std::uint32_t address,
        float delta_time, std::uint32_t argument) = 0;
};

class GameObjectLifecycleRuntimeStub : public IGameObjectLifecycleRuntime {
public:
    void CleanupEventsAndScripts(GameObjectState&, std::uint32_t,
                                 std::uint32_t) override;
    std::uint32_t Reset(GameObjectState&, std::uint32_t) override;
    std::uint32_t UpdateState(GameObjectState&, std::uint32_t,
                              std::uint32_t) override;
    void HandleEventProcessing(std::uint32_t, float, std::uint32_t,
                               std::uint32_t, std::uint32_t) override;
    std::uint32_t UpdatePosition(GameObjectState&, std::uint32_t,
                                 float, std::uint32_t) override;
};

class GameObjectLifecycle final {
public:
    static constexpr std::uint32_t kActiveOffset = 0x1f0u;
    static constexpr std::uint32_t kStatusOffset = 0x654u;
    static constexpr std::uint32_t kProcessorOffset = 0x658u;
    static constexpr std::uint32_t kScriptOffset = 0x688u;
    static constexpr std::uint32_t kTreeOffset = 0x698u;
    static constexpr std::uint32_t kContextOffset = 0x6acu;

    // IDA 0x4393e0.
    [[nodiscard]] static std::uint32_t CleanupVirtual(
        GameObjectState& state, std::uint32_t address,
        IGameObjectLifecycleRuntime& runtime);
    // IDA 0x439400.
    [[nodiscard]] static std::uint32_t ConditionalProcess(
        GameObjectState& state, std::uint32_t address,
        std::uint32_t argument, IGameObjectLifecycleRuntime& runtime);
    // IDA 0x439420.
    [[nodiscard]] static std::uint32_t HandleEventAndProcess(
        GameObjectState& state, std::uint32_t address,
        float delta_time, std::uint32_t argument,
        IGameObjectLifecycleRuntime& runtime);
};

}  // namespace pal4
