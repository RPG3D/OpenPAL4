#pragma once

#include <cstdint>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeSkillRecordCatalog;
class NativePlayerSkillCallbacks;
class INativeSkillSystemServices;

// Borrows memory, records, callbacks and actual SkillSystem services.
// Live Player scripts and the original isolated 8430E8 comparison snapshots.
// Unsupported effects throw at dispatch.
class NativePlayerSkillRuntime final {
public:
    NativePlayerSkillRuntime(NativeBaseObjectMemory& memory,
                            NativeSkillRecordCatalog& catalog,
                            NativePlayerSkillCallbacks& callbacks,
                            INativeSkillSystemServices& system);
    ~NativePlayerSkillRuntime();
    NativePlayerSkillRuntime(const NativePlayerSkillRuntime&) = delete;
    NativePlayerSkillRuntime& operator=(const NativePlayerSkillRuntime&) = delete;
    [[nodiscard]] bool Apply(std::uint32_t script, std::uint32_t id,
                             std::uint32_t context);
    void Process(std::uint32_t script, std::uint32_t trigger, std::uint32_t target);
    // IDA5897F0 / script virtual+0x108: execute this Skill ID immediately.
    [[nodiscard]] bool Trigger(std::uint32_t script,std::uint32_t skill,std::uint32_t target);
    // IDA5896A0: current permanent/timed skill contexts, not learned Stunt IDs.
    [[nodiscard]] bool HasSkillKey(std::uint32_t script,std::uint32_t key);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
