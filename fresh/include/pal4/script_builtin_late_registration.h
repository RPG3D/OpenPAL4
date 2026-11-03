#pragma once

#include "pal4/script_builtin_static_registration.h"

#include <cstdint>

namespace pal4 {

class IScriptBuiltinLateRuntime {
public:
    virtual ~IScriptBuiltinLateRuntime() = default;
    virtual void RegisterBuiltin(StaticBuiltin builtin,
                                 std::uint32_t descriptor,
                                 std::uint32_t native_entry) = 0;
    [[nodiscard]] virtual int RegisterAtExit(
        std::uint32_t callback) = 0;
    virtual void DeallocateDescriptor(std::uint32_t descriptor) = 0;
};

struct ScriptBuiltinLateSpec {
    std::uint32_t descriptor;
    std::uint32_t registration_entry;
    std::uint32_t cleanup_callback;
};

class ScriptBuiltinLateRegistration final {
public:
    // IDA 0x4a25d0-0x4a5fc0, in StaticBuiltin enum order.
    [[nodiscard]] static ScriptBuiltinLateSpec SpecFor(StaticBuiltin builtin);
    [[nodiscard]] static int Initialize(
        StaticBuiltin builtin, IScriptBuiltinLateRuntime& runtime);
    static void Register(StaticBuiltin builtin,
                         IScriptBuiltinLateRuntime& runtime);
    static void Cleanup(StaticBuiltin builtin,
                        IScriptBuiltinLateRuntime& runtime);
};

}  // namespace pal4
