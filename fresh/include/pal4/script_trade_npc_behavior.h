#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptTradeSlotRuntime {
public:
    virtual ~IScriptTradeSlotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class ScriptTradeSlots final {
public:
    // IDA 0x5d3ed0 and 0x5d3ee0.
    [[nodiscard]] static std::uint32_t Get(
        std::int32_t index, IScriptTradeSlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t Set(
        std::int32_t index, std::uint32_t value,
        IScriptTradeSlotRuntime& runtime);
};

class IScriptNpcBehaviorRuntime {
public:
    virtual ~IScriptNpcBehaviorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindNpc(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void CleanupEventsAndScripts(std::uint32_t npc,
                                         std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t SetActionCode(
        std::uint32_t component, std::string_view code) = 0;
    [[nodiscard]] virtual std::uint32_t FindNpcInfo(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void ProcessEventsAndClear(std::uint32_t npc,
                                       std::uint32_t info,
                                       std::uint32_t flag) = 0;
};

class ScriptNpcBehavior final {
public:
    // IDA 0x5d3f00 and 0x5d3f60.
    [[nodiscard]] static std::uint32_t Pause(
        const char* name, IScriptNpcBehaviorRuntime& runtime);
    static void Resume(const char* name, IScriptNpcBehaviorRuntime& runtime);
};

}  // namespace pal4
