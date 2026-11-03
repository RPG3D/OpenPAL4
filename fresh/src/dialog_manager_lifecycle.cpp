#include "pal4/dialog_manager_lifecycle.h"

#include <cstring>
#include <stdexcept>

namespace pal4 {
namespace {

std::uint32_t ReleaseSharedNil(
    DialogManagerPrimaryTreeGlobals& globals,
    IDialogManagerPrimaryTreeRuntime& runtime) {
    const auto remaining = --globals.references;
    if (remaining != 0) return remaining;
    const auto nil = globals.nil_node;
    globals.nil_node = 0;
    return nil == 0 ? 0 : runtime.FreeMemory(nil);
}

}  // namespace

std::uint32_t DialogManagerState::Word(const std::size_t offset) const {
    if (offset % 4 != 0 || offset + 4 > image_.size())
        throw std::out_of_range("dialog manager word offset");
    std::uint32_t value = 0;
    std::memcpy(&value, image_.data() + offset, sizeof(value));
    return value;
}

void DialogManagerState::SetWord(const std::size_t offset,
                                 const std::uint32_t value) {
    if (offset % 4 != 0 || offset + 4 > image_.size())
        throw std::out_of_range("dialog manager word offset");
    std::memcpy(image_.data() + offset, &value, sizeof(value));
}

std::uint8_t DialogManagerState::Byte(const std::size_t offset) const {
    if (offset >= image_.size())
        throw std::out_of_range("dialog manager byte offset");
    return image_[offset];
}

void DialogManagerState::SetByte(const std::size_t offset,
                                 const std::uint8_t value) {
    if (offset >= image_.size())
        throw std::out_of_range("dialog manager byte offset");
    image_[offset] = value;
}

DialogManagerState* DialogManagerLifecycle::Construct(
    DialogManagerState& state, const std::uint8_t scratch_byte,
    IDialogManagerLifecycleRuntime& runtime) {
    state.SetWord(24, 0);
    for (std::size_t offset = 84; offset <= 112; offset += 4)
        state.SetWord(offset, 0);

    constexpr std::array<std::size_t, 4> tree_offsets{116, 132, 148, 164};
    std::size_t constructed = 0;
    bool array_constructed = false;
    try {
        for (const auto offset : tree_offsets) {
            state.SetByte(offset, scratch_byte);
            state.SetByte(offset + 1, scratch_byte);
            state.SetByte(offset + 8, 0);
            runtime.ConstructTree(state, offset,
                                  offset == 116 ? DialogTreeKind::Extended
                                                : DialogTreeKind::Standard);
            ++constructed;
        }
        state.SetWord(184, 0);
        state.SetByte(180, scratch_byte);
        state.SetWord(188, 0);
        state.SetWord(192, 0);
        array_constructed = true;
        runtime.Reset(state);
    } catch (...) {
        if (array_constructed) runtime.CleanupArray(state, 180);
        while (constructed != 0)
            runtime.DestroyTree(state, tree_offsets[--constructed]);
        throw;
    }
    return &state;
}

std::uint32_t DialogManagerLifecycle::DestroyPrimaryTree(
    DialogManagerState& state, DialogManagerPrimaryTreeGlobals& globals,
    IDialogManagerPrimaryTreeRuntime& runtime) {
    const auto head = state.Word(4);
    auto node = runtime.NodeWord(head, 0);
    if (state.Word(12) != 0) {
        runtime.FreeSubtree(state, runtime.NodeWord(head, 4));
        runtime.SetNodeWord(head, 4, globals.nil_node);
        state.SetWord(12, 0);
        runtime.SetNodeWord(head, 0, head);
        runtime.SetNodeWord(head, 8, head);
        runtime.FirstElement(state);
    } else {
        while (node != head) {
            const auto current = node;
            node = runtime.NextNode(current);
            runtime.DeleteNode(state, current);
        }
    }

    static_cast<void>(runtime.FreeMemory(head));
    state.SetWord(4, 0);
    state.SetWord(12, 0);
    return ReleaseSharedNil(globals, runtime);
}

std::uint32_t DialogManagerLifecycle::Cleanup(
    DialogManagerState& state,
    DialogManagerPrimaryTreeGlobals& collection_globals,
    DialogManagerPrimaryTreeGlobals& primary_globals,
    IDialogManagerCleanupRuntime& runtime) {
    constexpr std::array<std::size_t, 3> collection_offsets{164, 148, 132};
    constexpr std::array<std::size_t, 3> unwind_offsets{148, 132, 116};
    std::size_t pending = unwind_offsets.size();
    try {
        static_cast<void>(runtime.FreeMemory(state.Word(184)));
        state.SetWord(184, 0);
        state.SetWord(188, 0);
        state.SetWord(192, 0);

        for (const auto offset : collection_offsets) {
            const auto head = state.Word(offset + 4);
            const auto first = offset == 164
                                   ? runtime.FirstElementAt(state, offset)
                                   : runtime.NodeWord(head, 0);
            runtime.DeleteRange(state, offset, first, head);
            static_cast<void>(runtime.FreeMemory(head));
            state.SetWord(offset + 4, 0);
            state.SetWord(offset + 12, 0);
            static_cast<void>(ReleaseSharedNil(collection_globals, runtime));
            --pending;
        }

        const auto primary_head = state.Word(120);
        runtime.ClearRange(state, 116,
                           runtime.NodeWord(primary_head, 0), primary_head);
        static_cast<void>(runtime.FreeMemory(primary_head));
        state.SetWord(120, 0);
        state.SetWord(128, 0);
        return ReleaseSharedNil(primary_globals, runtime);
    } catch (...) {
        for (std::size_t index = unwind_offsets.size() - pending;
             index < unwind_offsets.size(); ++index)
            runtime.DestroyTreeOnException(state, unwind_offsets[index]);
        throw;
    }
}

std::uint32_t DialogManagerLifecycle::ResetState(
    DialogManagerState& state, IDialogManagerResetRuntime& runtime) {
    for (std::size_t offset = 28; offset <= 80; offset += 4)
        state.SetWord(offset, 0);
    state.SetWord(24, 0);
    for (std::size_t offset = 84; offset <= 112; offset += 4)
        state.SetWord(offset, 0);

    GlobalDataSettingsState settings;
    for (std::size_t index = 0; index < settings.words.size(); ++index)
        settings.words[index] = state.Word(index * 4);
    settings.byte_14 = state.Byte(20);
    static_cast<void>(GlobalDataSettings::ResetDefaults(settings, runtime));
    for (std::size_t index = 0; index < settings.words.size(); ++index)
        state.SetWord(index * 4, settings.words[index]);
    state.SetByte(20, settings.byte_14);

    for (const auto offset : {132u, 164u, 148u, 116u})
        runtime.ClearTree(state, offset);
    runtime.ClearStoryArray(state);

    const auto game = runtime.GameInstance();
    std::uint32_t result = game;
    for (std::uint32_t index = 0;; ++index) {
        const auto begin = runtime.GameStoryBegin(game);
        if (begin == 0) break;
        const auto count = (runtime.GameStoryEnd(game) - begin) / 4;
        result = count;
        if (index >= count) break;
        result = runtime.InsertStory(
            state, runtime.GameStoryValue(begin + index * 4));
    }
    return result;
}

}  // namespace pal4
