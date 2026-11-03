#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PauseEffectTaskState final {
    std::array<std::uint32_t, 19u> words{};
};
static_assert(sizeof(PauseEffectTaskState) == 0x4cu);

// Base object and EffectSystem facet dispatch for IDA 0x61f960..0x61f9c0.
class IPauseEffectTaskRuntime {
public:
    virtual ~IPauseEffectTaskRuntime() = default;
    virtual void ConstructBase(PauseEffectTaskState& state) = 0;
    virtual void DestroyBase(PauseEffectTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    virtual void Pause(std::uint32_t facet) = 0;
};

class PauseEffectTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84ace0u;

    // IDA 0x61f960/0x61f9a0/0x61f9b0/0x61f9c0.
    static PauseEffectTaskState& Construct(PauseEffectTaskState& state,
                                           IPauseEffectTaskRuntime& runtime);
    static PauseEffectTaskState& Destroy(PauseEffectTaskState& state,
                                         IPauseEffectTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static std::uint8_t Start(PauseEffectTaskState& state,
                                            IPauseEffectTaskRuntime& runtime);
};

}  // namespace pal4
