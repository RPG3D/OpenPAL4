#pragma once
#include "pal4/clump_spatial_bounds.h"
#include <rwcore.h>

namespace pal4 {
// Im3D keeps the input pointer through Render/End. This adapter owns typed
// storage for that interval; one SDK immediate session may be active at a time.
// Keep the adapter alive until End, including after a failed Transform. The
// original game failure branch skips End; this adapter does not hide that policy.
class NativeClumpBoundsRenderRuntime final : public IClumpBoundsRenderRuntime {
public:
    int SetRenderState(int state,int value) override;
    bool TransformImmediate(const std::array<ClumpBoundsVertex,8>& vertices,
                            std::uint32_t matrix,int flags) override;
    void RenderIndexedLines(int primitive,int index_count) override;
    void EndImmediate() override;
private:
    std::array<RwIm3DVertex,8> vertices_{};
};
}
