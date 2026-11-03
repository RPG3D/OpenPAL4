#include "pal4/load_window_factory.h"

namespace pal4 {

std::uint32_t LoadWindowFactory::CreateObject(
    std::uint32_t name, ILoadWindowFactoryRuntime& runtime) {
    const auto storage = runtime.Allocate(0x64u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t LoadWindowFactory::RegisterCreateObject(
    ILoadWindowFactoryRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto name = runtime.RegisteredName();
    return runtime.RegisterFactory(manager, name, &CreateObject);
}

std::uint32_t LoadWindowFactory::RegisterCreateObjectThunk(
    ILoadWindowFactoryRuntime& runtime) {
    return RegisterCreateObject(runtime);
}

}  // namespace pal4
