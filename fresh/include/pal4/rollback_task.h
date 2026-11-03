#pragma once

#include <cstdint>

namespace pal4 {

class IRollbackTaskRuntime {
public:
    virtual ~IRollbackTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void CleanupCollection(std::uint32_t collection) = 0;
    virtual void NotifyPrevious(std::uint32_t task) = 0;
    [[nodiscard]] virtual bool SiblingComplete(std::uint32_t task) = 0;
    [[nodiscard]] virtual bool WaitComplete(std::uint32_t object) = 0;
};

class RollbackTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8488f4u;
    // IDA 0x598250..0x5982e0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IRollbackTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IRollbackTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t ReturnZeroWithArgument(
        std::uint32_t ignored);
    [[nodiscard]] static std::uint8_t ReturnZero();
    [[nodiscard]] static bool WaitUntilSiblingsDone(
        std::uint32_t object, IRollbackTaskRuntime& runtime);
};

}  // namespace pal4
