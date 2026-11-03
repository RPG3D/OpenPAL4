#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct GameObjectPropertyState {
    std::array<std::uint32_t, 169> words{};
};
static_assert(sizeof(GameObjectPropertyState) == 0x2a4u);

class IGameObjectPropertyRuntime {
public:
    virtual ~IGameObjectPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CallGetter(
        std::uint32_t vtable, std::uint32_t slot_offset,
        std::uint32_t object, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CallSetter(
        std::uint32_t vtable, std::uint32_t slot_offset,
        std::uint32_t object, std::uint32_t index,
        std::uint32_t value) = 0;
    virtual void NotifyHealthStatus(std::uint32_t object) = 0;
};

class GameObjectPropertyRuntimeStub :
    public IGameObjectPropertyRuntime {
public:
    std::uint32_t CallGetter(std::uint32_t, std::uint32_t,
                             std::uint32_t, std::uint32_t) override;
    std::uint32_t CallSetter(std::uint32_t, std::uint32_t,
                             std::uint32_t, std::uint32_t,
                             std::uint32_t) override;
    void NotifyHealthStatus(std::uint32_t) override;
};

class GameObjectPropertyAccess final {
public:
    // IDA 0x485210: inherited object field at +0x1a0.
    [[nodiscard]] static std::uint32_t GetHeightLimitFlag(
        const GameObjectPropertyState& state);
    // IDA 0x440480/0x440490.
    [[nodiscard]] static std::uint32_t Get85(
        const GameObjectPropertyState& state);
    [[nodiscard]] static std::uint32_t Set85(
        GameObjectPropertyState& state, std::uint32_t value);
    // IDA 0x4404a0/0x4404b0.
    [[nodiscard]] static std::uint32_t Get168(
        const GameObjectPropertyState& state);
    [[nodiscard]] static std::uint32_t Set168(
        GameObjectPropertyState& state, std::uint32_t value);
    // IDA 0x4404c0/0x4404d0.
    [[nodiscard]] static std::uint32_t Get167(
        const GameObjectPropertyState& state);
    [[nodiscard]] static std::uint32_t Set167And83(
        GameObjectPropertyState& state, std::uint32_t value);
    // IDA 0x4404f0/0x440510.
    [[nodiscard]] static std::uint32_t GetIndexed116(
        const GameObjectPropertyState& state, std::uint32_t index);
    [[nodiscard]] static std::uint32_t SetIndexed116(
        GameObjectPropertyState& state, std::uint32_t index,
        std::uint32_t value);
    // IDA 0x440530.
    [[nodiscard]] static std::uint32_t AdjustIndexed(
        const GameObjectPropertyState& state, std::uint32_t object,
        std::uint32_t index, std::uint32_t delta,
        IGameObjectPropertyRuntime& runtime);
    // IDA 0x440560/0x440580.
    [[nodiscard]] static std::uint32_t GetIndexed136(
        const GameObjectPropertyState& state, std::uint32_t index);
    [[nodiscard]] static std::uint32_t SetIndexed136(
        GameObjectPropertyState& state, std::uint32_t object,
        std::uint32_t index, std::int32_t value,
        IGameObjectPropertyRuntime& runtime);
    // IDA 0x4406b0.
    [[nodiscard]] static std::uint32_t AdjustProperty(
        const GameObjectPropertyState& state, std::uint32_t object,
        std::uint32_t index, std::uint32_t delta,
        IGameObjectPropertyRuntime& runtime);
    // IDA 0x4406e0/0x440700/0x440720.
    [[nodiscard]] static std::uint32_t GetIndexed86(
        const GameObjectPropertyState& state, std::uint32_t index);
    [[nodiscard]] static std::uint32_t SetIndexed86(
        GameObjectPropertyState& state, std::uint32_t index,
        std::uint32_t value);
    [[nodiscard]] static std::uint32_t AdjustRecordValue(
        const GameObjectPropertyState& state, std::uint32_t object,
        std::uint32_t index, std::uint32_t delta,
        IGameObjectPropertyRuntime& runtime);
};

}  // namespace pal4
