#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IPalGameObjectFactoryScriptRuntime {
public:
    virtual ~IPalGameObjectFactoryScriptRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string ReadBytes(
        std::uint32_t address, std::uint32_t size) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    virtual void InitializeScript() = 0;
    [[nodiscard]] virtual bool IsCsbMode() = 0;
    [[nodiscard]] virtual bool ScriptCacheContains(
        std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t CreateScriptInterpreter(
        bool csb_mode, std::string_view path,
        std::string_view cache_key, std::string_view function,
        std::uint32_t context) = 0;
    [[nodiscard]] virtual bool AddScriptModule(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string ScriptFunctionName(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string ScriptFullPath(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::int32_t PushStackModule(
        std::string_view path,
        std::string_view function) = 0;
    virtual void ReleaseInterpreter(
        std::uint32_t interpreter) = 0;
    virtual void ShowMissingScriptMessage() = 0;
    virtual void CleanupAllScripts() = 0;
};

class PalGameObjectFactoryScript final {
public:
    // IDA 0x5a9cb0: LocalFloor callback, successful push sets movement root +88.
    [[nodiscard]] static std::uint32_t ExecuteLocalFloor(
        std::uint32_t handle, std::uint32_t argument,
        IPalGameObjectFactoryScriptRuntime& runtime);
    // IDA 0x5a9f10: Machine callback requires a positive script-name length.
    [[nodiscard]] static std::uint32_t ExecuteMachine(
        std::uint32_t handle, std::uint32_t argument,
        IPalGameObjectFactoryScriptRuntime& runtime);
};

}  // namespace pal4
