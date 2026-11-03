#pragma once
#include "pal4/portable/scene_data.h"
#include <memory>
#include <span>

namespace pal4::portable {
struct ScenePathHandle {std::uint64_t generation{};std::uint32_t index{};};
struct ScenePathProjection {
    std::array<float,3> point{},direction{};
    float outside{};
};
// Immutable source snapshot, owned original PathwayGrid arrays and per-path
// query scratch. Single game thread; generation must change on every reload.
class ScenePathways {
public:
    ScenePathways(std::uint64_t generation,std::span<const ScenePath> records);
    ~ScenePathways();
    ScenePathways(const ScenePathways&)=delete;
    ScenePathways& operator=(const ScenePathways&)=delete;
    std::uint64_t Generation() const;
    std::size_t Size() const;
    ScenePathHandle Handle(std::size_t index) const;
    // Original lookup returns the first matching record, including grid-less ones.
    std::optional<ScenePathHandle> Find(std::string_view name) const;
    const ScenePath& Source(ScenePathHandle) const;
    bool HasGrid(ScenePathHandle) const;
    std::uint32_t PointCount(ScenePathHandle) const;
    std::array<float,3> Point(ScenePathHandle,std::size_t index) const;
    float Length(ScenePathHandle) const;
    float ClosestPosition(ScenePathHandle,const std::array<float,3>& point);
    std::array<float,3> CoordinatesAt(ScenePathHandle,float distance);
    ScenePathProjection ProjectToTube(ScenePathHandle,const std::array<float,3>& point,
        const std::array<float,3>& initial_direction);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
