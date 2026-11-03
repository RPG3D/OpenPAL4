#include "pal4/cegui_lifecycle.h"

namespace pal4 {

std::uint32_t CeguiLifecycleStub::ReadDword(
    std::uint32_t /*address*/) { return 0u; }
void CeguiLifecycleStub::WriteDword(
    std::uint32_t /*address*/, std::uint32_t /*value*/) {}
void CeguiLifecycleStub::CleanupNodesRecursive(
    std::uint32_t /*map_base*/, std::uint32_t /*root*/) {}
std::uint32_t CeguiLifecycleStub::AdvanceCursor(
    std::uint32_t cursor) { return cursor; }
void CeguiLifecycleStub::DestroyNode(
    std::uint32_t /*map_base*/, std::uint32_t /*node*/) {}
std::uint32_t CeguiLifecycleStub::Free(
    std::uint32_t /*address*/) { return 0u; }
std::uint32_t CeguiLifecycleStub::GetSystemSingletonPtr() { return 0u; }
std::uint32_t CeguiLifecycleStub::GetImageManager() { return 0u; }
void CeguiLifecycleStub::DestroyAllImages(std::uint32_t /*manager*/) {}
void CeguiLifecycleStub::InvokeDeletingDestructor(
    std::uint32_t /*object*/, std::uint32_t /*flags*/) {}
void CeguiLifecycleStub::DestructSystem(std::uint32_t /*system*/) {}
std::uint32_t CeguiLifecycleStub::GetSystemSingleton() { return 0u; }
std::uint32_t CeguiLifecycleStub::InjectTimePulse(
    std::uint32_t /*system*/, float /*delta*/) { return 0u; }
void CeguiLifecycleStub::RenderGui(std::uint32_t /*system*/) {}

std::uint32_t CeguiMapTreeLifecycle::Destroy(
    const std::uint32_t map_base, CeguiTreeGlobals& globals,
    ICeguiLifecycleRuntime& runtime) {
    const auto header = runtime.ReadDword(map_base + 4u);
    const auto count = runtime.ReadDword(map_base + 0x0cu);
    auto cursor = runtime.ReadDword(header);
    if (count != 0u) {
        runtime.CleanupNodesRecursive(map_base,
                                      runtime.ReadDword(header + 4u));
        runtime.WriteDword(header + 4u, globals.nil);
        runtime.WriteDword(map_base + 0x0cu, 0u);
        runtime.WriteDword(header, header);
        runtime.WriteDword(header + 8u, header);
    } else {
        while (cursor != header) {
            const auto node = cursor;
            cursor = runtime.AdvanceCursor(cursor);
            runtime.DestroyNode(map_base, node);
        }
    }
    static_cast<void>(runtime.Free(header));
    runtime.WriteDword(map_base + 4u, 0u);
    runtime.WriteDword(map_base + 0x0cu, 0u);
    --globals.reference_count;
    if (globals.reference_count != 0u) {
        return globals.reference_count;
    }
    const auto nil = globals.nil;
    globals.nil = 0u;
    if (nil != 0u) return runtime.Free(nil);
    return 0u;
}

std::uint32_t CeguiSystemLifecycle::DestroySystem(
    const std::uint32_t system, const std::uint32_t flags,
    ICeguiLifecycleRuntime& runtime) {
    runtime.DestructSystem(system);
    if ((flags & 1u) != 0u) {
        static_cast<void>(runtime.Free(system));
    }
    return system;
}

std::uint32_t CeguiSystemLifecycle::DestroyUi(
    const std::uint32_t owner, CeguiTreeGlobals& globals,
    ICeguiLifecycleRuntime& runtime) {
    const auto system = runtime.GetSystemSingletonPtr();
    const auto provider = runtime.ReadDword(system + 0x1cu);
    runtime.DestroyAllImages(runtime.GetImageManager());
    runtime.InvokeDeletingDestructor(system, 1u);
    if (provider != 0u) {
        runtime.InvokeDeletingDestructor(provider, 1u);
    }
    return CeguiMapTreeLifecycle::Destroy(owner + 4u, globals, runtime);
}

std::uint32_t CeguiSystemLifecycle::TimePulse(
    const float delta, ICeguiLifecycleRuntime& runtime) {
    return runtime.InjectTimePulse(runtime.GetSystemSingleton(), delta);
}

void CeguiSystemLifecycle::Render(ICeguiLifecycleRuntime& runtime) {
    runtime.RenderGui(runtime.GetSystemSingleton());
}

}  // namespace pal4
