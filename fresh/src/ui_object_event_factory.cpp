#include "pal4/ui_object_event_factory.h"

namespace pal4 {

std::uint32_t UiObjectEventFactory::Create(
    std::uint32_t source_address, IUiObjectEventFactoryRuntime& runtime) {
    const auto result = runtime.Allocate(8u);
    if (result == 0u) return 0u;

    runtime.WriteWord(result, kBaseVtable);
    runtime.WriteWord(result + 4u,
                      runtime.ReadWord(source_address + 4u));
    runtime.WriteWord(result, kDerivedVtable);
    return result;
}

std::uint32_t UiObjectEventCallback::DeleteBase(
    std::uint32_t callback, std::uint32_t flags,
    IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback, kBaseVtable);
    if ((flags & 1u) != 0u) runtime.Free(callback);
    return callback;
}

bool UiObjectEventCallback::ForwardEvent(
    std::uint32_t callback, std::uint32_t event,
    IUiObjectEventFactoryRuntime& runtime) {
    return runtime.ForwardToFrameManager(callback + 4u, event);
}

std::uint32_t UiObjectEventCallback::Construct(
    std::uint32_t callback, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback + 8u, first);
    runtime.WriteWord(callback + 12u, second);
    runtime.WriteWord(callback + 16u, third);
    runtime.WriteWord(callback, kCallbackVtable);
    return callback;
}

std::uint32_t UiObjectEventCallback::ConstructIntroduction(
    std::uint32_t callback, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback + 8u, first);
    runtime.WriteWord(callback + 12u, second);
    runtime.WriteWord(callback + 16u, third);
    runtime.WriteWord(callback, kIntroductionVtable);
    return callback;
}

void UiObjectEventCallback::Destroy(
    std::uint32_t callback, IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback, kBaseVtable);
}

void UiObjectEventCallback::ConstructAlternateBase(
    std::uint32_t callback, IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback, kAlternateBaseVtable);
}

std::uint32_t UiObjectEventCallback::DeleteAlternateBase(
    std::uint32_t callback, std::uint32_t flags,
    IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback, kAlternateBaseVtable);
    if ((flags & 1u) != 0u) runtime.Free(callback);
    return callback;
}

std::uint32_t UiObjectEventCallback::ReleaseOwned(
    std::uint32_t owner, IUiObjectEventFactoryRuntime& runtime) {
    const auto callback = runtime.ReadWord(owner);
    if (callback == 0u) return 0u;
    return runtime.CallDeletingDestructor(callback, 1u);
}

std::uint32_t UiObjectEventCallback::CloneLoad(
    std::uint32_t source, IUiObjectEventFactoryRuntime& runtime) {
    const auto callback = runtime.Allocate(0x18u);
    if (callback == 0u) return 0u;
    runtime.WriteWord(callback, kLoadBaseVtable);
    runtime.WriteWord(callback + 8u, runtime.ReadWord(source + 8u));
    runtime.WriteWord(callback + 12u, runtime.ReadWord(source + 12u));
    runtime.WriteWord(callback + 16u, runtime.ReadWord(source + 16u));
    runtime.WriteWord(callback, kLoadCallbackVtable);
    return callback;
}

std::uint32_t UiObjectEventCallback::DeleteLoadBase(
    std::uint32_t callback, std::uint32_t flags,
    IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback, kLoadBaseVtable);
    if ((flags & 1u) != 0u) runtime.Free(callback);
    return callback;
}

void UiObjectEventCallback::DestroyLoad(
    std::uint32_t callback, IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback, kLoadBaseVtable);
}

std::uint32_t UiObjectEventCallback::CloneFrame(
    std::uint32_t source, IUiObjectEventFactoryRuntime& runtime) {
    const auto callback = runtime.Allocate(8u);
    if (callback == 0u) return 0u;
    runtime.WriteWord(callback, kBaseVtable);
    runtime.WriteWord(callback + 4u, runtime.ReadWord(source + 4u));
    runtime.WriteWord(callback, kFrameCallbackVtable);
    return callback;
}

std::uint32_t UiObjectEventCallback::CallStoredHandler(
    std::uint32_t callback, std::uint32_t event,
    IUiObjectEventFactoryRuntime& runtime) {
    return runtime.InvokeStoredHandler(runtime.ReadWord(callback + 4u), event);
}

std::uint32_t UiObjectEventCallback::DeleteFrame(
    std::uint32_t callback, std::uint32_t flags,
    IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback, kBaseVtable);
    if ((flags & 1u) != 0u) runtime.Free(callback);
    return callback;
}

std::uint32_t UiObjectEventCallback::ConstructMain(
    std::uint32_t callback, std::uint32_t handler,
    std::uint32_t unused, std::uint32_t owner,
    IUiObjectEventFactoryRuntime& runtime) {
    runtime.WriteWord(callback + 8u, handler);
    runtime.WriteWord(callback + 12u, unused);
    runtime.WriteWord(callback + 16u, owner);
    runtime.WriteWord(callback, kMainCallbackVtable);
    return callback;
}

}  // namespace pal4
