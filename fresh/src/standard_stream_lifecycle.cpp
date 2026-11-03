#include "pal4/standard_stream_lifecycle.h"

namespace pal4 {

std::uint32_t StandardStreamLifecycle::ConstructNarrow(
    IStandardStreamRuntime& runtime) const {
    return runtime.ConstructNarrow(kNarrowObjectAddress);
}

int StandardStreamLifecycle::RegisterNarrowExit(
    IStandardStreamRuntime& runtime) const {
    return runtime.RegisterExit(kNarrowExitAddress);
}

int StandardStreamLifecycle::InitializeNarrow(
    IStandardStreamRuntime& runtime) const {
    (void)ConstructNarrow(runtime);
    return RegisterNarrowExit(runtime);
}

void StandardStreamLifecycle::CleanupNarrow(IStandardStreamRuntime& runtime) {
    if (--narrow_count_ != 0u) return;
    runtime.FlushNarrow(0x950678u);
    runtime.FlushNarrow(0x9507a0u);
    runtime.FlushNarrow(0x9505e8u);
}

std::uint32_t StandardStreamLifecycle::ConstructWide(
    IStandardStreamRuntime& runtime) const {
    return runtime.ConstructWide(kWideObjectAddress);
}

int StandardStreamLifecycle::RegisterWideExit(
    IStandardStreamRuntime& runtime) const {
    return runtime.RegisterExit(kWideExitAddress);
}

int StandardStreamLifecycle::InitializeWide(
    IStandardStreamRuntime& runtime) const {
    (void)ConstructWide(runtime);
    return RegisterWideExit(runtime);
}

void StandardStreamLifecycle::CleanupWide(IStandardStreamRuntime& runtime) {
    if (--wide_count_ != 0u) return;
    runtime.CleanupWide(0x950940u);
    runtime.CleanupWide(0x9507d8u);
    runtime.CleanupWide(0x950818u);
}

}  // namespace pal4
