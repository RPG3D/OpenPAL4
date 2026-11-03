#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobSnapshotRuntime;

class IGobObstacleEventRuntime {
public:
    virtual ~IGobObstacleEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    virtual void InitializeScript() = 0;
    [[nodiscard]] virtual std::string CurrentSceneScriptPath() = 0;
    [[nodiscard]] virtual bool ScriptCacheContains(std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t CreateScriptInterpreter(
        std::string_view path, std::string_view key,
        std::string_view entry) = 0;
    [[nodiscard]] virtual bool HandleScriptExecution(
        std::uint32_t interpreter) = 0;
    virtual void ReportScriptException(std::uint32_t interpreter) = 0;
    virtual void ReleaseScriptException(std::uint32_t interpreter) = 0;
    virtual void ShowScriptNotFound() = 0;
    virtual void CleanupScriptPackages() = 0;
    virtual void CleanupScriptPath() = 0;
    virtual void TryTriggerTaskRun(std::uint32_t object,
                                   std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ConsumeResearchUse(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ResetMachineTriggeredRuntime(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual IGobSnapshotRuntime& SnapshotRuntime() = 0;
};

}  // namespace pal4
