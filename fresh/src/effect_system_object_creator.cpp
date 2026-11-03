#include "pal4/effect_system_object_creator.h"

namespace pal4 {

std::uint32_t EffectSystemObjectCreator::CreateFromDocument(
    std::uint32_t document, std::uint32_t owner,
    std::uint32_t context,
    IEffectSystemObjectCreateRuntime& runtime) {
    if (document == 0u)
        return 0u;
    const auto class_name = runtime.StringAttribute(
        document, kClassAttribute);
    if (!class_name)
        return 0u;
    const auto factory = runtime.FindFactory(*class_name);
    if (factory == 0u)
        return 0u;
    const auto object = runtime.InvokeFactory(factory);
    if (object == 0u)
        return 0u;
    runtime.WriteWord(object + 12u, owner);
    runtime.WriteWord(object + 16u, context);
    if (!runtime.InitializeObject(object, document)) {
        runtime.DestroyObject(object);
        return 0u;
    }
    return object;
}

}  // namespace pal4
