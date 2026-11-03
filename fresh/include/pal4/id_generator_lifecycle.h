#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct IdGeneratorGlobalState {
    std::uint8_t seed{};
    std::array<std::uint8_t, 3> padding{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity{};
    std::uint8_t guard{};
    std::array<std::uint8_t, 3> tail_padding{};
};
static_assert(sizeof(IdGeneratorGlobalState) == 20u);
static_assert(offsetof(IdGeneratorGlobalState, begin) == 4u);
static_assert(offsetof(IdGeneratorGlobalState, guard) == 16u);

class IIdGeneratorLifecycleRuntime {
public:
    virtual ~IIdGeneratorLifecycleRuntime() = default;
    virtual void RegisterAtExit(std::uint32_t callback) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class IdGeneratorLifecycleRuntimeStub final : public IIdGeneratorLifecycleRuntime {
public:
    void RegisterAtExit(std::uint32_t) override {}
    void FreeMemory(std::uint32_t) override {}
};

class IdGeneratorLifecycle final {
public:
    static constexpr std::uint32_t kGlobalAddress = 0x90f12cu;
    static constexpr std::uint32_t kCleanupCallback = 0x5ec810u;

    explicit IdGeneratorLifecycle(IIdGeneratorLifecycleRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5ec7c0: the seed comes from the high byte of saved ECX.
    [[nodiscard]] std::uint32_t Initialize(
        IdGeneratorGlobalState& state, std::uint32_t incoming_ecx);
    // 0x5ec810: the allocator is called even when begin is zero.
    void Cleanup(IdGeneratorGlobalState& state);

private:
    IIdGeneratorLifecycleRuntime& runtime_;
};

}  // namespace pal4
