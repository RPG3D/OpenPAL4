#include "pal4/world_manager_world_loader.h"

namespace pal4 {

void IWorldManagerWorldLoadRuntime::InitializeScene(std::uint32_t world,std::string_view region,std::string_view scene){
    (void)WorldManagerSceneInitializer::Initialize(world,region,scene,*this);
}
std::uint32_t WorldManagerWorldLoader::Load(
    const std::uint32_t manager, const std::string_view region,
    const std::string_view scene, IWorldManagerWorldLoadServices& runtime) {
    const auto world = manager + 116u;
    runtime.TouchSkillSystem();
    runtime.CompactAtomicList(manager + 312u);
    runtime.CompactAtomicList(manager + 328u);

    if (runtime.StoredText(manager + 348u) != region ||
        runtime.StoredText(manager + 364u) != scene) {
        runtime.InitializeScene(world, region, scene);
        runtime.AssignText(manager + 348u, region);
        runtime.AssignText(manager + 364u, scene);
    }

    const auto current = runtime.CurrentCameraObject();
    runtime.WriteWord(manager + 56u, world);
    runtime.WriteWord(manager + 60u, current);
    runtime.TouchSkillSystem();
    const auto camera = runtime.CurrentCameraObject();
    return runtime.SetWorldCamera(world, runtime.ReadWord(camera));
}

}  // namespace pal4
