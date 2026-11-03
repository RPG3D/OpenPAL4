#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct RevoluteTaskState final {
    std::array<std::uint32_t, 34u> words{};
};
static_assert(sizeof(RevoluteTaskState) == 0x88u);

class IRevoluteTaskRuntime {
public:
    virtual ~IRevoluteTaskRuntime() = default;
    virtual void ConstructBase(RevoluteTaskState& state) = 0;
    virtual void DestroyBase(RevoluteTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    [[nodiscard]] virtual bool CameraIsActive(std::uint32_t facet) = 0;
    [[nodiscard]] virtual bool CameraHasTarget(std::uint32_t facet) = 0;
    virtual void ReadCameraTarget(std::uint32_t facet,
                                  std::array<float, 3u>& target) = 0;
    virtual void ApplyRevolution(std::uint32_t facet,
                                 const std::array<float, 3u>& origin,
                                 const std::array<float, 6u>& motion) = 0;
};

class IRevoluteConfigRuntime {
public:
    virtual ~IRevoluteConfigRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t config,
                                           std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(std::uint32_t config,
                                               std::string_view key,
                                               std::int32_t fallback) = 0;
};

class RevoluteTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84ada0u;

    // IDA 0x620610..0x620670.
    static RevoluteTaskState& Construct(RevoluteTaskState& state,
                                        IRevoluteTaskRuntime& runtime);
    static RevoluteTaskState& Destroy(RevoluteTaskState& state,
                                      IRevoluteTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x620680..0x620b10.
    [[nodiscard]] static bool LoadConfig(RevoluteTaskState& state,
                                          std::uint32_t config,
                                          IRevoluteConfigRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(RevoluteTaskState& state,
                                             IRevoluteTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(RevoluteTaskState& state,
                                              float elapsed,
                                              IRevoluteTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopAndCorrect(
        RevoluteTaskState& state, IRevoluteTaskRuntime& runtime);
};

}  // namespace pal4
