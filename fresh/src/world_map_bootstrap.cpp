#include "pal4/world_map_bootstrap.h"
#include "pal4/world_map_frame.h"

namespace pal4 {

std::uint32_t WorldMapBootstrap::GetFrameName(
    IWorldMapBootstrapRuntime& runtime) {
    return InitializeFrameName(runtime);
}

std::uint32_t WorldMapBootstrap::InitializeFrameName(
    IWorldMapBootstrapRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    runtime.WriteWord(kCurrentName, name);
    return name;
}

std::uint32_t WorldMapBootstrap::CreateMap(
    const std::uint32_t name, IWorldMapBootstrapRuntime& runtime) {
    const auto storage = runtime.Allocate(0x274u);
    if (storage == 0u) {
        return 0u;
    }
    try {
        return runtime.ConstructMap(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t WorldMapBootstrap::CreateMap(
    const std::uint32_t name, IWorldMapBootstrapRuntime& runtime,
    IWorldMapFrameRuntime& frame_runtime) {
    const auto storage = runtime.Allocate(0x274u);
    if (storage == 0u) {
        return 0u;
    }
    try {
        return WorldMapFrame::Construct(storage, name, frame_runtime);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t WorldMapBootstrap::GetFactory(
    IWorldMapBootstrapRuntime& runtime) {
    return SetupFrame(runtime);
}

std::uint32_t WorldMapBootstrap::SetupFrame(
    IWorldMapBootstrapRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.ProcessFrameEvent(manager,
        runtime.ReadWord(kCurrentName), kFactoryAddress);
}

}  // namespace pal4
