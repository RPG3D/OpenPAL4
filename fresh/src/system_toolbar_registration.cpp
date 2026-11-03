#include "pal4/system_toolbar_registration.h"

namespace pal4 {
namespace {

struct HandlerDescriptor {
    std::uint32_t storage;
    std::uint32_t key;
    std::uint32_t callback;
    std::uint32_t vtable;
    bool callback_is_name;
};

constexpr HandlerDescriptor kMain{
    0x8e797bu, 0x8a80acu, 0x8b07d8u, 0x843494u, true};
constexpr HandlerDescriptor kButton1{
    0x8e797au, 0x8a80b0u, 0x482f50u, 0x8435dcu, false};
constexpr HandlerDescriptor kButton2{
    0x8e7979u, 0x8a80b4u, 0x482fb0u, 0x8435dcu, false};
constexpr HandlerDescriptor kButton3{
    0x8e7978u, 0x8a80b8u, 0x482fe0u, 0x8435dcu, false};

std::uint32_t Register(const HandlerDescriptor& descriptor,
                       ISystemToolbarRegistrationRuntime& runtime) {
    const auto callback = descriptor.callback_is_name ?
        runtime.ReadWord(descriptor.callback) : descriptor.callback;
    const auto handler = runtime.Allocate(8u);
    if (handler != 0u) {
        runtime.WriteWord(handler + 4u, callback);
        runtime.WriteWord(handler, descriptor.vtable);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                descriptor.key, handler);
    } catch (...) {
        if (handler != 0u) {
            runtime.ReleaseHandler(handler, 1u);
        }
        throw;
    }
    if (handler != 0u) {
        runtime.ReleaseHandler(handler, 1u);
    }
    return descriptor.storage;
}

}  // namespace

std::uint32_t SystemToolbarRegistration::GetMainHandler(
    ISystemToolbarRegistrationRuntime& runtime) {
    return RegisterMainHandler(runtime);
}

std::uint32_t SystemToolbarRegistration::RegisterMainHandler(
    ISystemToolbarRegistrationRuntime& runtime) {
    return Register(kMain, runtime);
}

std::uint32_t SystemToolbarRegistration::GetButton1Handler(
    ISystemToolbarRegistrationRuntime& runtime) {
    return RegisterButton1Handler(runtime);
}

std::uint32_t SystemToolbarRegistration::RegisterButton1Handler(
    ISystemToolbarRegistrationRuntime& runtime) {
    return Register(kButton1, runtime);
}

std::uint32_t SystemToolbarRegistration::GetButton2Handler(
    ISystemToolbarRegistrationRuntime& runtime) {
    return RegisterButton2Handler(runtime);
}

std::uint32_t SystemToolbarRegistration::RegisterButton2Handler(
    ISystemToolbarRegistrationRuntime& runtime) {
    return Register(kButton2, runtime);
}

std::uint32_t SystemToolbarRegistration::GetButton3Handler(
    ISystemToolbarRegistrationRuntime& runtime) {
    return RegisterButton3Handler(runtime);
}

std::uint32_t SystemToolbarRegistration::RegisterButton3Handler(
    ISystemToolbarRegistrationRuntime& runtime) {
    return Register(kButton3, runtime);
}

}  // namespace pal4
