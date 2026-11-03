#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct WobbleTaskState final {
    std::array<std::uint32_t, 21u> words{};
};
static_assert(sizeof(WobbleTaskState) == 0x54u);

class IWobbleTaskRuntime {
public:
    virtual ~IWobbleTaskRuntime() = default;
    virtual void ConstructBase(WobbleTaskState& state) = 0;
    virtual void DestroyBase(WobbleTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolvePositionFacet(
        std::uint32_t owner, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t TickCountMilliseconds() = 0;
    virtual void MovePosition(std::uint32_t facet, float x, float y,
                              float z, bool incremental) = 0;
};

class IWobbleTaskConfigRuntime {
public:
    virtual ~IWobbleTaskConfigRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key, float fallback) = 0;
};

class WobbleTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84b130u;
    static WobbleTaskState& Construct(WobbleTaskState& state,
                                      IWobbleTaskRuntime& runtime);
    static WobbleTaskState& Destroy(WobbleTaskState& state,
                                    IWobbleTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static std::uint8_t Start(WobbleTaskState& state,
                                            IWobbleTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(WobbleTaskState& state,
                                             IWobbleTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(WobbleTaskState& state);
    [[nodiscard]] static bool LoadConfig(WobbleTaskState& state,
                                         std::uint32_t config,
                                         IWobbleTaskConfigRuntime& runtime);
};

}  // namespace pal4
