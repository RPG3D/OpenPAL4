#pragma once
#include <cstdint>
namespace pal4 {
class NativeBehaviorGridAssociationRuntime;
class NativeBehaviorCurveMovement final {
public:
    // Original class4 A4=42C520; actual image is C0, not the C8 shared prefix.
    static void Update(std::uint32_t helper,float elapsed,NativeBehaviorGridAssociationRuntime&);
};
}
