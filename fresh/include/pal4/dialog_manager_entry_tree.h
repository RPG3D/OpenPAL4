#pragma once

#include "pal4/dialog_manager_lifecycle.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct DialogEntryInsertResult {
    std::uint32_t node = 0;
    bool inserted = false;
};

class IDialogManagerEntryTreeRuntime {
public:
    virtual ~IDialogManagerEntryTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NilNode() = 0;
    [[nodiscard]] virtual std::uint32_t NodeWord(
        std::uint32_t node, std::size_t offset) = 0;
    virtual void SetNodeWord(std::uint32_t node, std::size_t offset,
                             std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateNode(
        std::uint32_t bytes) = 0;
};

class IDialogManagerEntryTreeEraseRuntime
    : public IDialogManagerEntryTreeRuntime {
public:
    virtual void FreeNode(std::uint32_t node) = 0;
};

class DialogManagerEntryTree final {
public:
    // IDA 0x5ce860.
    [[nodiscard]] static DialogEntryInsertResult Insert(
        DialogManagerState& state, std::uint32_t key,
        IDialogManagerEntryTreeRuntime& runtime);
    // IDA 0x5cf1b0.
    [[nodiscard]] static std::uint32_t& InsertAt(
        DialogManagerState& state, std::uint32_t& output,
        std::uint32_t leaf, std::uint32_t parent, std::uint32_t key,
        IDialogManagerEntryTreeRuntime& runtime);
    // IDA 0x5cf440 / 0x5cf4a0.
    [[nodiscard]] static std::uint32_t RotateLeft(
        DialogManagerState& state, std::uint32_t node,
        IDialogManagerEntryTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        DialogManagerState& state, std::uint32_t node,
        IDialogManagerEntryTreeRuntime& runtime);
    // IDA 0x5cf500.
    [[nodiscard]] static std::uint32_t CreateNode(
        std::uint32_t parent, std::uint32_t color,
        IDialogManagerEntryTreeRuntime& runtime);
    // IDA 0x5cf520 (despite the IDA name, traverses to the predecessor).
    [[nodiscard]] static std::uint32_t Predecessor(
        std::uint32_t& node, IDialogManagerEntryTreeRuntime& runtime);
    // IDA 0x5ced10.
    [[nodiscard]] static std::uint32_t& Erase(
        DialogManagerState& state, std::uint32_t& output,
        std::uint32_t node, IDialogManagerEntryTreeEraseRuntime& runtime);
};

}  // namespace pal4
