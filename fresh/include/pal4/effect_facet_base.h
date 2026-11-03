#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

enum class EffectFacetBaseKind : std::uint32_t {
    First, Second, Third, Fourth, Fifth, Sixth, Seventh, Eighth, Ninth, Tenth,
    Eleventh, Twelfth, Thirteenth
};

class IEffectFacetBaseRuntime {
public:
    virtual ~IEffectFacetBaseRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class EffectFacetBaseRuntimeStub final : public IEffectFacetBaseRuntime {
public:
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void Free(std::uint32_t) override {}
};

class EffectFacetBase final {
public:
    static constexpr std::array<std::uint32_t, 13> kVtables{
        0x84a1dcu, 0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
        0x84a1acu, 0x84a1bcu, 0x84a184u, 0x84a178u, 0x84a2c0u,
        0x84a430u, 0x84a420u, 0x84a408u};

    // IDA 0x6069d0, 0x606a00, ... 0x606b50, 0x608e80,
    // 0x60bf20/0x60bf50/0x60bf80.
    static void Initialize(std::uint32_t address, EffectFacetBaseKind kind,
                           IEffectFacetBaseRuntime& runtime);
    // IDA 0x6069e0, 0x606a10, ... 0x606b60, 0x608e90,
    // 0x60bf30/0x60bf60/0x60bf90.
    [[nodiscard]] static std::uint32_t DeletingDestroy(
        std::uint32_t address, EffectFacetBaseKind kind,
        std::uint32_t flags, IEffectFacetBaseRuntime& runtime);
};

}  // namespace pal4
