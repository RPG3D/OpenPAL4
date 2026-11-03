#include "pal4/script_builtin_late_registration.h"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::array<ScriptBuiltinLateSpec,
                     static_cast<std::size_t>(StaticBuiltin::Count)> kSpecs{{
#include "pal4/detail/script_builtin_late_specs.inc"
}};

}  // namespace

ScriptBuiltinLateSpec ScriptBuiltinLateRegistration::SpecFor(
    const StaticBuiltin builtin) {
    const auto index = static_cast<std::size_t>(builtin);
    if (index >= kSpecs.size())
        throw std::out_of_range("late script builtin entry not reconstructed");
    return kSpecs[index];
}

int ScriptBuiltinLateRegistration::Initialize(
    const StaticBuiltin builtin, IScriptBuiltinLateRuntime& runtime) {
    Register(builtin, runtime);
    return runtime.RegisterAtExit(SpecFor(builtin).cleanup_callback);
}

void ScriptBuiltinLateRegistration::Register(
    const StaticBuiltin builtin, IScriptBuiltinLateRuntime& runtime) {
    const auto spec = SpecFor(builtin);
    runtime.RegisterBuiltin(builtin, spec.descriptor,
                            spec.registration_entry);
}

void ScriptBuiltinLateRegistration::Cleanup(
    const StaticBuiltin builtin, IScriptBuiltinLateRuntime& runtime) {
    runtime.DeallocateDescriptor(SpecFor(builtin).descriptor);
}

}  // namespace pal4
