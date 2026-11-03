#include "pal4/combat_fatal_child_creation.h"

#include <initializer_list>

namespace pal4 {
namespace {

struct ConstructionGuard {
    IFatalChildCreationRuntime& runtime;
    std::uint32_t object;
    bool base = false;
    bool name = false;

    ~ConstructionGuard() {
        if (object == 0u) return;
        if (name) runtime.ResetNameNode(object + 80u);
        if (base) runtime.DestroyBase(object);
        runtime.Free(object);
    }
};

std::uint32_t CreateSimple(
    IFatalChildCreationRuntime& runtime,
    std::uint32_t size, std::uint32_t vtable,
    std::initializer_list<std::uint32_t> before_vtable,
    std::initializer_list<std::uint32_t> after_vtable) {
    const auto object = runtime.Allocate(size);
    if (object == 0u) return 0u;
    ConstructionGuard guard{runtime, object};
    runtime.ConstructBase(object);
    guard.base = true;
    for (const auto offset : before_vtable)
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteWord(object, vtable);
    for (const auto offset : after_vtable)
        runtime.WriteWord(object + offset, 0u);
    guard.object = 0u;
    return object;
}

}  // namespace

std::uint32_t FatalEventChildTask::Create(
    IFatalChildCreationRuntime& runtime) {
    return CreateSimple(runtime, kSize, kVtable,
                        {}, {64u, 60u});
}

std::uint32_t FatalFirstFinalChildTask::Create(
    IFatalChildCreationRuntime& runtime) {
    return CreateSimple(runtime, kSize, kVtable,
                        {60u}, {});
}

std::uint32_t FatalFinishChildTask::Create(
    IFatalChildCreationRuntime& runtime) {
    return CreateSimple(runtime, kSize, kVtable,
                        {60u}, {});
}

std::uint32_t FatalEndChildTask::Create(
    IFatalChildCreationRuntime& runtime) {
    return CreateSimple(runtime, kSize, kVtable,
                        {}, {});
}

std::uint32_t FatalMessageChildTask::Create(
    IFatalChildCreationRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    ConstructionGuard guard{runtime, object};
    runtime.ConstructBase(object);
    guard.base = true;
    runtime.WriteWord(object + 64u, 0u);
    // The original copies an unwritten stack byte at +60.
    runtime.WriteByte(object + 60u, 0u);
    runtime.WriteWord(object + 68u, 0u);
    runtime.WriteWord(object + 72u, 0u);
    runtime.WriteWord(object + 76u, 0u);
    runtime.ConstructNameNode(object + 80u);
    guard.name = true;
    runtime.WriteByte(object + 88u, 0u);
    runtime.WriteWord(object + 92u, 0u);
    runtime.WriteWord(object + 96u, 0u);
    runtime.WriteWord(object, kVtable);
    guard.object = 0u;
    return object;
}

}  // namespace pal4
