#pragma once

#include <cstdint>

namespace pal4 {
class IScriptTaskQueueSequence;

class IScriptTaskQueueRuntime {
public:
    virtual ~IScriptTaskQueueRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EventIdentity(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual bool EventProcessed(
        std::uint32_t event) = 0;
    virtual void PrepareEvent(std::uint32_t event) = 0;
    virtual void SetProcessedFlag(std::uint32_t event_data) = 0;
    virtual void GetSkillSystem() = 0;
    virtual void RunNoOp() = 0;
    virtual void StartIdentityAction(std::uint32_t identity) = 0;
    virtual void UpdateContainer(std::uint32_t container,
                                 float delta) = 0;
    virtual void StopTask(std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t ResultEvent(
        std::uint32_t task) = 0;
    virtual void DeleteTask(std::uint32_t task,
                            std::uint32_t flags) = 0;
};

class ScriptTaskQueue final {
public:
    // Same game algorithm, with storage delegated to a host-owned sequence.
    [[nodiscard]] static std::uint32_t RegisterEvent(std::uint32_t event,IScriptTaskQueueSequence&,IScriptTaskQueueRuntime&);
    [[nodiscard]] static std::uint32_t Update(float delta,IScriptTaskQueueSequence&,IScriptTaskQueueRuntime&);
    [[nodiscard]] static std::uint32_t ResetFlags(IScriptTaskQueueSequence&,IScriptTaskQueueRuntime&);
    [[nodiscard]] static std::uint32_t Cleanup(IScriptTaskQueueSequence&);
    // IDA 0x5637c0/0x563920/0x5639e0/0x563a10.
    [[nodiscard]] static std::uint32_t RegisterEvent(
        std::uint32_t queue, std::uint32_t event,
        IScriptTaskQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t queue, float delta,
        IScriptTaskQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetFlags(
        std::uint32_t queue, IScriptTaskQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t queue, IScriptTaskQueueRuntime& runtime);
};

}  // namespace pal4
