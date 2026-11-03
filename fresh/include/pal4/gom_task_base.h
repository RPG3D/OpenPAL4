#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct GomTaskBaseState {
    std::uint32_t vtable{};                 // +0
    std::uint8_t active{};                  // +4
    std::array<std::uint8_t, 3> padding{};
    std::uint32_t name{};                   // +8
    std::uint32_t wait_mode{};              // +0c
    float wait_time{};                      // +10
    float expect_time{};                    // +14
    float total_elapsed{};                  // +18
    float wait_elapsed{};                   // +1c
    float run_elapsed{};                    // +20
    float finish_latch{};                   // +24
    std::uint32_t phase{};                  // +28
    std::uint32_t reserved{};               // +2c
    std::uint32_t previous{};               // +30
    std::uint32_t next{};                   // +34
    std::uint32_t context{};                // +38
};
static_assert(sizeof(GomTaskBaseState) == 0x3cu);
static_assert(offsetof(GomTaskBaseState, name) == 8u);
static_assert(offsetof(GomTaskBaseState, total_elapsed) == 0x18u);
static_assert(offsetof(GomTaskBaseState, phase) == 0x28u);
static_assert(offsetof(GomTaskBaseState, previous) == 0x30u);

struct GomTaskTimingConfig {
    std::uint32_t wait_mode{};
    float wait_time{};
    float expect_time{};
};
static_assert(sizeof(GomTaskTimingConfig) == 12u);

class IGomTaskBaseRuntime {
public:
    virtual ~IGomTaskBaseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void CopyCString(std::uint32_t destination,
                             const char* source, std::uint32_t bytes) = 0;
};

class GomTaskBaseRuntimeStub final : public IGomTaskBaseRuntime {
public:
    [[nodiscard]] std::uint32_t AllocateMemory(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FreeMemory(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void CopyCString(std::uint32_t, const char*, std::uint32_t) override {}
};

class GomTaskBase final {
public:
    static constexpr std::uint32_t kVtable = 0x8496fcu;
    explicit GomTaskBase(IGomTaskBaseRuntime& runtime) : runtime_(runtime) {}

    // 0x5ecb60: initialize observed fields without touching +5..+7.
    GomTaskBaseState& Construct(GomTaskBaseState& state);
    // 0x5ecbc0: release optional name and clear the three link words.
    [[nodiscard]] std::uint32_t Destroy(GomTaskBaseState& state);
    // 0x5ecba0: vtable deleting-destructor code island.
    [[nodiscard]] std::uint32_t DestroyDeleting(
        GomTaskBaseState& state, std::uint32_t object,
        std::uint32_t flags);
    // 0x5ecbf0: replace name and configure timing/link fields.
    [[nodiscard]] std::uint32_t InitializeTimingAndLinks(
        GomTaskBaseState& state, const char* name,
        const GomTaskTimingConfig& timing, std::uint32_t previous,
        std::uint32_t next, std::uint32_t context);
    // 0x5ecc80/90/a0: independent elapsed-time accumulators.
    void AccumulateTotal(GomTaskBaseState& state, float delta);
    void AccumulateWait(GomTaskBaseState& state, float delta);
    void AccumulateRun(GomTaskBaseState& state, float delta);
    // 0x5eccb0: copy the full float bit pattern from +0x18 to +0x24.
    [[nodiscard]] std::uint32_t LatchFinishTime(GomTaskBaseState& state);
    // 0x5eccc0/0x5ecd10/0x5ecd40: task state predicates.
    [[nodiscard]] bool IsWaitComplete(const GomTaskBaseState& state);
    [[nodiscard]] static bool IsDurationComplete(
        const GomTaskBaseState& state);
    [[nodiscard]] static bool ReturnFalse() noexcept { return false; }

private:
    IGomTaskBaseRuntime& runtime_;
};

}  // namespace pal4
