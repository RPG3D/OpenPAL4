#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct StopSound3DTaskState final {
    std::array<std::uint32_t, 19u> words{};
};
static_assert(sizeof(StopSound3DTaskState) == 0x4cu);

class IStopSound3DTaskRuntime {
public:
    virtual ~IStopSound3DTaskRuntime() = default;
    virtual void ConstructBase(StopSound3DTaskState& state) = 0;
    virtual void DestroyBase(StopSound3DTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    virtual void StopSound(std::uint32_t facet) = 0;
};

class StopSound3DTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84afe8u;
    static StopSound3DTaskState& Construct(StopSound3DTaskState& state,
                                           IStopSound3DTaskRuntime& runtime);
    static StopSound3DTaskState& Destroy(StopSound3DTaskState& state,
                                         IStopSound3DTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static std::uint8_t Start(StopSound3DTaskState& state,
                                             IStopSound3DTaskRuntime& runtime);
};

}  // namespace pal4
