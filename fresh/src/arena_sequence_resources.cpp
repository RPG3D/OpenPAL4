#include "pal4/arena_sequence_resources.h"

#include <bit>
#include <stdexcept>
#include <new>

namespace pal4 {
namespace {

void MakeTextRecord(const std::uint32_t record, const std::uint32_t source,
                    IArenaSequenceRuntime& runtime) {
    runtime.WriteByte(record, static_cast<std::uint8_t>(source));
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
    std::uint32_t length = 0u;
    while (runtime.ReadByte(source + length) != 0u) ++length;
    const auto allocation = runtime.Allocate(length + 2u);
    if (allocation == 0u) return;
    runtime.WriteByte(allocation, 0u);
    for (std::uint32_t i = 0u; i < length; ++i)
        runtime.WriteByte(allocation + 1u + i,
                          runtime.ReadByte(source + i));
    runtime.WriteByte(allocation + 1u + length, 0u);
    runtime.WriteWord(record + 4u, allocation + 1u);
    runtime.WriteWord(record + 8u, length);
    runtime.WriteWord(record + 12u, length);
}

void ReleaseTextRecord(const std::uint32_t record,
                       IArenaSequenceRuntime& runtime) {
    const auto text = runtime.ReadWord(record + 4u);
    if (text == 0u) return;
    const auto allocation = text - 1u;
    const auto references = runtime.ReadByte(allocation);
    if (references == 0u || references == 0xffu)
        (void)runtime.Free(allocation);
    else
        runtime.WriteByte(allocation,
                          static_cast<std::uint8_t>(references - 1u));
}

void AppendClump(const std::uint32_t object, const std::uint32_t clump,
                 IArenaSequenceRuntime& runtime) {
    const auto begin = runtime.ReadWord(object + 24u);
    const auto end = runtime.ReadWord(object + 28u);
    const auto capacity_end = runtime.ReadWord(object + 32u);
    if (end != capacity_end) {
        runtime.WriteWord(end, clump);
        runtime.WriteWord(object + 28u, end + 4u);
        return;
    }
    const auto size = begin == 0u ? 0u : (end - begin) / 4u;
    const auto growth = size > 1u ? size : 1u;
    const auto replacement = runtime.Allocate((size + growth) * 4u);
    if (replacement == 0u) return;
    for (std::uint32_t i = 0u; i < size; ++i)
        runtime.WriteWord(replacement + i * 4u,
                          runtime.ReadWord(begin + i * 4u));
    runtime.WriteWord(replacement + size * 4u, clump);
    (void)runtime.Free(begin);
    runtime.WriteWord(object + 24u, replacement);
    runtime.WriteWord(object + 28u, replacement + (size + 1u) * 4u);
    runtime.WriteWord(object + 32u, replacement + (size + growth) * 4u);
}

std::uint32_t NextTreeNode(const std::uint32_t node,
                           const std::uint32_t nil,
                           IArenaSequenceRuntime& runtime) {
    auto right = runtime.ReadWord(node + 8u);
    if (right != nil) {
        auto next = right;
        while (runtime.ReadWord(next) != nil) next = runtime.ReadWord(next);
        return next;
    }
    auto current = node;
    auto parent = runtime.ReadWord(current + 4u);
    while (current == runtime.ReadWord(parent + 8u)) {
        current = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    return runtime.ReadWord(current + 8u) == parent ? current : parent;
}

}  // namespace

std::uint32_t ArenaSequenceResources::UppercaseKey(
    const std::uint32_t destination, const std::uint32_t source,
    IArenaSequenceRuntime& runtime) {
    runtime.WriteByte(destination, static_cast<std::uint8_t>(source));
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    if (runtime.ReadWord(source + 8u) == 0u) return destination;

    const auto text = runtime.ReadWord(source + 4u);
    if (text == 0u) return destination;
    std::uint32_t length = 0u;
    while (runtime.ReadByte(text + length) != 0u) ++length;
    const auto allocation = runtime.Allocate(length + 2u);
    if (allocation == 0u) return destination;
    const auto output = allocation + 1u;
    runtime.WriteByte(allocation, 0u);
    for (std::uint32_t i = 0u; i < length; ++i) {
        const auto ch = runtime.ReadByte(text + i);
        runtime.WriteByte(output + i,
                          ch >= 'a' && ch <= 'z' ?
                              static_cast<std::uint8_t>(ch - 'a' + 'A') : ch);
    }
    runtime.WriteByte(output + length, 0u);
    runtime.WriteWord(destination + 4u, output);
    runtime.WriteWord(destination + 8u, length);
    runtime.WriteWord(destination + 12u, length);
    return destination;
}

std::uint32_t ArenaSequenceResources::Initialize(
    const std::uint32_t object, std::uint32_t& shared_nil,
    std::uint32_t& shared_references, IArenaSequenceRuntime& runtime) {
    const auto high_byte = static_cast<std::uint8_t>(object >> 24u);
    runtime.WriteByte(object, 0u);
    runtime.WriteByte(object + 4u, high_byte);
    runtime.WriteByte(object + 5u, high_byte);
    runtime.WriteByte(object + 12u, 0u);
    const auto candidate = runtime.Allocate(36u);
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 32u, 1u);
    if (shared_nil == 0u) {
        shared_nil = candidate;
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    } else {
        (void)runtime.Free(candidate);
    }
    ++shared_references;
    const auto head = runtime.Call(ArenaSequenceCall::AllocateTreeHead,
                                   {object + 4u, shared_nil, 0u});
    runtime.WriteWord(object + 8u, head);
    runtime.WriteWord(object + 16u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    runtime.WriteByte(object + 20u, high_byte);
    runtime.WriteWord(object + 24u, 0u);
    runtime.WriteWord(object + 28u, 0u);
    runtime.WriteWord(object + 32u, 0u);
    runtime.WriteByte(object + 36u, high_byte);
    runtime.WriteWord(object + 40u, 0u);
    runtime.WriteWord(object + 44u, 0u);
    runtime.WriteWord(object + 48u, 0u);
    return object;
}

std::uint32_t ArenaSequenceResources::Cleanup(
    const std::uint32_t object, std::uint32_t& shared_nil,
    std::uint32_t& shared_references, IArenaSequenceRuntime& runtime) {
    const auto text = runtime.ReadWord(object + 40u);
    if (text != 0u) {
        const auto allocation = text - 1u;
        const auto references = runtime.ReadByte(allocation);
        if (references == 0u || references == 0xffu)
            (void)runtime.Free(allocation);
        else
            runtime.WriteByte(allocation,
                              static_cast<std::uint8_t>(references - 1u));
    }
    for (const auto offset : {40u, 44u, 48u})
        runtime.WriteWord(object + offset, 0u);
    (void)runtime.Free(runtime.ReadWord(object + 24u));
    for (const auto offset : {24u, 28u, 32u})
        runtime.WriteWord(object + offset, 0u);

    const auto head = runtime.ReadWord(object + 8u);
    if (runtime.ReadWord(object + 16u) != 0u) {
        (void)runtime.Call(ArenaSequenceCall::DeleteTreeSubtree,
                           {object + 4u, runtime.ReadWord(head + 4u)});
        runtime.WriteWord(head + 4u, shared_nil);
        runtime.WriteWord(object + 16u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        (void)runtime.Call(ArenaSequenceCall::TreeFirstElement,
                           {object + 4u});
    } else if (runtime.ReadWord(head) != head) {
        (void)runtime.Call(ArenaSequenceCall::EraseRemainingTreeNodes,
                           {object + 4u, head});
    }
    (void)runtime.Free(head);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 16u, 0u);
    --shared_references;
    if (shared_references != 0u) return shared_references;
    const auto nil = shared_nil;
    shared_nil = 0u;
    return nil == 0u ? 0u : runtime.Free(nil);
}

void ArenaSequenceResources::Update(
    const std::uint32_t object, const double seconds,
    IArenaSequenceRuntime& runtime) {
    if (runtime.ReadByte(object) == 0u) return;
    const auto delta = std::bit_cast<std::uint32_t>(
        static_cast<float>(seconds));
    for (auto cursor = runtime.ReadWord(object + 24u);
         cursor != runtime.ReadWord(object + 28u); cursor += 4u)
        (void)runtime.Call(ArenaSequenceCall::UpdateAnimationTime,
                           {runtime.ReadWord(cursor), delta});
}

void ArenaSequenceResources::Render(
    const std::uint32_t object, const std::uint32_t pass,
    IArenaSequenceRuntime& runtime) {
    if (runtime.ReadByte(object) == 0u) return;
    for (auto cursor = runtime.ReadWord(object + 24u);
         cursor != runtime.ReadWord(object + 28u); cursor += 4u)
        (void)runtime.Call(ArenaSequenceCall::RenderClump,
                           {runtime.ReadWord(cursor), pass});
}

std::uint32_t ArenaSequenceResources::LoadClump(
    const std::uint32_t object, const std::uint32_t name,
    const std::uint32_t enabled_gate, const std::uint32_t temporary,
    IArenaSequenceRuntime& runtime) {
    if (runtime.ReadByte(object) == 0u || name == 0u ||
        enabled_gate == 0u) return 0u;
    MakeTextRecord(temporary, name, runtime);
    (void)UppercaseKey(temporary + 16u, temporary, runtime);
    const auto node = runtime.Call(ArenaSequenceCall::FindSequenceNode,
                                   {object + 4u, temporary + 16u});
    std::uint32_t result = 0u;
    if (node != runtime.ReadWord(object + 8u)) {
        const auto config = runtime.ReadWord(node + 28u);
        const auto fps = runtime.Call(ArenaSequenceCall::ReadFps, {config});
        const auto count = runtime.Call(ArenaSequenceCall::ReadCount, {config});
        const auto directory = runtime.Call(ArenaSequenceCall::ReadDir,
                                             {config});
        const auto clump_name = runtime.Call(ArenaSequenceCall::ReadClumpName,
                                              {config});
        const auto texture = runtime.Call(ArenaSequenceCall::ReadTextureName,
                                           {config});
        const auto allocation = runtime.Allocate(64u);
        const auto clump = allocation == 0u ? 0u :
            runtime.Call(ArenaSequenceCall::ConstructClump, {allocation});
        if (clump != 0u) {
            const auto asset_host = runtime.Call(ArenaSequenceCall::GetAssetHost,
                                                  {});
            const auto resource_name = runtime.ReadWord(object + 40u);
            if (runtime.Call(ArenaSequenceCall::LoadClump,
                             {clump, resource_name, clump_name, asset_host,
                              texture, fps, count}) != 0u) {
                AppendClump(object, clump, runtime);
                result = runtime.ReadWord(clump + 16u);
            } else {
                (void)runtime.Call(ArenaSequenceCall::DestroyClump,
                                   {clump, 1u});
            }
        }
        (void)directory;
    }
    ReleaseTextRecord(temporary + 16u, runtime);
    ReleaseTextRecord(temporary, runtime);
    return result;
}

std::uint32_t ArenaSequenceResources::LoadSequenceXml(
    const std::uint32_t object, const std::uint32_t source,
    const std::uint32_t document, const std::uint32_t temporary,
    const std::uint32_t shared_nil,
    IArenaSequenceRuntime& runtime) {
    Reset(object, shared_nil, runtime);
    const auto file = runtime.Call(ArenaSequenceCall::OpenSequenceFile,
                                   {source});
    if (file == 0u) return 0u;
    (void)runtime.Call(ArenaSequenceCall::InitXmlDocument, {document});
    if (runtime.Call(ArenaSequenceCall::ParseXmlFile,
                     {file, document}) == 0u) {
        (void)runtime.Call(ArenaSequenceCall::CloseSequenceFile, {file});
        (void)runtime.Call(ArenaSequenceCall::DestroyXmlDocument,
                           {document});
        return 0u;
    }
    (void)runtime.Call(ArenaSequenceCall::CloseSequenceFile, {file});
    for (auto node = runtime.Call(ArenaSequenceCall::FirstSequenceNode,
                                  {document}); node != 0u;
         node = runtime.Call(ArenaSequenceCall::NextSequenceNode,
                             {document, node})) {
        const auto label = runtime.Call(ArenaSequenceCall::ReadSequenceLabel,
                                        {node});
        const auto child = runtime.Call(
            ArenaSequenceCall::CopyFirstSequenceChild, {node});
        MakeTextRecord(temporary, label, runtime);
        (void)UppercaseKey(temporary + 16u, temporary, runtime);
        (void)runtime.Call(ArenaSequenceCall::InsertSequenceEntry,
                           {object + 4u, temporary + 16u, child});
        ReleaseTextRecord(temporary + 16u, runtime);
        ReleaseTextRecord(temporary, runtime);
    }

    MakeTextRecord(temporary, source, runtime);
    const auto separator = FindLastOf(temporary, "\\", 0xffffffffu,
                                      runtime);
    const auto prefix = separator > 0 ?
        static_cast<std::uint32_t>(separator) : 0u;
    if (prefix != 0u)
        (void)Substring(temporary + 16u, temporary, 0u, prefix, runtime);
    const auto allocation = runtime.Allocate(prefix + 3u);
    if (allocation != 0u) {
        runtime.WriteByte(allocation, 0u);
        const auto prefix_source = prefix == 0u ? 0u :
            runtime.ReadWord(temporary + 20u);
        for (std::uint32_t i = 0u; i < prefix; ++i)
            runtime.WriteByte(allocation + 1u + i,
                              runtime.ReadByte(prefix_source + i));
        runtime.WriteByte(allocation + 1u + prefix, '\\');
        runtime.WriteByte(allocation + 2u + prefix, 0u);
        runtime.WriteWord(object + 40u, allocation + 1u);
        runtime.WriteWord(object + 44u, prefix + 1u);
        runtime.WriteWord(object + 48u, prefix + 1u);
    }
    if (prefix != 0u) ReleaseText(temporary + 16u, runtime);
    ReleaseText(temporary, runtime);
    runtime.WriteByte(object, 1u);
    (void)runtime.Call(ArenaSequenceCall::DestroyXmlDocument, {document});
    return 1u;
}

void ArenaSequenceResources::ReleaseText(
    const std::uint32_t record, IArenaSequenceRuntime& runtime) {
    ReleaseTextRecord(record, runtime);
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
}

void ArenaSequenceResources::ApplySequenceEntries(
    const std::uint32_t object, const std::uint32_t gate_bits,
    const std::uint32_t shared_nil, const std::uint32_t temporary,
    IArenaSequenceRuntime& runtime) {
    if (runtime.ReadByte(object) == 0u) return;
    runtime.WriteWord(object + 52u, gate_bits);
    const auto head = runtime.ReadWord(object + 8u);
    for (auto node = runtime.ReadWord(head); node != head;
         node = NextTreeNode(node, shared_nil, runtime)) {
        auto name = runtime.ReadWord(node + 16u);
        if (name == 0u)
            name = runtime.Call(ArenaSequenceCall::GetEmptyString, {});
        (void)LoadClump(object, name, gate_bits, temporary, runtime);
    }
}

void ArenaSequenceResources::Reset(
    const std::uint32_t object, const std::uint32_t shared_nil,
    IArenaSequenceRuntime& runtime) {
    runtime.WriteByte(object, 0u);
    const auto head = runtime.ReadWord(object + 8u);
    if (head != 0u) {
        for (auto node = runtime.ReadWord(head); node != head;
             node = NextTreeNode(node, shared_nil, runtime)) {
            const auto xml = runtime.ReadWord(node + 28u);
            if (xml != 0u)
                (void)runtime.Call(ArenaSequenceCall::DestroyXmlNode, {xml});
        }
        (void)runtime.Call(ArenaSequenceCall::ClearTree,
                           {object + 4u, runtime.ReadWord(head), head});
    }
    for (auto cursor = runtime.ReadWord(object + 24u);
         cursor != runtime.ReadWord(object + 28u); cursor += 4u) {
        const auto clump = runtime.ReadWord(cursor);
        (void)runtime.Call(ArenaSequenceCall::CleanupClump,
                           {clump, runtime.ReadWord(object + 52u)});
        if (clump != 0u)
            (void)runtime.Call(ArenaSequenceCall::DestroyClump,
                               {clump, 1u});
    }
    runtime.WriteWord(object + 28u, runtime.ReadWord(object + 24u));
    ReleaseText(object + 36u, runtime);
    runtime.WriteWord(object + 52u, 0u);
}

std::uint32_t ArenaSequenceResources::Substring(
    const std::uint32_t destination, const std::uint32_t source,
    const std::uint32_t start, const std::uint32_t count,
    IArenaSequenceRuntime& runtime) {
    runtime.WriteByte(destination, static_cast<std::uint8_t>(start));
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    const auto length = runtime.ReadWord(source + 8u);
    if (start > length) throw std::out_of_range("sequence text substring");
    const auto available = length - start;
    const auto selected = count < available ? count : available;
    if (selected == 0u) return destination;
    const auto source_data = runtime.ReadWord(source + 4u);
    if (destination != source && start == 0u && selected == length &&
        source_data != 0u && runtime.ReadByte(source_data - 1u) < 0xfeu) {
        runtime.WriteWord(destination + 4u, source_data);
        runtime.WriteWord(destination + 8u, length);
        runtime.WriteWord(destination + 12u,
                          runtime.ReadWord(source + 12u));
        runtime.WriteByte(source_data - 1u,
                          static_cast<std::uint8_t>(
                              runtime.ReadByte(source_data - 1u) + 1u));
        return destination;
    }
    const auto allocation = runtime.Allocate(selected + 2u);
    if (allocation == 0u) return destination;
    runtime.WriteByte(allocation, 0u);
    for (std::uint32_t i = 0u; i < selected; ++i)
        runtime.WriteByte(allocation + 1u + i,
                          runtime.ReadByte(source_data + start + i));
    runtime.WriteByte(allocation + 1u + selected, 0u);
    runtime.WriteWord(destination + 4u, allocation + 1u);
    runtime.WriteWord(destination + 8u, selected);
    runtime.WriteWord(destination + 12u, selected);
    return destination;
}

std::int32_t ArenaSequenceResources::FindLastOf(
    const std::uint32_t record, const std::string_view characters,
    const std::uint32_t position, IArenaSequenceRuntime& runtime) {
    const auto length = runtime.ReadWord(record + 8u);
    if (characters.empty() || length == 0u) return -1;
    const auto data = runtime.ReadWord(record + 4u);
    auto index = position < length ? position : length - 1u;
    for (;;) {
        const auto ch = static_cast<char>(runtime.ReadByte(data + index));
        if (characters.find(ch) != std::string_view::npos)
            return static_cast<std::int32_t>(index);
        if (index == 0u) return -1;
        --index;
    }
}

namespace {

int CompareTreeKeys(const std::uint32_t left, const std::uint32_t right,
                    IArenaSequenceRuntime& runtime) {
    const auto left_data = runtime.ReadWord(left + 4u);
    const auto right_data = runtime.ReadWord(right + 4u);
    const auto left_length = runtime.ReadWord(left + 8u);
    const auto right_length = runtime.ReadWord(right + 8u);
    const auto shared = left_length < right_length ? left_length : right_length;
    for (std::uint32_t index = 0u; index < shared; ++index) {
        const auto a = left_data == 0u ? 0u : runtime.ReadByte(left_data + index);
        const auto b = right_data == 0u ? 0u : runtime.ReadByte(right_data + index);
        if (a != b) return a < b ? -1 : 1;
    }
    if (left_length == right_length) return 0;
    return left_length < right_length ? -1 : 1;
}

void SetTreeResult(const std::uint32_t result, const std::uint32_t node,
                   const bool inserted, IArenaSequenceRuntime& runtime) {
    runtime.WriteWord(result, node);
    runtime.WriteByte(result + 4u, inserted ? 1u : 0u);
}

void LinkAndBalanceTreeNode(const std::uint32_t tree,
                            const std::uint32_t node,
                            const std::uint32_t cursor,
                            const std::uint32_t parent,
                            const std::uint32_t entry,
                            const std::uint32_t nil,
                            IArenaSequenceRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    ArenaSequenceTree::CopyEntry(node + 12u, entry, runtime);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) + 1u);
    const bool link_left = parent == head || cursor != nil ||
                           CompareTreeKeys(entry, parent + 12u, runtime) < 0;
    runtime.WriteWord(parent + (link_left ? 0u : 8u), node);
    if (parent == head) {
        runtime.WriteWord(head + 4u, node);
        runtime.WriteWord(head + 8u, node);
    } else if (link_left && parent == runtime.ReadWord(head)) {
        runtime.WriteWord(head, node);
    } else if (!link_left && parent == runtime.ReadWord(head + 8u)) {
        runtime.WriteWord(head + 8u, node);
    }

