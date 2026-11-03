#include "pal4/sound_player.h"

namespace pal4 {

std::uint32_t SoundPlayer::ResolveNode(
    const std::uint32_t tree, const std::uint8_t seed,
    const SoundEvent event, const bool insert_variant,
    ISoundPlayerRuntime& runtime) {
    runtime.BeginTemporaryText(seed);
    std::uint32_t key = 0u;
    bool key_created = false;
    std::uint32_t node = 0u;
    try {
        key = runtime.CopyCeguiKey(event);
        key_created = true;
        node = insert_variant
                   ? runtime.InsertAndResolve(tree, key)
                   : runtime.FindOrInsert(tree, key);
    } catch (...) {
        if (key_created) runtime.DestroyCeguiKey(key);
        runtime.EndTemporaryText();
        throw;
    }
    runtime.DestroyCeguiKey(key);
    runtime.EndTemporaryText();
    return node;
}

void SoundPlayer::AssignFile(
    const std::uint32_t node, const std::string_view path,
    const bool use_size_helper, ISoundPlayerRuntime& runtime) {
    const std::uint32_t record = node + 164u;
    const std::uint32_t length = static_cast<std::uint32_t>(path.size());
    if (!runtime.AllocateText(record, length, 1u)) return;
    const std::uint32_t data = runtime.ReadWord(record + 4u);
    runtime.CopyText(data, path);
    if (use_size_helper) {
        runtime.SetTextSize(record, length);
    } else {
        runtime.WriteWord(record + 8u, length);
        runtime.WriteByte(data + length, 0u);
    }
}

std::uint32_t SoundPlayer::Construct(
    const std::uint32_t object, const std::uint8_t seed,
    ISoundPlayerRuntime& runtime) {
    runtime.WriteWord(object, runtime.ImportedBaseVtable());
    runtime.WriteByte(object + 4u, seed);
    runtime.WriteByte(object + 5u, seed);
    runtime.WriteByte(object + 12u, 0u);
    const std::uint32_t tree = object + 4u;
    runtime.ConstructTree(tree);
    runtime.WriteWord(object, kVtable);

    const std::uint32_t clicked = ResolveNode(
        tree, seed, SoundEvent::Clicked, false, runtime);
    AssignFile(clicked, "wa01.wav", false, runtime);
    const std::uint32_t denied = ResolveNode(
        tree, seed, SoundEvent::ClickDenied, true, runtime);
    AssignFile(denied, "wa05.wav", false, runtime);
    const std::uint32_t selected = ResolveNode(
        tree, seed, SoundEvent::SelectionChanged, false, runtime);
    AssignFile(selected, "wa02.wav", true, runtime);
    return object;
}

std::uint32_t SoundPlayer::DestroyTree(
    const std::uint32_t tree, ISoundPlayerRuntime& runtime) {
    const std::uint32_t head = runtime.ReadWord(tree + 4u);
    std::uint32_t node = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        runtime.FreeTreeNodes(tree, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.ReadWord(0x8edfc8u));
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.GetFirstTreeNode(tree);
    } else {
        while (node != head) {
            const std::uint32_t current = node;
            node = runtime.NextTreeNode(node);
            runtime.DeleteTreeNode(tree, current);
        }
    }
    (void)runtime.Free(head);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const std::uint32_t references =
        runtime.ReadWord(0x8edfc4u) - 1u;
    runtime.WriteWord(0x8edfc4u, references);
    if (references != 0u) return references;
    const std::uint32_t nil = runtime.ReadWord(0x8edfc8u);
    runtime.WriteWord(0x8edfc8u, 0u);
    return nil == 0u ? 0u : runtime.Free(nil);
}

std::uint8_t SoundPlayer::FindAndPlayMusic(
    const std::uint32_t object, const std::uint32_t key,
    ISoundPlayerRuntime& runtime) {
    const std::uint32_t tree = object + 4u;
    const std::uint32_t candidate = runtime.FindTreeNode(tree, key);
    const std::uint32_t head = runtime.ReadWord(tree + 4u);
    const std::uint32_t selected =
        candidate == head || runtime.CeguiLess(key, candidate + 12u)
            ? head : candidate;
    if (selected == runtime.ReadWord(object + 8u))
        return static_cast<std::uint8_t>(selected);
    std::uint32_t path = runtime.ReadWord(selected + 168u);
    if (path == 0u) path = runtime.EmptyTextPointer();
    return runtime.PlayMusic(path, 1u);
}

