#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameSectionRuntime {
public:
    virtual ~IGameSectionRuntime() = default;
    virtual void InitializeScripts();
    virtual std::string ArenaScriptPath();
    virtual std::uint32_t ArenaScriptContext();
    virtual std::uint32_t CsbMode();
    virtual bool CacheContains(std::string_view key);
    virtual std::uint32_t CreateInterpreter(
        std::uint32_t mode, std::string_view script_path,
        std::string_view section, std::string_view script_name,
        std::uint32_t context, std::uint32_t read_callback);
    virtual bool HandleExecutionException(std::uint32_t interpreter);
    virtual std::string FunctionName(std::uint32_t interpreter);
    virtual std::string ExceptionWhat(std::uint32_t interpreter);
    virtual void ReportScriptException(std::string_view what,
                                       std::string_view function_name);
    virtual void ReleaseException(std::uint32_t interpreter);
    virtual void ShowMissingScript(std::string_view message,
                                   std::string_view title);
    virtual void CleanupScripts();
};

class GameSectionExecutor final {
public:
    static constexpr std::uint32_t kCsbReadCallback = 0x600d50u;

    // IDA 0x425ab0. The original always returns zero.
    [[nodiscard]] static std::uint32_t Execute(
        std::string_view section, IGameSectionRuntime& runtime);
};

}  // namespace pal4
