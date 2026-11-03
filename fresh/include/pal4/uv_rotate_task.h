#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct UVRotateTaskState final {
    std::array<std::uint32_t, 24u> words{};
};
static_assert(sizeof(UVRotateTaskState) == 0x60u);

class IUVRotateTaskRuntime {
public:
    virtual ~IUVRotateTaskRuntime() = default;
    virtual void ConstructBase(UVRotateTaskState& state) = 0;
    virtual void DestroyBase(UVRotateTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view name) = 0;
    virtual void StartRotation(std::uint32_t facet, std::int32_t layer) = 0;
    virtual void UpdateRotation(std::uint32_t facet, std::int32_t layer,
                                const std::array<float, 3u>& axis,
                                float angular_step) = 0;
    virtual void StopRotation(std::uint32_t facet, std::int32_t layer) = 0;
};

class IUVRotateTaskConfigRuntime {
public:
    virtual ~IUVRotateTaskConfigRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t fallback) = 0;
};

class UVRotateTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84b0b0u;
    static UVRotateTaskState& Construct(UVRotateTaskState& state,
                                        IUVRotateTaskRuntime& runtime);
    static UVRotateTaskState& Destroy(UVRotateTaskState& state,
                                      IUVRotateTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static std::uint8_t Start(UVRotateTaskState& state,
                                            IUVRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(UVRotateTaskState& state,
                                             float elapsed,
                                             IUVRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(UVRotateTaskState& state,
                                           IUVRotateTaskRuntime& runtime);
    [[nodiscard]] static bool LoadConfig(UVRotateTaskState& state,
                                         std::uint32_t config,
                                         IUVRotateTaskConfigRuntime& runtime);
};

}  // namespace pal4
