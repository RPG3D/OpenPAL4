#include "pal4/callback_object_lifecycle.h"

namespace pal4 {

int CallbackObjectLifecycle::Destroy(CallbackState::Storage& object,
                                     ICallbackObjectRuntime& runtime) {
    (void)CleanupCollection(object, runtime);
    const auto owned_address = CallbackState::ReadDword(object, 0x1c);
    if (owned_address != 0) {
        const auto result = runtime.Free(owned_address);
        CallbackState::WriteDword(object, 0x1c, 0);
        return result;
    }
    return 0;
}

int CallbackObjectLifecycle::DestroyThunk(CallbackState::Storage& object,
                                          ICallbackObjectRuntime& runtime) {
    return Destroy(object, runtime);
}

int CallbackObjectLifecycle::CleanupCollection(
    CallbackState::Storage& object, ICallbackObjectRuntime& runtime) {
    const auto root_address = CallbackState::ReadDword(object, 0x10);
    if (root_address == 0) return 0;

    BinaryTreeIterator iterator;
    (void)iterator.InitializeWithPreferredLeaf(runtime.ResolveNode(root_address));
    while (!iterator.IsAtEnd()) {
        auto* node = iterator.Current();
        (void)iterator.RemoveNextElementThunk(0);
        runtime.HandleStatusCheck(node);
        runtime.FreeNode(node);
    }
    CallbackState::WriteDword(object, 0x10, 0);
    CallbackState::WriteDword(object, 0x14, 0);
    return 1;
}

}  // namespace pal4
