#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct HideobjTaskState final {
    std::array<std::uint32_t, 19u> words{};
};
static_assert(sizeof(HideobjTaskState) == 0x4cu);

class IHideobjTaskRuntime {
public:
    virtual ~IHideobjTaskRuntime() = default;
    virtual void ConstructBase(HideobjTaskState& state) = 0;
    virtual void DestroyBase(HideobjTaskState& state) = 0;
};

class IHideobjFacetRuntime {
public:
    virtual ~IHideobjFacetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    virtual void Hide(std::uint32_t facet) = 0;
};

class HideobjTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84ac60u;

    // IDA 0x61e350/0x61e370/0x61e3a0/0x61e3b0.
    static HideobjTaskState& Construct(HideobjTaskState& state,
                                       IHideobjTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    static HideobjTaskState& Destroy(HideobjTaskState& state,
                                     IHideobjTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(HideobjTaskState& state,
                                             IHideobjFacetRuntime& runtime);
};

}  // namespace pal4
