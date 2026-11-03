#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

enum class ScriptRegistrationKind : std::uint8_t {
    GlobalFunctions,
    GlobalVariables,
    ObjectTypes,
};

struct ScriptRegistrationQueueState {
    std::uint8_t seed_byte{};
    std::uint8_t reserved[3]{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity_end{};
};
static_assert(sizeof(ScriptRegistrationQueueState) == 16);

class IScriptRegistrationCatalogRuntime {
public:
    virtual ~IScriptRegistrationCatalogRuntime() = default;
    virtual void RegisterAtExit(std::uint32_t callback) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual std::uint32_t ReadPointer(std::uint32_t address) = 0;
    virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WritePointer(std::uint32_t address, std::uint32_t value) = 0;
};

class ScriptRegistrationCatalog final {
public:
    static constexpr std::array<std::uint32_t, 3> kCleanupCallbacks{
        0x7e0120u, 0x7e0190u, 0x7e0200u};

    ScriptRegistrationQueueState& GetOrInit(
        ScriptRegistrationKind kind, std::uint8_t native_seed_byte,
        IScriptRegistrationCatalogRuntime& runtime);

    void Cleanup(ScriptRegistrationKind kind,
                 IScriptRegistrationCatalogRuntime& runtime);

    ScriptRegistrationQueueState& ClearAll(
        const std::array<std::uint8_t, 3>& native_seed_bytes,
        IScriptRegistrationCatalogRuntime& runtime);

    void RegisterGlobalFunction(std::uint32_t descriptor,
                                std::uint8_t native_seed_byte,
                                IScriptRegistrationCatalogRuntime& runtime);
    void RegisterGlobalVariable(std::uint32_t descriptor,
                                std::uint8_t native_seed_byte,
                                IScriptRegistrationCatalogRuntime& runtime);

    [[nodiscard]] std::uint8_t Guard(ScriptRegistrationKind kind) const;
    void SetGuard(ScriptRegistrationKind kind, std::uint8_t value);

private:
    static std::size_t Index(ScriptRegistrationKind kind);
    void RegisterDescriptor(ScriptRegistrationKind kind, std::uint32_t descriptor,
                            std::uint8_t native_seed_byte,
                            IScriptRegistrationCatalogRuntime& runtime);
    std::array<ScriptRegistrationQueueState, 3> queues_{};
    std::array<std::uint8_t, 3> guards_{};
};

}  // namespace pal4
