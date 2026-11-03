#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
namespace pal4 {
struct BaseObjectState;
class NativeAnimatedActorAsset;
// Publishes a borrowed asset light and routes the original embedded camera
// image through the shared camera lifecycle. The actor image outlives this binding.
class NativeActorShadowBinding final {
public:
    NativeActorShadowBinding(BaseObjectState&,const NativeAnimatedActorAsset&,const std::filesystem::path&);
    ~NativeActorShadowBinding();
    NativeActorShadowBinding(const NativeActorShadowBinding&)=delete;
    NativeActorShadowBinding& operator=(const NativeActorShadowBinding&)=delete;
    static std::uint32_t Enable(std::uint32_t actor,std::uint32_t enabled,std::uint32_t world);
    static void Update(std::uint32_t* camera,float elapsed,std::uint32_t* actor);
    static void Render(std::uint32_t* camera,std::uint32_t scene_camera);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