    auto current = node;
    while (current != runtime.ReadWord(head + 4u)) {
        const auto current_parent = runtime.ReadWord(current + 4u);
        if (runtime.ReadWord(current_parent + 32u) != 0u) break;
        const auto grandparent = runtime.ReadWord(current_parent + 4u);
        if (current_parent == runtime.ReadWord(grandparent)) {
            const auto uncle = runtime.ReadWord(grandparent + 8u);
            if (runtime.ReadWord(uncle + 32u) == 0u) {
                runtime.WriteWord(current_parent + 32u, 1u);
                runtime.WriteWord(uncle + 32u, 1u);
                runtime.WriteWord(grandparent + 32u, 0u);
                current = grandparent;
            } else {
                if (current == runtime.ReadWord(current_parent + 8u)) {
                    current = current_parent;
                    (void)ArenaSequenceTree::RotateLeft(tree, current, nil,
                                                         runtime);
                }
                const auto new_parent = runtime.ReadWord(current + 4u);
                const auto new_grandparent = runtime.ReadWord(new_parent + 4u);
                runtime.WriteWord(new_parent + 32u, 1u);
                runtime.WriteWord(new_grandparent + 32u, 0u);
                (void)ArenaSequenceTree::RotateRight(tree, new_grandparent,
                                                      nil, runtime);
            }
        } else {
            const auto uncle = runtime.ReadWord(grandparent);
            if (runtime.ReadWord(uncle + 32u) == 0u) {
                runtime.WriteWord(current_parent + 32u, 1u);
                runtime.WriteWord(uncle + 32u, 1u);
                runtime.WriteWord(grandparent + 32u, 0u);
                current = grandparent;
            } else {
                if (current == runtime.ReadWord(current_parent)) {
                    current = current_parent;
                    (void)ArenaSequenceTree::RotateRight(tree, current, nil,
                                                          runtime);
                }
                const auto new_parent = runtime.ReadWord(current + 4u);
                const auto new_grandparent = runtime.ReadWord(new_parent + 4u);
                runtime.WriteWord(new_parent + 32u, 1u);
                runtime.WriteWord(new_grandparent + 32u, 0u);
                (void)ArenaSequenceTree::RotateLeft(tree, new_grandparent,
                                                     nil, runtime);
            }
        }
    }
    runtime.WriteWord(runtime.ReadWord(head + 4u) + 32u, 1u);
}

}  // namespace

