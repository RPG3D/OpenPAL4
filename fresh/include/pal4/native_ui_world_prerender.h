#pragma once
#include <cstdint>
namespace pal4 {
class NativeUiCollectionObjects;
class NativeEffectSceneSession;
// IDA4BF270: world traversal, UI effects, then the collection's transparent queue.
// Borrows the active camera and owners for this call; it does not draw CEGUI.
void NativeUiWorldPreRender(std::uint32_t manager,std::uint32_t camera,
    NativeUiCollectionObjects&,NativeEffectSceneSession&);
}
