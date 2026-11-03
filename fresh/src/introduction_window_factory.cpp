#include "pal4/introduction_window_factory.h"

namespace pal4 {

std::uint32_t IntroductionWindowFactory::CreateObject(
    std::uint32_t name, IIntroductionWindowFactoryRuntime& runtime) {
    const auto storage = runtime.Allocate(0xb8u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t IntroductionWindowFactory::RegisterCreateObject(
    IIntroductionWindowFactoryRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto name = runtime.RegisteredName();
    return runtime.RegisterFactory(manager, name, &CreateObject);
}

std::uint32_t IntroductionWindowFactory::RegisterCreateObjectThunk(
    IIntroductionWindowFactoryRuntime& runtime) {
    return RegisterCreateObject(runtime);
}

}  // namespace pal4
