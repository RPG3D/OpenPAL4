#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ShowobjTaskState final {
    std::array<std::uint32_t, 19u> words{};
};
static_assert(sizeof(ShowobjTaskState) == 0x4cu);

class IShowobjTaskRuntime {
public:
    virtual ~IShowobjTaskRuntime() = default;
    virtual void ConstructBase(ShowobjTaskState& state) = 0;
    virtual void DestroyBase(ShowobjTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    virtual void Show(std::uint32_t facet) = 0;
};

class ShowobjTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84aee8u;
    static ShowobjTaskState& Construct(ShowobjTaskState& state,
                                       IShowobjTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    static ShowobjTaskState& Destroy(ShowobjTaskState& state,
                                     IShowobjTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(ShowobjTaskState& state,
                                             IShowobjTaskRuntime& runtime);
};

}  // namespace pal4
