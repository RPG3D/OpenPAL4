#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct RewindEffectTaskState final {
    std::array<std::uint32_t, 19u> words{};
};
static_assert(sizeof(RewindEffectTaskState) == 0x4cu);

// Native EffectSystem base and facet dispatch for IDA 0x6210f0..0x621150.
class IRewindEffectTaskRuntime {
public:
    virtual ~IRewindEffectTaskRuntime() = default;
    virtual void ConstructBase(RewindEffectTaskState& state) = 0;
    virtual void DestroyBase(RewindEffectTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    virtual void Rewind(std::uint32_t facet) = 0;
};

class RewindEffectTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84ade0u;

    // IDA 0x6210f0/0x621130/0x621140/0x621150.
    static RewindEffectTaskState& Construct(RewindEffectTaskState& state,
                                            IRewindEffectTaskRuntime& runtime);
    static RewindEffectTaskState& Destroy(RewindEffectTaskState& state,
                                          IRewindEffectTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static std::uint8_t Start(RewindEffectTaskState& state,
                                             IRewindEffectTaskRuntime& runtime);
};

}  // namespace pal4
