#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct StopCameraTaskState final {
    std::array<std::uint32_t, 19u> words{};
};
static_assert(sizeof(StopCameraTaskState) == 0x4cu);

class IStopCameraTaskRuntime {
public:
    virtual ~IStopCameraTaskRuntime() = default;
    virtual void ConstructBase(StopCameraTaskState& state) = 0;
    virtual void DestroyBase(StopCameraTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    virtual void StopCamera(std::uint32_t facet) = 0;
    [[nodiscard]] virtual std::uint32_t GetOwnerFrame(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t GetFrameParent(std::uint32_t) {
        return 0u;
    }
    virtual void RemoveFrameChild(std::uint32_t) {}
};

class StopCameraTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84af68u;
    static StopCameraTaskState& Construct(StopCameraTaskState& state,
                                          IStopCameraTaskRuntime& runtime);
    static StopCameraTaskState& Destroy(StopCameraTaskState& state,
                                        IStopCameraTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static std::uint8_t Start(StopCameraTaskState& state,
                                             IStopCameraTaskRuntime& runtime);
};

}  // namespace pal4
