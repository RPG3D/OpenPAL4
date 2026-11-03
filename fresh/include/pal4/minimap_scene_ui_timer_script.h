#pragma once

#include "pal4/minimap_scene_ui.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IMinimapSceneUiTimerScriptRuntime {
public:
    virtual ~IMinimapSceneUiTimerScriptRuntime() = default;
    virtual void SetWindowText(std::uint32_t window,
                               std::string_view text) = 0;
    [[nodiscard]] virtual std::string_view EmptyErrorText() = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void RemoveTimer(std::uint32_t queue,
                             std::uint32_t identifier) = 0;
    virtual void InitializeScriptSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::string CopyContext(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEngine() = 0;
    [[nodiscard]] virtual bool HasStackModule(
        std::uint32_t context, std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyLogMessage() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptMode() = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        std::uint32_t mode, std::string_view script_path,
        std::string_view full_path, std::uint32_t script_name,
        std::string_view context, std::uint32_t reader) = 0;
    [[nodiscard]] virtual bool AddScriptModule(
        std::uint32_t context, std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string_view FunctionName(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string_view FullPath(
        std::uint32_t interpreter) = 0;
    virtual void PushStackModule(
        std::uint32_t context, std::string_view full_path,
        std::string_view function_name) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    virtual void CleanupAllScripts() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupCountdown(
        std::string_view text) = 0;
};

class MinimapSceneUiTimerScript final {
public:
    static constexpr std::uint32_t kReaderFunction = 0x600d50u;
    static constexpr std::string_view kScriptPath = "PFNTimerScript";
    static constexpr std::string_view kScriptSuffix = "[TimerScript]";

    [[nodiscard]] static std::uint32_t Execute(
        MinimapSceneUiState& state, std::uint32_t owner,
        IMinimapSceneUiTimerScriptRuntime& runtime);
};

}  // namespace pal4
