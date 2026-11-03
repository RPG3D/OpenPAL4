#include "pal4/actor_global_tga.h"

namespace pal4 {

int ActorGlobalTga::Initialize(IActorGlobalTgaRuntime& runtime) {
    (void)InitializeHeader(runtime);
    return runtime.RegisterAtExit(&CleanupHeader);
}

std::uint32_t ActorGlobalTga::InitializeHeader(
    IActorGlobalTgaRuntime& runtime) {
    return runtime.InitializeHeader(kHeaderAddress);
}

void ActorGlobalTga::CleanupHeader() {
    // IDA 0x52af40 tail-jumps to nullsub_91, which is a single return.
}

}  // namespace pal4