void ArenaSequenceTree::Insert(const std::uint32_t tree,
                               const std::uint32_t result,
                               const std::uint32_t entry,
                               const std::uint32_t nil,
                               IArenaSequenceRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto parent = head;
    auto cursor = runtime.ReadWord(head + 4u);
    bool goes_left = true;
    while (cursor != nil) {
        parent = cursor;
        goes_left = CompareTreeKeys(entry, cursor + 12u, runtime) < 0;
        cursor = runtime.ReadWord(cursor + (goes_left ? 0u : 8u));
    }

    if (runtime.ReadByte(tree + 8u) == 0u) {
        auto candidate = parent;
        if (goes_left) {
            if (parent == runtime.ReadWord(head)) {
                InsertAt(tree, result, cursor, parent, entry, nil, runtime);
                const auto node = runtime.ReadWord(result);
                SetTreeResult(result, node, true, runtime);
                return;
            }
            runtime.WriteWord(result, parent);
            Previous(result, nil, runtime);
            candidate = runtime.ReadWord(result);
        }
        if (CompareTreeKeys(candidate + 12u, entry, runtime) >= 0) {
            SetTreeResult(result, candidate, false, runtime);
            return;
        }
        InsertAt(tree, result, cursor, parent, entry, nil, runtime);
        const auto node = runtime.ReadWord(result);
        SetTreeResult(result, node, true, runtime);
        return;
    }

    const auto node = runtime.Call(ArenaSequenceCall::AllocateTreeNode,
                                   {tree, parent, 0u});
    if (node == 0u) throw std::bad_alloc();
    LinkAndBalanceTreeNode(tree, node, cursor, parent, entry, nil, runtime);
    SetTreeResult(result, node, true, runtime);
}