std::uint32_t SoundPlayer::FindOrInsertNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t key, ISoundPlayerRuntime& runtime) {
    const std::uint32_t head = runtime.ReadWord(tree + 4u);
    const std::uint32_t nil = runtime.ReadWord(0x8edfc8u);
    std::uint32_t parent = head;
    std::uint32_t child = runtime.ReadWord(head + 4u);
    bool less = true;
    while (child != nil) {
        parent = child;
        less = runtime.CeguiLess(key, child + 12u);
        child = runtime.ReadWord(child + (less ? 0u : 8u));
    }

    bool inserted = false;
    std::uint32_t selected = parent;
    if (runtime.ReadByte(tree + 8u) != 0u) {
        selected = runtime.InsertTreeNode(tree, child, parent, key);
        inserted = true;
    } else {
        if (less) {
            if (parent == runtime.ReadWord(head)) {
                selected = runtime.InsertTreeNode(
                    tree, child, parent, key);
                inserted = true;
            } else {
                selected = runtime.PreviousTreeNode(parent);
            }
        }
        if (!inserted && runtime.CeguiLess(selected + 12u, key)) {
            selected = runtime.InsertTreeNode(
                tree, child, parent, key);
            inserted = true;
        }
    }
    runtime.WriteWord(output, selected);
    runtime.WriteByte(output + 4u, inserted ? 1u : 0u);
    return output;
}

std::uint32_t SoundEventKeyRecord::CopyWithProperties(
    const std::uint32_t destination, const std::uint32_t source_cegui,
    const std::uint32_t source_text,
    ISoundPlayerRuntime& runtime) {
    runtime.WriteWord(destination + 4u, 32u);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 16u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    runtime.WriteWord(destination, 0u);
    runtime.WriteWord(destination + 20u, 0u);

    const std::uint32_t length = runtime.ReadWord(source_cegui);
    runtime.GrowCeguiString(destination, length);
    runtime.WriteWord(destination, length);
    const std::uint32_t target =
        runtime.ReadWord(destination + 4u) <= 32u
            ? destination + 20u
            : runtime.ReadWord(destination + 148u);
    runtime.WriteWord(target + length * 4u, 0u);
    const std::uint32_t source =
        runtime.ReadWord(source_cegui + 4u) <= 32u
            ? source_cegui + 20u
            : runtime.ReadWord(source_cegui + 148u);
    runtime.CopyMemory(target, source, length * 4u);

    runtime.WriteByte(destination + 152u, runtime.ReadByte(source_text));
    runtime.WriteWord(destination + 156u, 0u);
    runtime.WriteWord(destination + 160u, 0u);
    runtime.WriteWord(destination + 164u, 0u);
    (void)runtime.AssignTextRecord(destination + 152u, source_text,
                                   0u, 0xffffffffu);
    return destination;
}

std::uint32_t SoundEventKeyRecord::CopyFromRecord(
    const std::uint32_t destination, const std::uint32_t source,
    ISoundPlayerRuntime& runtime) {
    if (destination == 0u) return 0u;
    runtime.WriteWord(destination + 4u, 32u);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 16u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    runtime.WriteWord(destination, 0u);
    runtime.WriteWord(destination + 20u, 0u);

    const std::uint32_t length = runtime.ReadWord(source);
    runtime.GrowCeguiString(destination, length);
    runtime.SetCeguiStringLength(destination, length);
    const std::uint32_t source_data =
        runtime.ReadWord(source + 4u) <= 32u
            ? source + 20u : runtime.ReadWord(source + 148u);
    const std::uint32_t destination_data =
        runtime.ReadWord(destination + 4u) <= 32u
            ? destination + 20u
            : runtime.ReadWord(destination + 148u);
    runtime.CopyMemory(destination_data, source_data, length * 4u);
    runtime.WriteByte(destination + 152u,
                      runtime.ReadByte(source + 152u));
    runtime.WriteWord(destination + 156u, 0u);
    runtime.WriteWord(destination + 160u, 0u);
    runtime.WriteWord(destination + 164u, 0u);
    return runtime.AssignTextRecord(destination + 152u, source + 152u,
                                    0u, 0xffffffffu);
}

}  // namespace pal4
