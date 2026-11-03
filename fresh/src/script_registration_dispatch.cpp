#include "pal4/script_registration_dispatch.h"

#include <stdexcept>

namespace pal4 {

ScriptFunctionPointerWords ScriptRegistrationDispatcher::ReadCallback(
    std::uint32_t address, IScriptRegistrationDispatchRuntime& runtime) {
    ScriptFunctionPointerWords words{};
    for (std::uint32_t index = 0; index < words.size(); ++index) {
        words[index] = runtime.ReadPointer(address + index * 4u);
    }
    return words;
}

std::uint32_t ScriptRegistrationDispatcher::Count(
    ScriptRegistrationKind kind, std::uint8_t seed,
    IScriptRegistrationDispatchRuntime& runtime) {
    const auto& queue = catalog_.GetOrInit(kind, seed, runtime);
    return queue.begin == 0u ? 0u : (queue.end - queue.begin) / 4u;
}

std::uint32_t ScriptRegistrationDispatcher::Descriptor(
    ScriptRegistrationKind kind, std::uint32_t index, std::uint8_t seed,
    IScriptRegistrationDispatchRuntime& runtime) {
    if (index >= Count(kind, seed, runtime)) {
        throw std::out_of_range("invalid vector<T> subscript");
    }
    const auto& queue = catalog_.GetOrInit(kind, seed, runtime);
    return runtime.ReadPointer(queue.begin + index * 4u);
}

void ScriptRegistrationDispatcher::Record(
    std::uint32_t descriptor, IScriptRegistrationDispatchRuntime& runtime) {
    runtime.RecordRegistration(runtime.ReadPointer(descriptor + 8u),
                               runtime.ReadPointer(descriptor));
}

int ScriptRegistrationDispatcher::RegisterAll(
    const std::array<std::uint8_t, 3>& native_seed_bytes,
    IScriptRegistrationDispatchRuntime& runtime) {
    const auto function_seed = native_seed_bytes[0];
    const auto variable_seed = native_seed_bytes[1];
    const auto type_seed = native_seed_bytes[2];

    for (std::uint32_t index = 0;
         index < Count(ScriptRegistrationKind::GlobalFunctions, function_seed,
                       runtime);
         ++index) {
        const auto descriptor = Descriptor(
            ScriptRegistrationKind::GlobalFunctions, index, function_seed,
            runtime);
        runtime.RegisterFunction(runtime.ReadPointer(descriptor + 8u),
                                 ReadCallback(descriptor + 16u, runtime), 0u);
        Record(Descriptor(ScriptRegistrationKind::GlobalFunctions, index,
                          function_seed, runtime), runtime);
    }

    for (std::uint32_t index = 0;
         index < Count(ScriptRegistrationKind::ObjectTypes, type_seed, runtime);
         ++index) {
        const auto descriptor = Descriptor(ScriptRegistrationKind::ObjectTypes,
                                           index, type_seed, runtime);
        const auto type_name = runtime.ReadPointer(descriptor + 8u);
        runtime.RegisterObjectType(type_name,
                                   runtime.ReadPointer(descriptor + 16u), 1u);
        runtime.RegisterObjectBehavior(
            type_name, 0u, "void f()",
            ReadCallback(descriptor + 24u, runtime), 3u);
        runtime.RegisterObjectBehavior(
            type_name, 1u, "void f()",
            ReadCallback(descriptor + 56u, runtime), 3u);
        Record(Descriptor(ScriptRegistrationKind::ObjectTypes, index,
                          type_seed, runtime), runtime);

        const auto method_begin = runtime.ReadPointer(descriptor + 92u);
        const auto method_end = runtime.ReadPointer(descriptor + 96u);
        for (auto slot = method_begin; slot != method_end; slot += 4u) {
            const auto method = runtime.ReadPointer(slot);
            runtime.RegisterObjectMethod(
                type_name, runtime.ReadPointer(method + 8u),
                ReadCallback(method + 16u, runtime), 2u);
            Record(method, runtime);
        }

        const auto property_begin = runtime.ReadPointer(descriptor + 108u);
        const auto property_end = runtime.ReadPointer(descriptor + 112u);
        for (auto slot = property_begin; slot != property_end; slot += 4u) {
            const auto property = runtime.ReadPointer(slot);
            runtime.RegisterObjectProperty(
                type_name, runtime.ReadPointer(property + 8u),
                runtime.ReadPointer(property + 52u));
            Record(property, runtime);
        }
    }

    for (std::uint32_t index = 0;
         index < Count(ScriptRegistrationKind::GlobalVariables, variable_seed,
                       runtime);
         ++index) {
        const auto descriptor = Descriptor(
            ScriptRegistrationKind::GlobalVariables, index, variable_seed,
            runtime);
        runtime.RegisterGlobalProperty(runtime.ReadPointer(descriptor + 8u),
                                       runtime.ReadPointer(descriptor + 48u));
        Record(Descriptor(ScriptRegistrationKind::GlobalVariables, index,
                          variable_seed, runtime), runtime);
    }

    catalog_.ClearAll(native_seed_bytes, runtime);
    return 1;
}

}  // namespace pal4
