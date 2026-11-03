#include "pal4/system_dialog_bootstrap.h"
#include "pal4/system_dialog_frame.h"

namespace pal4 {

std::uint32_t SystemDialogBootstrap::GetFrameName(
    ISystemDialogBootstrapRuntime& runtime) {
    return InitializeFrameName(runtime);
}

std::uint32_t SystemDialogBootstrap::InitializeFrameName(
    ISystemDialogBootstrapRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    runtime.WriteWord(kCurrentName, name);
    return name;
}

std::uint32_t SystemDialogBootstrap::CreateDialog(
    const std::uint32_t name, ISystemDialogBootstrapRuntime& runtime) {
    const auto storage = runtime.Allocate(0x30u);
    if (storage == 0u) {
        return 0u;
    }
    try {
        return runtime.ConstructDialog(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t SystemDialogBootstrap::CreateDialog(
    const std::uint32_t name, ISystemDialogBootstrapRuntime& runtime,
    ISystemDialogFrameRuntime& frame_runtime) {
    const auto storage = runtime.Allocate(0x30u);
    if (storage == 0u) {
        return 0u;
    }
    try {
        return SystemDialogFrame::Construct(storage, name, frame_runtime);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t SystemDialogBootstrap::GetFactory(
    ISystemDialogBootstrapRuntime& runtime) {
    return SetupFrame(runtime);
}

std::uint32_t SystemDialogBootstrap::SetupFrame(
    ISystemDialogBootstrapRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.ProcessFrameEvent(manager,
        runtime.ReadWord(kCurrentName), kFactoryAddress);
}

}  // namespace pal4
