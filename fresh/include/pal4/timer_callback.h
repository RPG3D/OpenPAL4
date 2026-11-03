#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct TimerCallbackState {
    std::uint32_t vtable{};
    std::uint32_t reserved_04{};
    std::uint32_t owner{};
    std::uint32_t function{};
    std::uint32_t argument{};
    std::uint32_t reserved_14{};
};

static_assert(sizeof(TimerCallbackState) == 0x18u);
static_assert(offsetof(TimerCallbackState, owner) == 0x8u);

class ITimerCallbackRuntime {
public:
    virtual ~ITimerCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class TimerCallback final {
public:
    static constexpr std::uint32_t kFirstVtable = 0x843ae8u;
    static constexpr std::uint32_t kSecondVtable = 0x843b34u;
    static constexpr std::uint32_t kBaseVtable = 0x843500u;

    [[nodiscard]] static TimerCallbackState& ConstructFirst(
        TimerCallbackState& callback, std::uint32_t owner,
        std::uint32_t function, std::uint32_t argument);
    [[nodiscard]] static TimerCallbackState& ConstructSecond(
        TimerCallbackState& callback, std::uint32_t owner,
        std::uint32_t function, std::uint32_t argument);
    [[nodiscard]] static std::uint32_t Clone(
        const TimerCallbackState& source, ITimerCallbackRuntime& runtime);
};

}  // namespace pal4
