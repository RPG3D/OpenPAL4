#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalGameIVStartupPreparationRuntime {
public:
    virtual ~IPalGameIVStartupPreparationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void InitializeInventory(std::uint32_t inventory) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    virtual void ProcessLoadedPlayers(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptInstance() = 0;
    virtual void ClearScriptCache(std::uint32_t cache) = 0;
    [[nodiscard]] virtual std::uint32_t Camera() = 0;
    virtual void ConfigureCamera(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaCache() = 0;
    virtual void ClearArenaCache(std::uint32_t cache) = 0;
    virtual void InitializeCoreSystems(std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    virtual void SaveDefaultDialogSettings(std::uint32_t data) = 0;
    [[nodiscard]] virtual std::uint32_t PetManager() = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class IPalGameIVStartupRuntime : public IPalGameIVStartupPreparationRuntime {
public:
    [[nodiscard]] virtual bool IsCsbMode() = 0;
    virtual void InitializeScriptEngine() = 0;
    [[nodiscard]] virtual bool ScriptCacheContains(
        std::uint32_t cache, std::string_view full_name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        bool csb_mode, std::string_view script_name,
        std::string_view full_name, std::string_view function_name,
        std::string_view source_path, std::uint32_t read_callback) = 0;
    [[nodiscard]] virtual bool AddScriptModule(
        std::uint32_t cache, std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string_view InterpreterName(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string_view InterpreterFullPath(
        std::uint32_t interpreter) = 0;
    virtual void PushStackModule(std::uint32_t cache,
                                 std::string_view full_path,
                                 std::string_view name) = 0;
    virtual void ReleaseInterpreter(
        std::uint32_t interpreter) = 0;
    virtual void ShowMissingScriptMessage(std::string_view message,
                                          std::string_view title) = 0;
    virtual void CleanupAllScripts() = 0;
};

class PalGameIVStartup final {
public:
    static constexpr std::uint32_t kScriptCacheOffset = 0x54u;
    static constexpr std::uint32_t kPetStateOffset = 0x654u;
    static constexpr std::uint32_t kReadToBufferCallback = 0x600d50u;

    // Shared prelude also used with the native SDK script scheduler.
    static void Prepare(std::uint32_t game, IPalGameIVStartupPreparationRuntime& runtime);

    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t game, IPalGameIVStartupRuntime& runtime);
};

}  // namespace pal4
