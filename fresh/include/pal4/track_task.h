#pragma once

#include "pal4/track_point_array.h"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace pal4 {

struct TrackTaskState final {
    std::array<std::uint32_t, 36u> words{};
};
static_assert(sizeof(TrackTaskState) == 0x90u);

class ITrackTaskRuntime {
public:
    virtual ~ITrackTaskRuntime() = default;
    virtual void ConstructBase(TrackTaskState& state) = 0;
    virtual void DestroyBase(TrackTaskState& state) = 0;
    virtual void FreeNativePointBuffer(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ConstructorStackByte() = 0;
    virtual void TransformPoints(std::uint32_t, TrackPointArray&) {}
    [[nodiscard]] virtual TrackPoint Normalize(const TrackPoint& point) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    virtual void SetPosition(std::uint32_t facet, const TrackPoint& point) = 0;
    virtual void FacePoint(std::uint32_t facet,
                           const TrackPointArray& points,
                           std::size_t index) = 0;
};

class ITrackTaskConfigRuntime {
public:
    virtual ~ITrackTaskConfigRuntime() = default;
    [[nodiscard]] virtual std::vector<std::uint32_t> Children(
        std::uint32_t config, std::string_view name) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t node, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadTreeInt(
        std::uint32_t node, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t node, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual std::uint32_t CreateSpline(
        const TrackPointArray&, std::int32_t) { return 0u; }
    [[nodiscard]] virtual TrackPoint SampleSpline(
        std::uint32_t, std::int32_t, float) { return {}; }
    virtual void DestroySpline(std::uint32_t) {}
    virtual void SynchronizePointStorage(TrackTaskState&,
                                         const TrackPointArray&) {}
};

class TrackTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84b030u;
    static TrackTaskState& Construct(TrackTaskState& state,
                                     ITrackTaskRuntime& runtime);
    static TrackTaskState& Destroy(TrackTaskState& state,
                                   TrackPointArray& points,
                                   ITrackTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static bool LoadConfig(
        TrackTaskState& state, std::uint32_t config,
        TrackPointArray& points, ITrackTaskConfigRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(
        TrackTaskState& state, TrackPointArray& points,
        ITrackTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        TrackTaskState& state, float elapsed, const TrackPointArray& points,
        ITrackTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(TrackTaskState& state);
};

}  // namespace pal4
