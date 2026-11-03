#include "pal4/collection_ui_registration.h"

namespace pal4 {

std::uint32_t CollectionUiRegistration::RegisterShowCastUi(
    ICollectionUiRegistrationRuntime& runtime) {
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.InitializeCallback(callback, kCallbackVtable, "CastWindow");
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_cast_ui", callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kResultAddress;
}

std::uint32_t CollectionUiRegistration::RegisterShowCastUiThunk(
    ICollectionUiRegistrationRuntime& runtime) {
    return RegisterShowCastUi(runtime);
}

}  // namespace pal4
