#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct GameWorldSceneScriptRequest {
    bool csb_mode = false;
    std::string script_path;
    std::string function_path;
    std::string function_name;
    std::string file_path;
    std::uint32_t read_callback = 0u;
};

enum class GameWorldScriptTemporary : std::uint8_t {
    FunctionPath,
    ScriptPath,
    FunctionName,
};

class IGameWorldSceneScriptNamesRuntime {
public:
    virtual ~IGameWorldSceneScriptNamesRuntime() = default;
    [[nodiscard]] virtual std::string_view ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteStoredText(std::uint32_t address,
                                 std::string_view text) = 0;
    [[nodiscard]] virtual std::string_view InitialFilePrefix() = 0;
    [[nodiscard]] virtual bool IsCsbMode() = 0;
};

class IGameWorldSceneScriptRuntime : public IGameWorldSceneScriptNamesRuntime {
public:
    virtual void InitializeScriptSystem() = 0;
    [[nodiscard]] virtual std::uint32_t GetExecutor() = 0;
    [[nodiscard]] virtual bool HasStackModule(
        std::uint32_t executor, std::string_view function_path) = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        const GameWorldSceneScriptRequest& request) = 0;
    [[nodiscard]] virtual bool AddScriptModule(std::uint32_t executor,
                                       std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string_view InterpreterFunctionName(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string_view InterpreterFullPath(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::int32_t PushStackModule(
        std::uint32_t executor, std::string_view full_path,
        std::string_view function_name) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    virtual void ShowMissing(std::string_view message,
                             std::string_view title) = 0;
    virtual void CleanupScripts() = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseTemporary(
        GameWorldScriptTemporary text, std::string_view value) = 0;
};

class GameWorldSceneScriptRuntimeStub final
    : public IGameWorldSceneScriptRuntime {
public:
    [[nodiscard]] std::string_view ReadText(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteStoredText(std::uint32_t, std::string_view) override {}
    [[nodiscard]] std::string_view InitialFilePrefix() override { return {}; }
    [[nodiscard]] bool IsCsbMode() override { return false; }
    void InitializeScriptSystem() override {}
    [[nodiscard]] std::uint32_t GetExecutor() override { return 0u; }
    [[nodiscard]] bool HasStackModule(std::uint32_t,
                                     std::string_view) override {
        return true;
    }
    [[nodiscard]] std::uint32_t CreateInterpreter(
        const GameWorldSceneScriptRequest&) override { return 0u; }
    [[nodiscard]] bool AddScriptModule(std::uint32_t, std::uint32_t) override {
        return false;
    }
    [[nodiscard]] std::string_view InterpreterFunctionName(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::string_view InterpreterFullPath(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::int32_t PushStackModule(
        std::uint32_t, std::string_view,
        std::string_view) override { return false; }
    void ReleaseInterpreter(std::uint32_t) override {}
    void ShowMissing(std::string_view, std::string_view) override {}
    void CleanupScripts() override {}
    [[nodiscard]] std::uint32_t ReleaseTemporary(
        GameWorldScriptTemporary, std::string_view) override { return 0u; }
};

class GameWorldSceneScriptLoader final {
public:
    static constexpr std::uint32_t kReadCallback = 0x600d50u;

    // Record owned arena names and reset the queued flag before SDK preparation.
    [[nodiscard]] static GameWorldSceneScriptRequest PrepareNames(
        std::uint32_t world, IGameWorldSceneScriptNamesRuntime& runtime);

    // IDA 0x51a820. The original scriptName argument is unused.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t world, std::uint32_t script_name_argument,
        IGameWorldSceneScriptRuntime& runtime);
};

}  // namespace pal4