void ArenaSequenceTree::InsertAt(const std::uint32_t tree,
                                 const std::uint32_t result,
                                 const std::uint32_t cursor,
                                 const std::uint32_t parent,
                                 const std::uint32_t entry,
                                 const std::uint32_t nil,
                                 IArenaSequenceRuntime& runtime) {
    const auto node = runtime.Allocate(36u);
    if (node == 0u) throw std::bad_alloc();
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 32u, 0u);
    LinkAndBalanceTreeNode(tree, node, cursor, parent, entry, nil, runtime);
    runtime.WriteWord(result, node);
}

void ArenaSequenceTree::Next(const std::uint32_t iterator,
                             const std::uint32_t nil,
                             IArenaSequenceRuntime& runtime) {
    auto node = runtime.ReadWord(iterator);
    const auto right = runtime.ReadWord(node + 8u);
    if (right != nil) {
        runtime.WriteWord(iterator,
                          MinimumFromLink(node + 8u, nil, runtime));
        return;
    }
    auto parent = runtime.ReadWord(node + 4u);
    while (node == runtime.ReadWord(parent + 8u)) {
        node = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    if (runtime.ReadWord(node + 8u) != parent)
        runtime.WriteWord(iterator, parent);
    else
        runtime.WriteWord(iterator, node);
}

std::uint32_t ArenaSequenceTree::MinimumFromLink(
    const std::uint32_t link, const std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    auto minimum = link;
    for (auto node = runtime.ReadWord(link); node != nil;
         node = runtime.ReadWord(node))
        minimum = node;
    return minimum;
}

void ArenaSequenceTree::NextAndCapture(
    const std::uint32_t iterator, const std::uint32_t original,
    const std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    const auto previous = runtime.ReadWord(iterator);
    Next(iterator, nil, runtime);
    runtime.WriteWord(original, previous);
}

std::uint32_t ArenaSequenceTree::RotateRight(
    const std::uint32_t tree, const std::uint32_t node,
    const std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node);
    const auto moved = runtime.ReadWord(pivot + 8u);
    runtime.WriteWord(node, moved);
    if (moved != nil) runtime.WriteWord(moved + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent))
        runtime.WriteWord(parent, pivot);
    else
        runtime.WriteWord(parent + 8u, pivot);
    runtime.WriteWord(pivot + 8u, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

std::uint32_t ArenaSequenceTree::RotateLeft(
    const std::uint32_t tree, const std::uint32_t node,
    const std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node + 8u);
    const auto moved = runtime.ReadWord(pivot);
    runtime.WriteWord(node + 8u, moved);
    if (moved != nil) runtime.WriteWord(moved + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent + 8u))
        runtime.WriteWord(parent + 8u, pivot);
    else
        runtime.WriteWord(parent, pivot);
    runtime.WriteWord(pivot, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

void ArenaSequenceTree::Previous(const std::uint32_t iterator,
                                 const std::uint32_t nil,
                                 IArenaSequenceRuntime& runtime) {
    auto node = runtime.ReadWord(iterator);
    const auto parent = runtime.ReadWord(node + 4u);
    if (runtime.ReadWord(node + 32u) == 0u &&
        runtime.ReadWord(parent + 4u) == node) {
        runtime.WriteWord(iterator, runtime.ReadWord(node + 8u));
        return;
    }
    auto left = runtime.ReadWord(node);
    if (left != nil) {
        while (runtime.ReadWord(left + 8u) != nil)
            left = runtime.ReadWord(left + 8u);
        runtime.WriteWord(iterator, left);
        return;
    }
    auto ancestor = parent;
    while (node == runtime.ReadWord(ancestor)) {
        node = ancestor;
        ancestor = runtime.ReadWord(ancestor + 4u);
    }
    runtime.WriteWord(iterator, ancestor);
}

void ArenaSequenceTree::CopyEntry(const std::uint32_t destination,
                                  const std::uint32_t source,
                                  IArenaSequenceRuntime& runtime) {
    if (destination == 0u) return;
    const auto child = runtime.ReadWord(source + 16u);
    runtime.WriteByte(destination, runtime.ReadByte(source));
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    if (destination != source) {
        const auto length = runtime.ReadWord(source + 8u);
        const auto text = runtime.ReadWord(source + 4u);
        if (length != 0u && text != 0u &&
            runtime.ReadByte(text - 1u) < 0xfeu) {
            runtime.WriteWord(destination + 4u, text);
            runtime.WriteWord(destination + 8u, length);
            runtime.WriteWord(destination + 12u,
                              runtime.ReadWord(source + 12u));
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(
                                  runtime.ReadByte(text - 1u) + 1u));
        } else if (length != 0u) {
            const auto allocation = runtime.Allocate(length + 2u);
            if (allocation != 0u) {
                runtime.WriteByte(allocation, 0u);
                for (std::uint32_t i = 0u; i < length; ++i)
                    runtime.WriteByte(allocation + 1u + i,
                                      text == 0u ? 0u :
                                      runtime.ReadByte(text + i));
                runtime.WriteByte(allocation + 1u + length, 0u);
                runtime.WriteWord(destination + 4u, allocation + 1u);
                runtime.WriteWord(destination + 8u, length);
                runtime.WriteWord(destination + 12u, length);
            }
        }
    }
    runtime.WriteWord(destination + 16u, child);
}

