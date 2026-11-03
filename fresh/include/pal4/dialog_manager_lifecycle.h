#pragma once

#include "pal4/global_data_settings.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

class DialogManagerState final {
public:
    static constexpr std::size_t kImageSize = 0xc4;

    [[nodiscard]] std::uint32_t Word(std::size_t offset) const;
    void SetWord(std::size_t offset, std::uint32_t value);
    [[nodiscard]] std::uint8_t Byte(std::size_t offset) const;
    void SetByte(std::size_t offset, std::uint8_t value);
    [[nodiscard]] std::uint8_t* ImageData() noexcept { return image_.data(); }
    [[nodiscard]] const std::uint8_t* ImageData() const noexcept { return image_.data(); }

private:
    std::array<std::uint8_t, kImageSize> image_{};
};

enum class DialogTreeKind : std::uint8_t {
    Extended,
    Standard,
};

class IDialogManagerLifecycleRuntime {
public:
    virtual ~IDialogManagerLifecycleRuntime() = default;
    virtual void ConstructTree(DialogManagerState& state,
                               std::size_t offset, DialogTreeKind kind) = 0;
    virtual void DestroyTree(DialogManagerState& state,
                             std::size_t offset) = 0;
    virtual void CleanupArray(DialogManagerState& state,
                              std::size_t offset) = 0;
    virtual void Reset(DialogManagerState& state) = 0;
};

struct DialogManagerPrimaryTreeGlobals {
    std::uint32_t references = 0;
    std::uint32_t nil_node = 0;
};

class IDialogManagerPrimaryTreeRuntime {
public:
    virtual ~IDialogManagerPrimaryTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NodeWord(
        std::uint32_t node, std::size_t offset) = 0;
    virtual void SetNodeWord(std::uint32_t node, std::size_t offset,
                             std::uint32_t value) = 0;
    virtual void FreeSubtree(DialogManagerState& state,
                             std::uint32_t root) = 0;
    virtual void FirstElement(DialogManagerState& state) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(std::uint32_t node) = 0;
    virtual void DeleteNode(DialogManagerState& state,
                            std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(std::uint32_t block) = 0;
};

class IDialogManagerCleanupRuntime : public IDialogManagerPrimaryTreeRuntime {
public:
    [[nodiscard]] virtual std::uint32_t FirstElementAt(
        DialogManagerState& state, std::size_t tree_offset) = 0;
    virtual void DeleteRange(DialogManagerState& state,
                             std::size_t tree_offset,
                             std::uint32_t first, std::uint32_t last) = 0;
    virtual void ClearRange(DialogManagerState& state,
                            std::size_t tree_offset,
                            std::uint32_t first, std::uint32_t last) = 0;
    virtual void DestroyTreeOnException(DialogManagerState& state,
                                        std::size_t tree_offset) = 0;
};

class IDialogManagerResetRuntime : public IGlobalDataSettingsRuntime {
public:
    virtual void ClearTree(DialogManagerState& state,
                           std::size_t tree_offset) = 0;
    virtual void ClearStoryArray(DialogManagerState& state) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t GameStoryBegin(
        std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t GameStoryEnd(
        std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t GameStoryValue(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InsertStory(
        DialogManagerState& state, std::uint32_t value) = 0;
};

class DialogManagerLifecycle final {
public:
    // IDA 0x5cd200. The original stack scratch byte is not initialized.
    [[nodiscard]] static DialogManagerState* Construct(
        DialogManagerState& state, std::uint8_t scratch_byte,
        IDialogManagerLifecycleRuntime& runtime);
    // IDA 0x5cd2f0.
    [[nodiscard]] static std::uint32_t DestroyPrimaryTree(
        DialogManagerState& state, DialogManagerPrimaryTreeGlobals& globals,
        IDialogManagerPrimaryTreeRuntime& runtime);
    // IDA 0x5cd3b0.
    [[nodiscard]] static std::uint32_t Cleanup(
        DialogManagerState& state,
        DialogManagerPrimaryTreeGlobals& collection_globals,
        DialogManagerPrimaryTreeGlobals& primary_globals,
        IDialogManagerCleanupRuntime& runtime);
    // IDA 0x5ce0a0.
    [[nodiscard]] static std::uint32_t ResetState(
        DialogManagerState& state, IDialogManagerResetRuntime& runtime);
};

}  // namespace pal4
