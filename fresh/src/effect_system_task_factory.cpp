#include "pal4/effect_system_task_factory.h"

namespace pal4 {

std::uint32_t EffectSystemTaskFactory::Create(
    EffectSystemTaskKind kind,
    IEffectSystemTaskFactoryRuntime& runtime,
    std::uint8_t native_stack_byte) {
    const auto object = runtime.Allocate(
        kSizes.at(static_cast<std::size_t>(kind)));
    if (object == 0u) return 0u;
    if (kind != EffectSystemTaskKind::Message &&
        kind != EffectSystemTaskKind::SlowMotion)
        return runtime.ConstructTask(kind, object);

    runtime.ConstructCommonTask(object);
    if (kind == EffectSystemTaskKind::SlowMotion) {
        runtime.WriteWord(object + 76u, 0u);
        runtime.WriteWord(object, 0x84b318u);
        return object;
    }
    runtime.WriteWord(object + 76u, 0u);
    runtime.WriteByte(object + 72u, native_stack_byte);
    runtime.WriteWord(object + 80u, 0u);
    runtime.WriteWord(object + 84u, 0u);
    runtime.WriteWord(object + 88u, 0u);
    runtime.WriteWord(object, 0x84b2d8u);
    return object;
}

}  // namespace pal4
