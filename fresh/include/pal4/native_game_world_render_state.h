#pragma once
namespace pal4 {
class NativeGameArenaManagerOwner;
// Original51ACF0/51AD00 through the existing shared algorithm and actual D3D9
// device. The arena and its borrowed fog-settings record must remain alive.
class NativeGameWorldRenderState final {
public:
    static void Apply(NativeGameArenaManagerOwner&);
    static void Clear(NativeGameArenaManagerOwner&);
};
}
