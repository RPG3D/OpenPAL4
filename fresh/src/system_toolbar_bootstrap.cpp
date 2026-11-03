#include "pal4/system_toolbar_bootstrap.h"
#include "pal4/system_toolbar_frame.h"

namespace pal4 {

std::uint32_t SystemToolbarBootstrap::GetFrameName(
    ISystemToolbarBootstrapRuntime& runtime) {
    return InitializeFrameName(runtime);
}

std::uint32_t SystemToolbarBootstrap::InitializeFrameName(
    ISystemToolbarBootstrapRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    runtime.WriteWord(kCurrentName, name);
    return name;
}

std::uint32_t SystemToolbarBootstrap::CreateFrame(
    const std::uint32_t name, ISystemToolbarBootstrapRuntime& runtime) {
    const auto storage = runtime.Allocate(0x5cu);
    if (storage == 0u) {
        return 0u;
    }
    try {
        return runtime.ConstructFrame(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t SystemToolbarBootstrap::CreateFrame(
    const std::uint32_t name, ISystemToolbarBootstrapRuntime& runtime,
    ISystemToolbarFrameRuntime& frame_runtime) {
    const auto storage = runtime.Allocate(0x5cu);
    if (storage == 0u) {
        return 0u;
    }
    try {
        return SystemToolbarFrame::Construct(storage, name, frame_runtime);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t SystemToolbarBootstrap::CreateUI(
    ISystemToolbarBootstrapRuntime& runtime) {
    return SetupFrame(runtime);
}

std::uint32_t SystemToolbarBootstrap::SetupFrame(
    ISystemToolbarBootstrapRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto name = runtime.ReadWord(kCurrentName);
    return runtime.ProcessFrameEvent(manager, name, kFactoryAddress);
}

}  // namespace pal4
