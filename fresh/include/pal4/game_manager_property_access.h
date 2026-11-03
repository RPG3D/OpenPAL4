#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct GameManagerPropertyState {
    std::array<std::uint32_t, 167> words{};
};
static_assert(sizeof(GameManagerPropertyState) == 0x29cu);

class IGameManagerPropertyRuntime {
public:
    virtual ~IGameManagerPropertyRuntime() = default;
    [[nodiscard]] virtual double GetFloat(std::uint32_t vtable,
        std::uint32_t slot, GameManagerPropertyState& state,
        std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t SetFloat(std::uint32_t vtable,
        std::uint32_t slot, GameManagerPropertyState& state,
        std::int32_t index, float value) = 0;
    [[nodiscard]] virtual std::uint32_t GetInteger(std::uint32_t vtable,
        std::uint32_t slot, GameManagerPropertyState& state,
        std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t SetInteger(std::uint32_t vtable,
        std::uint32_t slot, GameManagerPropertyState& state,
        std::int32_t index, std::uint32_t value) = 0;
};

class GameManagerPropertyRuntimeStub : public IGameManagerPropertyRuntime {
public:
    double GetFloat(std::uint32_t, std::uint32_t,
        GameManagerPropertyState&, std::int32_t) override;
    std::uint32_t SetFloat(std::uint32_t, std::uint32_t,
        GameManagerPropertyState&, std::int32_t, float) override;
    std::uint32_t GetInteger(std::uint32_t, std::uint32_t,
        GameManagerPropertyState&, std::int32_t) override;
    std::uint32_t SetInteger(std::uint32_t, std::uint32_t,
        GameManagerPropertyState&, std::int32_t, std::uint32_t) override;
};

class GameManagerPropertyAccess final {
public:
    // IDA 0x440750/0x440770/0x4407a0.
    [[nodiscard]] static double GetFloatArrayElement(
        const GameManagerPropertyState& state, std::int32_t index);
    [[nodiscard]] static double SetFloatArrayElement(
        GameManagerPropertyState& state, std::int32_t index, float value);
    [[nodiscard]] static std::uint32_t AdjustFloatArrayElement(
        GameManagerPropertyState& state, std::int32_t index, float delta,
        IGameManagerPropertyRuntime& runtime);

    // IDA 0x4407d0/0x4407f0/0x440820.
    [[nodiscard]] static double GetFloatGroup1(
        const GameManagerPropertyState& state, std::int32_t index);
    [[nodiscard]] static double SetFloatGroup1(
        GameManagerPropertyState& state, std::int32_t index, float value);
    [[nodiscard]] static std::uint32_t AdjustFloatGroup1(
        GameManagerPropertyState& state, std::int32_t index, float delta,
        IGameManagerPropertyRuntime& runtime);

    // IDA 0x440850/0x440870/0x4408a0.
    [[nodiscard]] static double GetFloatGroup2(
        const GameManagerPropertyState& state, std::int32_t index);
    [[nodiscard]] static double SetFloatGroup2(
        GameManagerPropertyState& state, std::int32_t index, float value);
    [[nodiscard]] static std::uint32_t AdjustFloatGroup2(
        GameManagerPropertyState& state, std::int32_t index, float delta,
        IGameManagerPropertyRuntime& runtime);

    // IDA 0x4408d0/0x4408f0/0x440910.
    [[nodiscard]] static std::uint32_t GetIntegerGroup3(
        const GameManagerPropertyState& state, std::int32_t index);
    [[nodiscard]] static std::uint32_t SetIntegerGroup3(
        GameManagerPropertyState& state, std::int32_t index,
        std::uint32_t value);
    [[nodiscard]] static std::uint32_t AdjustIntegerGroup3(
        GameManagerPropertyState& state, std::int32_t index,
        std::uint32_t delta, IGameManagerPropertyRuntime& runtime);

    // IDA 0x440940/0x440960/0x440990.
    [[nodiscard]] static double GetFloatGroup4(
        const GameManagerPropertyState& state, std::int32_t index);
    [[nodiscard]] static double SetFloatGroup4(
        GameManagerPropertyState& state, std::int32_t index, float value);
    [[nodiscard]] static std::uint32_t AdjustFloatGroup4(
        GameManagerPropertyState& state, std::int32_t index, float delta,
        IGameManagerPropertyRuntime& runtime);

    // IDA 0x4409c0/0x4409e0/0x440a10.
    [[nodiscard]] static double GetFloatGroup5(
        const GameManagerPropertyState& state, std::int32_t index);
    [[nodiscard]] static double SetFloatGroup5(
        GameManagerPropertyState& state, std::int32_t index, float value);
    [[nodiscard]] static std::uint32_t AdjustFloatGroup5(
        GameManagerPropertyState& state, std::int32_t index, float delta,
        IGameManagerPropertyRuntime& runtime);

    // IDA 0x440a40-0x440b70: seven fixed properties with distinct bounds.
    [[nodiscard]] static std::uint32_t Get158(const GameManagerPropertyState& state);
    [[nodiscard]] static std::uint32_t Set158(GameManagerPropertyState& state, std::int32_t value);
    [[nodiscard]] static std::uint32_t Get159(const GameManagerPropertyState& state);
    [[nodiscard]] static std::uint32_t Set159(GameManagerPropertyState& state, std::int32_t value);
    [[nodiscard]] static std::uint32_t Get160(const GameManagerPropertyState& state);
    [[nodiscard]] static std::uint32_t Set160(GameManagerPropertyState& state, std::int32_t value);
    [[nodiscard]] static std::uint32_t Get161(const GameManagerPropertyState& state);
    [[nodiscard]] static std::uint32_t Set161(GameManagerPropertyState& state, std::int32_t value);
    [[nodiscard]] static std::uint32_t Get162(const GameManagerPropertyState& state);
    [[nodiscard]] static std::uint32_t Set162(GameManagerPropertyState& state, std::int32_t value);
    [[nodiscard]] static std::uint32_t Get163(const GameManagerPropertyState& state);
    [[nodiscard]] static std::uint32_t Set163(GameManagerPropertyState& state, std::int32_t value);
    [[nodiscard]] static std::uint32_t Get164(const GameManagerPropertyState& state);
    [[nodiscard]] static std::uint32_t Set164(GameManagerPropertyState& state, std::int32_t value);
    // IDA 0x440b90-0x440bd0.
    [[nodiscard]] static std::uint32_t Get165(const GameManagerPropertyState& state);
    [[nodiscard]] static std::uint32_t Set165(GameManagerPropertyState& state, std::int32_t value);
    [[nodiscard]] static std::uint32_t Get166(const GameManagerPropertyState& state);
    [[nodiscard]] static std::uint32_t Set166(GameManagerPropertyState& state, std::int32_t value);

    // IDA 0x440bf0-0x440c40: direct float adjustment retains its unrounded return.
    [[nodiscard]] static double GetFloatGroup6(
        const GameManagerPropertyState& state, std::int32_t index);
    [[nodiscard]] static double SetFloatGroup6(
        GameManagerPropertyState& state, std::int32_t index, float value);
    [[nodiscard]] static double AdjustFloatGroup6(
        GameManagerPropertyState& state, std::int32_t index, float delta);

    // IDA 0x440c80-0x440cd0.
    [[nodiscard]] static double GetFloatGroup7(
        const GameManagerPropertyState& state, std::int32_t index);
    [[nodiscard]] static double SetFloatGroup7(
        GameManagerPropertyState& state, std::int32_t index, float value);
    [[nodiscard]] static double AdjustFloatGroup7(
        GameManagerPropertyState& state, std::int32_t index, float delta);

    // IDA 0x440d10-0x440d60.
    [[nodiscard]] static double GetFloatGroup8(
        const GameManagerPropertyState& state, std::int32_t index);
    [[nodiscard]] static double SetFloatGroup8(
        GameManagerPropertyState& state, std::int32_t index, float value);
    [[nodiscard]] static double AdjustFloatGroup8(
        GameManagerPropertyState& state, std::int32_t index, float delta);
};

}  // namespace pal4
