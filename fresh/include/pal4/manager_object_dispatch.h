#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ManagerObjectDispatchState {
    std::uint32_t vtable{};
    std::uint8_t initialized{};
    std::uint8_t enabled{};
    std::uint8_t reserved[2]{};
    std::uint32_t primary_target{};
    std::uint32_t secondary_target{};
};
static_assert(sizeof(ManagerObjectDispatchState) == 16u);
static_assert(offsetof(ManagerObjectDispatchState, initialized) == 4u);
static_assert(offsetof(ManagerObjectDispatchState, enabled) == 5u);
static_assert(offsetof(ManagerObjectDispatchState, primary_target) == 8u);
static_assert(offsetof(ManagerObjectDispatchState, secondary_target) == 12u);

class IManagerObjectDispatchRuntime {
public:
    virtual ~IManagerObjectDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Invoke(std::uint32_t target,
                                                std::uint32_t slot_offset) = 0;
};

class ManagerObjectDispatchRuntimeStub final
    : public IManagerObjectDispatchRuntime {
public:
    [[nodiscard]] std::uint32_t Invoke(std::uint32_t target,
                                        std::uint32_t slot_offset) override;
};

class ManagerObjectDispatch final {
public:
    [[nodiscard]] static std::uint8_t Begin(
        const ManagerObjectDispatchState& state,
        IManagerObjectDispatchRuntime& runtime);
    [[nodiscard]] static std::uint8_t End(
        const ManagerObjectDispatchState& state,
        IManagerObjectDispatchRuntime& runtime);
    [[nodiscard]] static std::uint8_t BeginSecondary(
        const ManagerObjectDispatchState& state,
        IManagerObjectDispatchRuntime& runtime);
    [[nodiscard]] static std::uint8_t EndSecondary(
        const ManagerObjectDispatchState& state,
        IManagerObjectDispatchRuntime& runtime);
    [[nodiscard]] static std::uint8_t ReturnFalse(
        std::uint32_t first, std::uint32_t second);

private:
    [[nodiscard]] static std::uint8_t Invoke(
        const ManagerObjectDispatchState& state, std::uint32_t target,
        std::uint32_t slot_offset,
        IManagerObjectDispatchRuntime& runtime);
};

}  // namespace pal4
