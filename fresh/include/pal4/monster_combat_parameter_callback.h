#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IMonsterCombatParameterCallbackRuntime {
public:
    virtual ~IMonsterCombatParameterCallbackRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

struct MonsterCombatVector3 {
    float x{};
    float y{};
    float z{};
};

class IMonsterCombatParameterProcessRuntime
    : public IMonsterCombatParameterCallbackRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t InvokeGameStateSlot20(
        std::uint32_t state) = 0;
    virtual void ClearGameState(std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCombatDocument(
        std::string_view root) = 0;
    virtual void DestroyCombatDocument(std::uint32_t document) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::string FloorSoundPath(
        std::string_view base) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual MonsterCombatVector3 NormalizeDirection(
        MonsterCombatVector3 direction) = 0;
    [[nodiscard]] virtual double AngleFromDot(float dot) = 0;
    virtual void SetCombatWorld(std::uint32_t document,
                                std::string_view property,
                                std::string_view path,
                                std::uint32_t world_mode,
                                std::uint32_t count) = 0;
    [[nodiscard]] virtual std::string ReadBehaviorScriptName(
        std::uint32_t node, std::string_view attribute) = 0;
    virtual void InitializeScript() = 0;
    [[nodiscard]] virtual bool IsCsbMode() = 0;
    [[nodiscard]] virtual bool ScriptCacheContains(
        std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t CreateScriptInterpreter(
        bool csb_mode, std::string_view path,
        std::string_view cache_key, std::string_view function,
        std::uint32_t context) = 0;
    [[nodiscard]] virtual bool LoadScriptModule(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string ScriptFunctionName(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::string ScriptModulePath(
        std::uint32_t interpreter) = 0;
    virtual void PushScriptModule(std::string_view module_path,
                                       std::string_view function) = 0;
    virtual void ReleaseInterpreter(
        std::uint32_t interpreter) = 0;
    virtual void ShowMissingScriptMessage() = 0;
    virtual void CleanupAllScripts() = 0;
    virtual void AddActiveCharacter(std::uint32_t arena,
                                    std::uint32_t monster,
                                    std::uint32_t flag) = 0;
    virtual void SetClumpRenderCallback(std::uint32_t adapter_root,
                                        std::uint32_t clump) = 0;
    virtual void DispatchCombatPositions(std::uint32_t monster,
                                         std::uint32_t parameter_bits) = 0;
    virtual void InitializeCombatSystem(std::uint32_t monster,
                                        std::uint32_t mode) = 0;
};

// Embedded in MonsterBehaviorManager at +0x6d4.
class MonsterCombatParameterCallback final {
public:
    // IDA 0x5a9510.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object,
        IMonsterCombatParameterCallbackRuntime& runtime);
    // IDA 0x5a9530: monster-triggered combat setup and script dispatch.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t callback, std::uint32_t argument,
        IMonsterCombatParameterProcessRuntime& runtime);
};

}  // namespace pal4