void ArenaSequenceTree::FindExact(const std::uint32_t tree,
                                  const std::uint32_t result,
                                  const std::uint32_t key,
                                  const std::uint32_t nil,
                                  IArenaSequenceRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    while (node != nil) {
        if (CompareTreeKeys(node + 12u, key, runtime) >= 0) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    if (candidate != head && CompareTreeKeys(key, candidate + 12u, runtime) >= 0)
        runtime.WriteWord(result, candidate);
    else
        runtime.WriteWord(result, head);
}

void ArenaSequenceTree::EraseRange(const std::uint32_t tree,
                                   const std::uint32_t result,
                                   const std::uint32_t first,
                                   const std::uint32_t last,
                                   const std::uint32_t nil,
                                   IArenaSequenceRuntime& runtime,
                                   const std::uint32_t color_offset,
                                   const bool release_text_record) {
    const auto head = runtime.ReadWord(tree + 4u);
    if (runtime.ReadWord(tree + 12u) != 0u &&
        first == runtime.ReadWord(head) && last == head) {
        auto node = runtime.ReadWord(head + 4u);
        while (node != nil) {
            (void)DeleteSubtree(runtime.ReadWord(node + 8u), nil, runtime,
                                release_text_record);
            const auto left = runtime.ReadWord(node);
            if (release_text_record)
                ArenaSequenceResources::ReleaseText(node + 12u, runtime);
            (void)runtime.Free(node);
            node = left;
        }
        runtime.WriteWord(head + 4u, nil);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.WriteWord(result, head);
        return;
    }

    auto node = first;
    while (node != last) {
        const auto current = node;
        const auto iterator = result;
        runtime.WriteWord(iterator, node);
        Next(iterator, nil, runtime);
        node = runtime.ReadWord(iterator);
        EraseNode(tree, result, current, nil, runtime, color_offset,
                  release_text_record);
    }
    runtime.WriteWord(result, node);
}

void ArenaSequenceTree::EraseNode(const std::uint32_t tree,
                                  const std::uint32_t result,
                                  const std::uint32_t node,
                                  const std::uint32_t nil,
                                  IArenaSequenceRuntime& runtime,
                                  const std::uint32_t color_offset,
                                  const bool release_text_record) {
    const auto head = runtime.ReadWord(tree + 4u);
    runtime.WriteWord(result, node);
    Next(result, nil, runtime);
    const auto successor = runtime.ReadWord(result);
    auto removed = node;
    std::uint32_t replacement;
    const auto left = runtime.ReadWord(node);
    const auto right = runtime.ReadWord(node + 8u);
    if (left == nil) {
        replacement = right;
    } else if (right == nil) {
        replacement = left;
    } else {
        removed = right;
        while (runtime.ReadWord(removed) != nil)
            removed = runtime.ReadWord(removed);
        replacement = runtime.ReadWord(removed + 8u);
    }

    std::uint32_t replacement_parent;
    if (removed != node) {
        runtime.WriteWord(left + 4u, removed);
        runtime.WriteWord(removed, left);
        if (removed == right) {
            replacement_parent = removed;
        } else {
            replacement_parent = runtime.ReadWord(removed + 4u);
            runtime.WriteWord(replacement + 4u, replacement_parent);
            runtime.WriteWord(replacement_parent, replacement);
            runtime.WriteWord(removed + 8u, right);
            runtime.WriteWord(right + 4u, removed);
        }
        const auto parent = runtime.ReadWord(node + 4u);
        runtime.WriteWord(removed + 4u, parent);
        if (node == runtime.ReadWord(head + 4u))
            runtime.WriteWord(head + 4u, removed);
        else if (node == runtime.ReadWord(parent))
            runtime.WriteWord(parent, removed);
        else
            runtime.WriteWord(parent + 8u, removed);
        const auto original_color = runtime.ReadWord(node + color_offset);
        runtime.WriteWord(node + color_offset,
                          runtime.ReadWord(removed + color_offset));
        runtime.WriteWord(removed + color_offset, original_color);
        removed = node;
    } else {
        replacement_parent = runtime.ReadWord(node + 4u);
        runtime.WriteWord(replacement + 4u, replacement_parent);
        if (node == runtime.ReadWord(head + 4u))
            runtime.WriteWord(head + 4u, replacement);
        else if (node == runtime.ReadWord(replacement_parent))
            runtime.WriteWord(replacement_parent, replacement);
        else
            runtime.WriteWord(replacement_parent + 8u, replacement);

        if (node == runtime.ReadWord(head)) {
            if (right == nil) {
                runtime.WriteWord(head, replacement_parent);
            } else {
                auto minimum = replacement;
                while (runtime.ReadWord(minimum) != nil)
                    minimum = runtime.ReadWord(minimum);
                runtime.WriteWord(head, minimum);
            }
        }
        if (node == runtime.ReadWord(head + 8u)) {
            if (left == nil) {
                runtime.WriteWord(head + 8u, replacement_parent);
            } else {
                auto maximum = replacement;
                while (runtime.ReadWord(maximum + 8u) != nil)
                    maximum = runtime.ReadWord(maximum + 8u);
                runtime.WriteWord(head + 8u, maximum);
            }
        }
    }

    if (runtime.ReadWord(removed + color_offset) == 1u) {
        auto current = replacement;
        while (current != runtime.ReadWord(head + 4u) &&
               runtime.ReadWord(current + color_offset) == 1u) {
            const auto parent = replacement_parent;
            if (current == runtime.ReadWord(parent)) {
                auto sibling = runtime.ReadWord(parent + 8u);
                if (runtime.ReadWord(sibling + color_offset) == 0u) {
                    runtime.WriteWord(sibling + color_offset, 1u);
                    runtime.WriteWord(parent + color_offset, 0u);
                    (void)RotateLeft(tree, parent, nil, runtime);
                    sibling = runtime.ReadWord(parent + 8u);
                }
                if (runtime.ReadWord(runtime.ReadWord(sibling) + color_offset) == 1u &&
                    runtime.ReadWord(runtime.ReadWord(sibling + 8u) + color_offset) == 1u) {
                    runtime.WriteWord(sibling + color_offset, 0u);
                    current = parent;
                    replacement_parent = runtime.ReadWord(current + 4u);
                } else {
                    if (runtime.ReadWord(runtime.ReadWord(sibling + 8u) + color_offset) == 1u) {
                        runtime.WriteWord(runtime.ReadWord(sibling) + color_offset, 1u);
                        runtime.WriteWord(sibling + color_offset, 0u);
                        (void)RotateRight(tree, sibling, nil, runtime);
                        sibling = runtime.ReadWord(parent + 8u);
                    }
                    runtime.WriteWord(sibling + color_offset,
                                      runtime.ReadWord(parent + color_offset));
                    runtime.WriteWord(parent + color_offset, 1u);
                    runtime.WriteWord(runtime.ReadWord(sibling + 8u) + color_offset, 1u);
                    (void)RotateLeft(tree, parent, nil, runtime);
                    break;
                }
            } else {
                auto sibling = runtime.ReadWord(parent);
                if (runtime.ReadWord(sibling + color_offset) == 0u) {
                    runtime.WriteWord(sibling + color_offset, 1u);
                    runtime.WriteWord(parent + color_offset, 0u);
                    (void)RotateRight(tree, parent, nil, runtime);
                    sibling = runtime.ReadWord(parent);
                }
                if (runtime.ReadWord(runtime.ReadWord(sibling + 8u) + color_offset) == 1u &&
                    runtime.ReadWord(runtime.ReadWord(sibling) + color_offset) == 1u) {
                    runtime.WriteWord(sibling + color_offset, 0u);
                    current = parent;
                    replacement_parent = runtime.ReadWord(current + 4u);
                } else {
                    if (runtime.ReadWord(runtime.ReadWord(sibling) + color_offset) == 1u) {
                        runtime.WriteWord(runtime.ReadWord(sibling + 8u) + color_offset, 1u);
                        runtime.WriteWord(sibling + color_offset, 0u);
                        (void)RotateLeft(tree, sibling, nil, runtime);
                        sibling = runtime.ReadWord(parent);
                    }
                    runtime.WriteWord(sibling + color_offset,
                                      runtime.ReadWord(parent + color_offset));
                    runtime.WriteWord(parent + color_offset, 1u);
                    runtime.WriteWord(runtime.ReadWord(sibling) + color_offset, 1u);
                    (void)RotateRight(tree, parent, nil, runtime);
                    break;
                }
            }
        }
        runtime.WriteWord(current + color_offset, 1u);
    }
    if (release_text_record)
        ArenaSequenceResources::ReleaseText(removed + 12u, runtime);
    (void)runtime.Free(removed);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) - 1u);
    runtime.WriteWord(result, successor);
}

std::uint32_t ArenaSequenceTree::DeleteSubtree(
    std::uint32_t node, const std::uint32_t nil,
    IArenaSequenceRuntime& runtime, const bool release_text_record) {
    while (node != nil) {
        (void)DeleteSubtree(runtime.ReadWord(node + 8u), nil, runtime,
                            release_text_record);
        const auto left = runtime.ReadWord(node);
        if (release_text_record)
            ArenaSequenceResources::ReleaseText(node + 12u, runtime);
        (void)runtime.Free(node);
        node = left;
    }
    return nil;
}

}  // namespace pal4
