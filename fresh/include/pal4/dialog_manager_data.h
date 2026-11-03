#pragma once

#include "pal4/dialog_manager_lifecycle.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pal4 {

class IDialogManagerEventRuntime {
public:
    virtual ~IDialogManagerEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NilNode() = 0;
    [[nodiscard]] virtual std::uint32_t NodeWord(
        std::uint32_t node, std::size_t offset) = 0;
    [[nodiscard]] virtual std::uint8_t FindPredecessor(
        std::uint32_t& node) = 0;
    [[nodiscard]] virtual std::uint8_t Insert(
        DialogManagerState& state, std::uint32_t leaf,
        std::uint32_t parent, std::uint32_t key) = 0;
};

class IDialogManagerStoryMergeRuntime {
public:
    virtual ~IDialogManagerStoryMergeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t GameStoryBegin(
        std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t GameStoryEnd(
        std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t GameStoryObject(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ObjectStat(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int32_t StoryStat(
        std::uint32_t address) = 0;
    virtual void SetStoryStat(std::uint32_t address,
                              std::int32_t value) = 0;
};

struct DialogStatCandidate {
    std::int32_t story_index = 0;
    std::int32_t threshold = 0;
    float score = 0.0f;
};

class IDialogManagerStatRuntime : public IDialogManagerStoryMergeRuntime {
public:
    [[nodiscard]] virtual std::vector<DialogStatCandidate>
        SortedCandidatesFromGameDb() = 0;
};

class DialogManagerData final {
public:
    // IDA 0x5ce2d0.
    [[nodiscard]] static double NormalizedStat(
        const DialogManagerState& state);
    // IDA 0x5ce300.
    [[nodiscard]] static std::uint8_t InsertEvent(
        DialogManagerState& state, std::uint32_t key,
        IDialogManagerEventRuntime& runtime);
    // IDA 0x5ce3a0.
    [[nodiscard]] static std::uint32_t MergeStoryMax(
        const DialogManagerState& state,
        IDialogManagerStoryMergeRuntime& runtime);
    // IDA 0x5ce3f0.
    [[nodiscard]] static double CalculateStatValue(
        const DialogManagerState& state,
        IDialogManagerStatRuntime& runtime);
};

}  // namespace pal4
