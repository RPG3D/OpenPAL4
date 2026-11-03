#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct StartCameraTaskState final {
    std::array<std::uint32_t, 19u> words{};
};
static_assert(sizeof(StartCameraTaskState) == 0x4cu);

class IStartCameraTaskRuntime {
public:
    virtual ~IStartCameraTaskRuntime() = default;
    virtual void ConstructBase(StartCameraTaskState& state) = 0;
    virtual void DestroyBase(StartCameraTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    virtual void StartCamera(std::uint32_t facet) = 0;
};

class StartCameraTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84af28u;
    static StartCameraTaskState& Construct(StartCameraTaskState& state,
                                           IStartCameraTaskRuntime& runtime);
    static StartCameraTaskState& Destroy(StartCameraTaskState& state,
                                         IStartCameraTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static std::uint8_t Start(StartCameraTaskState& state,
                                             IStartCameraTaskRuntime& runtime);
};

}  // namespace pal4
