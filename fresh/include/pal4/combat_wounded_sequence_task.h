#pragma once

#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;
class IFatalSequenceRuntime;
class IFinalHitCompletionRuntime;
class IWoundedSequenceStartRuntime;
class IWoundedDeathFactoryRuntime;

class FinalHitSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 52u;
    static constexpr std::uint32_t kVtable = 0x845bf4u;

    // IDA 0x567de0/0x567e00: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x56b390: update actor state and schedule final-hit children.
    static void Start(std::uint32_t task, IFatalSequenceRuntime& runtime);
    // IDA 0x56b570: invoke actor virtual slots +172 and +544.
    [[nodiscard]] static std::uint32_t CreateNext(
        std::uint32_t task, IFinalHitCompletionRuntime& runtime);
};

class WoundedSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 52u;
    static constexpr std::uint32_t kVtable = 0x845bd8u;

    // IDA 0x567e90/0x567eb0: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x56b680: state, sound and two child tasks.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, IWoundedSequenceStartRuntime& runtime);
    // IDA 0x56b7e0: create Dead when the source actor is in state 3.
    [[nodiscard]] static std::uint32_t CreateDeathTask(
        std::uint32_t task, IWoundedDeathFactoryRuntime& runtime);
};

class QuickWoundedSequenceTask final {
public:
    static constexpr std::uint32_t kSize = 52u;
    static constexpr std::uint32_t kVtable = 0x845c10u;

    // IDA 0x567f40/0x567f60: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
    // IDA 0x56b950: create QuickDead when the target actor is in state 3.
    [[nodiscard]] static std::uint32_t CreateDeathTask(
        std::uint32_t task, IWoundedDeathFactoryRuntime& runtime);
};

}  // namespace pal4
