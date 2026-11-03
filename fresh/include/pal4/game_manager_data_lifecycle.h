#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct GameManagerDataState {
    std::array<std::uint32_t, 177> words{};
};
static_assert(sizeof(GameManagerDataState) == 0x2c4u);

class IGameManagerDataMemory {
public:
    virtual ~IGameManagerDataMemory() = default;
    virtual void Free(std::uint32_t address) = 0;
};

class GameManagerDataMemoryStub final : public IGameManagerDataMemory {
public:
    void Free(std::uint32_t address) override;
};

class GameManagerDataLifecycle final {
public:
    static constexpr std::uint32_t kCleanupVtable = 0x8430e8u;
    static constexpr std::uint32_t kBaseVtable = 0x843270u;

    // IDA 0x440da0. The final vtable comes from class_init_vtable at 0x5835a0.
    static void Cleanup(GameManagerDataState& state,
                        IGameManagerDataMemory& memory);
    // IDA 0x440e00.
    [[nodiscard]] static std::uint32_t DestroyDeleting(
        GameManagerDataState& state, std::uint32_t object,
        std::uint32_t flags, IGameManagerDataMemory& memory);
};

}  // namespace pal4
