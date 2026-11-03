#pragma once
#include <cstdint>
namespace pal4 {
struct BaseObjectCallbackRecord;
class NativeAnimatedActorAsset;
class NativePlayerTransformRuntime;
class NativeBaseObjectMemory;
struct NativeBaseObjectSceneFrameResult final {
    std::uint32_t active_objects{},render_successes{},visible_objects{},animation_calls{},character_updates{};
};
// Shared native binding for original442930: clump -> embedded animation ->
// actual CollisionActor callbacks. Callers retain their owner/lifecycle guards.
class NativeBaseObjectSceneFrame final {
public:
    static void Render(BaseObjectCallbackRecord&,const NativeAnimatedActorAsset&,NativePlayerTransformRuntime&,
        NativeBaseObjectMemory&,std::uint32_t camera,NativeBaseObjectSceneFrameResult&);
};
}
