#pragma once
#include "pal4/portable/world.h"
#include <memory>
namespace pal4::portable {
class SceneMovement {
public:
    // Borrows geometry owned by the current arena; recreate on scene change.
    explicit SceneMovement(std::span<const CollisionTriangle> primary,std::span<const CollisionTriangle> secondary={});
    ~SceneMovement();
    SceneMovement(const SceneMovement&)=delete;
    SceneMovement& operator=(const SceneMovement&)=delete;
    bool SelectAndUpdate(Point3 position,float above,float below);
    bool CheckActive(Point3 position,float above,float below);
    bool CheckAndAdjust(Point3& position,float above,float below);
    // Monster behavior calls the primary composite directly. Does not switch
    // the active player movement child or overwrite its last probe result.
    bool CheckPrimaryAndAdjust(Point3& position,float above,float below);
    unsigned ActiveChild() const;
    std::optional<FloorHit> LastHit() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
