#include "pal4/input_method_static_registration.h"

namespace pal4 {

std::int32_t InputMethodStaticRegistration::Register(
    IInputMethodStaticRuntime& runtime) {
    Initialize(runtime);
    return runtime.RegisterAtExit(kCleanupCallback);
}

void InputMethodStaticRegistration::Initialize(
    IInputMethodStaticRuntime& runtime) {
    runtime.BeginInputMethod(kObject);
}

void InputMethodStaticRegistration::Cleanup(
    IInputMethodStaticRuntime& runtime) {
    runtime.ReleaseVirtualMemory(kObject);
}

}  // namespace pal4
