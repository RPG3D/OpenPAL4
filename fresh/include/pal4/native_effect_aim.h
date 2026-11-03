#pragma once
#include "pal4/clump_object_aim.h"
#include "pal4/effect_object_aim_facet.h"
#include "pal4/effect_object_transform_facets.h"

namespace pal4 {
// Compose shared Clump/EffectObject aim and frame rotation with public RenderWare APIs.
void UpdateNativeClumpAim(ClumpObjectState&,std::uint32_t frame,
                         IClumpObjectTransformRuntime&);
void UpdateNativeEffectAim(EffectObjectState&,std::uint32_t frame,
                          IEffectObjectTransformFacetRuntime&);
}
