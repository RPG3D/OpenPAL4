#pragma once
#include "pal4/cegui_immediate_rectangle.h"
namespace pal4 {
// Native SDK boundary for the original native/scaled/wide immediate paths.
std::uint32_t DrawNativeCeguiImmediateRectangle(const CeguiRendererState&,
    bool wide,const CeguiRectWords&,std::uint32_t texture,const CeguiRectWords& uv,
    const std::array<CeguiColourWords,4>&, bool native=false);
}
