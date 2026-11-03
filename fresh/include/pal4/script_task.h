#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IScriptTaskRuntime {
public:
    virtual ~IScriptTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestruct(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint8_t TransientInitialByte() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t source) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
    [[nodiscard]] virtual std::uint32_t SetErrorFlag(
        std::uint32_t record, std::uint8_t flag) = 0;
    virtual void ReallocateText(std::uint32_t record,
                                std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual bool ActorControlActive() = 0;
    [[nodiscard]] virtual bool ScriptSystemBusy() = 0;
    virtual void InitializeScript() = 0;
    [[nodiscard]] virtual bool ScriptCacheContains(
        std::string_view full_name) = 0;
    [[nodiscard]] virtual bool IsCsbMode() = 0;
    [[nodiscard]] virtual std::uint32_t ArenaContext() = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        bool csb_mode, std::string_view path,
        std::string_view full_name, std::string_view function_name,
        std::uint32_t arena_context) = 0;
    [[nodiscard]] virtual bool AddScriptModule(
        std::uint32_t interpreter) = 0;
    virtual void PushStackModule(
        std::uint32_t interpreter) = 0;
    virtual void ReleaseInterpreter(
        std::uint32_t interpreter) = 0;
    virtual void ShowMissingScriptError() = 0;
    virtual void CleanupScriptFiles() = 0;
    [[nodiscard]] virtual std::uint32_t FindCachedScript(
        std::string_view path) = 0;
};

class IScriptTaskSpawnRuntime : public virtual IScriptTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructHelperTask(
        std::uint32_t memory) = 0;
    virtual void AddHelperTask(std::uint32_t context,
                               std::uint32_t helper,
                               std::uint32_t first_timing,
                               std::uint32_t second_timing,
                               std::uint32_t third_timing) = 0;
    virtual void SetHelperOwner(std::uint32_t helper,
                                std::uint32_t owner) = 0;
    virtual void NotifyPreviousTask(std::uint32_t previous) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class ScriptTask final {
public:
    static constexpr std::uint32_t kVtable = 0x848940u;
    static constexpr std::string_view kScriptPath = "palGameOMTask_script";
    // IDA 0x598e30..0x599350.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IScriptTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IScriptTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IScriptTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, IScriptTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, IScriptTaskSpawnRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t function_name, std::uint32_t wait,
        std::uint32_t context, IScriptTaskRuntime& runtime);
};

}  // namespace pal4
