#include "pal4/collection_ui_object_factory.h"

namespace pal4 {

std::uint32_t CollectionUiObjectFactory::CreateObject(
    std::uint32_t name, ICollectionUiObjectRuntime& runtime) {
    const auto storage = runtime.Allocate(0x5cu);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t CollectionUiObjectFactory::RegisterCreateObject(
    ICollectionUiObjectRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto* name = runtime.CastWindowName();
    return runtime.ProcessFrameEvent(manager, name, &CreateObject);
}

std::uint32_t CollectionUiObjectFactory::RegisterCreateObjectThunk(
    ICollectionUiObjectRuntime& runtime) {
    return RegisterCreateObject(runtime);
}

}  // namespace pal4
