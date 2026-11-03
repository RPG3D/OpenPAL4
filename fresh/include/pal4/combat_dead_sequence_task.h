#pragma once

#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;
class IFatalSequenceRuntime;

class DeadSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 52u;
    static constexpr std::uint32_t kVtable = 0x845ba0u;

    // IDA 0x567a90/0x567ab0: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x56b160: update actor state and schedule death children.
    static void Start(std::uint32_t task, IFatalSequenceRuntime& runtime);
};

class QuickDeadSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 52u;
    static constexpr std::uint32_t kVtable = 0x845bbcu;

    // IDA 0x567b40/0x567b60: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x56b5a0: event child followed by source/target end child.
    static void Start(std::uint32_t task, IFatalSequenceRuntime& runtime);
};

}  // namespace pal4
