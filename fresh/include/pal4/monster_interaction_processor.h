#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

struct MonsterBehaviorManagerState;

struct MonsterInteractionTarget {
    std::array<float, 3> position{};
};

class IMonsterInteractionRuntime {
public:
    virtual ~IMonsterInteractionRuntime() = default;
    [[nodiscard]] virtual bool PlayerControlBusy() = 0;
    [[nodiscard]] virtual MonsterInteractionTarget Target(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual double PlanarDistance(float dx, float dz) = 0;
    [[nodiscard]] virtual float PlayerDistanceBase() = 0;
    [[nodiscard]] virtual std::array<float, 3> TargetForward(
        std::uint32_t target_address) = 0;
    [[nodiscard]] virtual std::array<float, 3> ManagerForward(
        std::uint32_t manager_address) = 0;
    virtual void NormalizeDirection(std::array<float, 3>& direction) = 0;
    [[nodiscard]] virtual double AngleForForwardDot(float dot) = 0;
    [[nodiscard]] virtual std::string ReadScriptName(
        std::uint32_t source_address,
        const std::array<std::uint32_t, 4>& key,
        const char* attribute_name) = 0;
    virtual void InitializeScripts() = 0;
    [[nodiscard]] virtual std::string ArenaScriptPath() = 0;
    [[nodiscard]] virtual bool ScriptCached(const std::string& full_path) = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        const std::string& path, const std::string& full_path,
        const std::string& script_name) = 0;
    [[nodiscard]] virtual bool ExecuteInterpreter(
        std::uint32_t interpreter) = 0;
    virtual void HandleInterpreterError(std::uint32_t interpreter) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    virtual void ShowMissingScriptError() = 0;
    virtual void CleanupScripts() = 0;
    [[nodiscard]] virtual std::int32_t PendingStateCount(
        std::uint32_t source_address) = 0;
    virtual void ReactivateManager(std::uint32_t manager_address,
                                   std::uint32_t flag) = 0;
    virtual void RestoreRenderCallback(std::uint32_t manager_address) = 0;
};

class MonsterInteractionRuntimeStub final : public IMonsterInteractionRuntime {
public:
    bool PlayerControlBusy() override;
    MonsterInteractionTarget Target(std::uint32_t address) override;
    double PlanarDistance(float dx, float dz) override;
    float PlayerDistanceBase() override;
    std::array<float, 3> TargetForward(std::uint32_t address) override;
    std::array<float, 3> ManagerForward(std::uint32_t address) override;
    void NormalizeDirection(std::array<float, 3>& direction) override;
    double AngleForForwardDot(float dot) override;
    std::string ReadScriptName(std::uint32_t source_address,
        const std::array<std::uint32_t, 4>& key,
        const char* attribute_name) override;
    void InitializeScripts() override;
    std::string ArenaScriptPath() override;
    bool ScriptCached(const std::string& full_path) override;
    std::uint32_t CreateInterpreter(const std::string& path,
        const std::string& full_path, const std::string& script_name) override;
    bool ExecuteInterpreter(std::uint32_t interpreter) override;
    void HandleInterpreterError(std::uint32_t interpreter) override;
    void ReleaseInterpreter(std::uint32_t interpreter) override;
    void ShowMissingScriptError() override;
    void CleanupScripts() override;
    std::int32_t PendingStateCount(std::uint32_t source_address) override;
    void ReactivateManager(std::uint32_t manager_address,
                           std::uint32_t flag) override;
    void RestoreRenderCallback(std::uint32_t manager_address) override;
};

class MonsterInteractionProcessor final {
public:
    // IDA 0x433ea0: pursuit encounter, facing classification and script trigger.
    [[nodiscard]] static bool Process(MonsterBehaviorManagerState& state,
                                      std::uint32_t manager_address,
                                      std::uint32_t target_address,
                                      IMonsterInteractionRuntime& runtime);
};

}  // namespace pal4
