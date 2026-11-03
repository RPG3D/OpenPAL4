#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

using PositionVector = std::array<float, 3u>;

struct PositionTaskState final {
    std::array<std::uint32_t, 57u> words{};
};
static_assert(sizeof(PositionTaskState) == 0xe4u);

class IPositionTaskConfigRuntime {
public:
    virtual ~IPositionTaskConfigRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual float RandomFloat(float lower, float upper) = 0;
};

class IPositionTaskRuntime {
public:
    virtual ~IPositionTaskRuntime() = default;
    virtual void ConstructBase(PositionTaskState& state) = 0;
    virtual void DestroyBase(PositionTaskState& state) = 0;
    virtual void FreeObject(PositionTaskState& state) = 0;
    [[nodiscard]] virtual bool BaseDurationComplete(
        const PositionTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolvePositionFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    [[nodiscard]] virtual PositionVector GetPosition(std::uint32_t facet) = 0;
    virtual void Move(std::uint32_t facet, const PositionVector& value,
                      bool incremental) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFaceFacet(
        std::uint32_t owner) = 0;
    virtual void Face(std::uint32_t facet, const PositionVector& direction) = 0;
    virtual void SetRotation(std::uint32_t facet,
                             const PositionVector& rotation) = 0;
    virtual void ActivateFace(std::uint32_t facet) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveControlFacet(
        std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint8_t ControlEnabled(
        std::uint32_t facet) = 0;
    virtual void SetControlEnabled(std::uint32_t facet,
                                   std::uint8_t enabled) = 0;
    [[nodiscard]] virtual int RandomInt() = 0;
};

class PositionTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84b070u;
    static PositionTaskState& Construct(PositionTaskState& state,
                                        IPositionTaskRuntime& runtime);
    static PositionTaskState& Destroy(PositionTaskState& state,
                                      IPositionTaskRuntime& runtime);
    static PositionTaskState& Delete(PositionTaskState& state,
                                     std::uint8_t flags,
                                     IPositionTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static bool LoadConfig(PositionTaskState& state,
                                         std::uint32_t config,
                                         IPositionTaskConfigRuntime& runtime);
    [[nodiscard]] static float ReadOptionalFloat(
        IPositionTaskConfigRuntime& runtime, std::uint32_t config,
        std::string_view key, float fallback);
    [[nodiscard]] static bool IsDurationComplete(
        const PositionTaskState& state, IPositionTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(PositionTaskState& state,
                                            IPositionTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(PositionTaskState& state,
                                             float elapsed,
                                             IPositionTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(PositionTaskState& state,
                                           IPositionTaskRuntime& runtime);
};

}  // namespace pal4
