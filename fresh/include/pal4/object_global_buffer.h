#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ObjectGlobalBufferState {
    std::uint8_t cleanup_guard = 0;
    std::array<std::uint8_t, 3> reserved0{};
    std::uint8_t source_byte = 0;
    std::array<std::uint8_t, 3> reserved1{};
    std::array<std::uint32_t, 3> words{};
};
static_assert(sizeof(ObjectGlobalBufferState) == 20u);
static_assert(offsetof(ObjectGlobalBufferState, source_byte) == 4u);
static_assert(offsetof(ObjectGlobalBufferState, words) == 8u);

class IObjectGlobalBufferRuntime {
public:
    virtual ~IObjectGlobalBufferRuntime() = default;
    [[nodiscard]] virtual int RegisterCleanup(
        std::uint32_t callback_address) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ObjectGlobalBufferRuntimeStub final : public IObjectGlobalBufferRuntime {
public:
    int RegisterCleanup(std::uint32_t callback_address) override;
    void Free(std::uint32_t address) override;
};

class ObjectGlobalBuffer final {
public:
    static constexpr std::uint32_t kStateAddress = 0x8e41f8u;
    static constexpr std::uint32_t kCleanupAddress = 0x42c880u;

    static void InitializeMembers(ObjectGlobalBufferState& state,
                                  std::uint32_t ecx_value);
    [[nodiscard]] static int InitializeAndRegister(
        ObjectGlobalBufferState& state, std::uint32_t ecx_value,
        IObjectGlobalBufferRuntime& runtime);
    static void CleanupOnce(ObjectGlobalBufferState& state,
                            IObjectGlobalBufferRuntime& runtime);
};

}  // namespace pal4
