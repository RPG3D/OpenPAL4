#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct UVTranslateTaskState final {
    std::array<std::uint32_t, 23u> words{};
};
static_assert(sizeof(UVTranslateTaskState) == 0x5cu);

class IUVTranslateTaskRuntime {
public:
    virtual ~IUVTranslateTaskRuntime() = default;
    virtual void ConstructBase(UVTranslateTaskState& state) = 0;
    virtual void DestroyBase(UVTranslateTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view name) = 0;
    virtual void StartTranslation(std::uint32_t facet,
                                  std::int32_t layer) = 0;
    virtual void UpdateTranslation(std::uint32_t facet, std::int32_t layer,
                                   const std::array<float, 3u>& delta) = 0;
    virtual void StopTranslation(std::uint32_t facet,
                                 std::int32_t layer) = 0;
};

class IUVTranslateTaskConfigRuntime {
public:
    virtual ~IUVTranslateTaskConfigRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t fallback) = 0;
};

class UVTranslateTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84b0f0u;
    static UVTranslateTaskState& Construct(UVTranslateTaskState& state,
                                           IUVTranslateTaskRuntime& runtime);
    static UVTranslateTaskState& Destroy(UVTranslateTaskState& state,
                                         IUVTranslateTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static std::uint8_t Start(UVTranslateTaskState& state,
                                            IUVTranslateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(UVTranslateTaskState& state,
                                             float elapsed,
                                             IUVTranslateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(UVTranslateTaskState& state,
                                           IUVTranslateTaskRuntime& runtime);
    [[nodiscard]] static bool LoadConfig(
        UVTranslateTaskState& state, std::uint32_t config,
        IUVTranslateTaskConfigRuntime& runtime);
};

}  // namespace pal4
