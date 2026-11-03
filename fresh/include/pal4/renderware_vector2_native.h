#pragma once
#include "pal4/renderware_vector2.h"
namespace pal4 {
// Requires a started RenderWare engine: the SDK owns its approximation tables.
class NativeRenderWareVector2 final : public IRenderWareVector2 {
public:
    [[nodiscard]] double Length(float x,float y) override;
    [[nodiscard]] std::pair<float,float> Normalize(float x,float y) override;
};
}
