#include "pal4/check_protect_children.h"

namespace pal4 {
namespace {

std::uint32_t CreateChild(const std::uint32_t size,
                          const std::uint32_t vtable,
                          const bool has_trailing_word,
                          ICheckProtectChildRuntime& runtime) {
    const auto object = runtime.Allocate(size);
    if (object == 0u) return 0u;
    struct Guard {
        ICheckProtectChildRuntime& runtime;
        std::uint32_t object;
        ~Guard() {
            if (object != 0u) runtime.Free(object);
        }
    } guard{runtime, object};
    runtime.ConstructBase(object);
    if (has_trailing_word) runtime.WriteWord(object + 72u, 0u);
    runtime.WriteWord(object, vtable);
    guard.object = 0u;
    return object;
}

}  // namespace

std::uint32_t CheckProtectPositionTask::Create(
    ICheckProtectChildRuntime& runtime) {
    return CreateChild(kSize, kVtable, true, runtime);
}

std::uint32_t CheckProtectResponseTask::Create(
    ICheckProtectChildRuntime& runtime) {
    return CreateChild(kSize, kVtable, false, runtime);
}

}  // namespace pal4
