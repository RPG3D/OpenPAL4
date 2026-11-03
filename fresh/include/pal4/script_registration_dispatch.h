#pragma once

#include "pal4/script_registration_catalog.h"

#include <array>
#include <cstdint>

namespace pal4 {

using ScriptFunctionPointerWords = std::array<std::uint32_t, 8>;

class IScriptRegistrationDispatchRuntime : public IScriptRegistrationCatalogRuntime {
public:
    virtual void RegisterFunction(std::uint32_t name,
                                  const ScriptFunctionPointerWords& callback,
                                  std::uint32_t calling_convention) = 0;
    virtual void RegisterObjectType(std::uint32_t name, std::uint32_t size,
                                    std::uint32_t flags) = 0;
    virtual void RegisterObjectBehavior(
        std::uint32_t type_name, std::uint32_t behavior,
        const char* declaration, const ScriptFunctionPointerWords& callback,
        std::uint32_t calling_convention) = 0;
    virtual void RegisterObjectMethod(
        std::uint32_t type_name, std::uint32_t method_name,
        const ScriptFunctionPointerWords& callback,
        std::uint32_t calling_convention) = 0;
    virtual void RegisterObjectProperty(std::uint32_t type_name,
                                        std::uint32_t property_name,
                                        std::uint32_t offset) = 0;
    virtual void RegisterGlobalProperty(std::uint32_t name,
                                        std::uint32_t address) = 0;
    virtual void RecordRegistration(std::uint32_t name,
                                    std::uint32_t description) = 0;
};

class ScriptRegistrationDispatcher final {
public:
    explicit ScriptRegistrationDispatcher(ScriptRegistrationCatalog& catalog)
        : catalog_(catalog) {}

    [[nodiscard]] int RegisterAll(
        const std::array<std::uint8_t, 3>& native_seed_bytes,
        IScriptRegistrationDispatchRuntime& runtime);

private:
    [[nodiscard]] static ScriptFunctionPointerWords ReadCallback(
        std::uint32_t address, IScriptRegistrationDispatchRuntime& runtime);
    [[nodiscard]] std::uint32_t Count(
        ScriptRegistrationKind kind, std::uint8_t seed,
        IScriptRegistrationDispatchRuntime& runtime);
    [[nodiscard]] std::uint32_t Descriptor(
        ScriptRegistrationKind kind, std::uint32_t index, std::uint8_t seed,
        IScriptRegistrationDispatchRuntime& runtime);
    static void Record(std::uint32_t descriptor,
                       IScriptRegistrationDispatchRuntime& runtime);

    ScriptRegistrationCatalog& catalog_;
};

}  // namespace pal4
