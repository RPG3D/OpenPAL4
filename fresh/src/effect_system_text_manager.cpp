#include "pal4/effect_system_text_manager.h"

#include <algorithm>
#include <cctype>

namespace pal4 {

std::uint32_t EffectSystemTextManager::Construct(
    std::uint32_t manager, std::uint8_t initial_byte,
    IEffectSystemTextManagerRuntime& runtime) {
    runtime.WriteByte(manager, initial_byte);
    runtime.WriteByte(manager + 1u, initial_byte);
    runtime.WriteByte(manager + 8u, 0u);

    const auto candidate = runtime.AllocateSharedNil();
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 32u, 1u);
    if (runtime.SharedNil() == 0u) {
        runtime.SetSharedNil(candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    }
    runtime.SetSharedNilReferences(runtime.SharedNilReferences() + 1u);
    if (candidate != runtime.SharedNil()) runtime.FreeSharedNil(candidate);

    std::uint32_t head{};
    try { head = runtime.Allocate(36u); }
    catch (...) {
        const auto references = runtime.SharedNilReferences() - 1u;
        runtime.SetSharedNilReferences(references);
        if (references == 0u) {
            const auto nil = runtime.SharedNil();
            runtime.SetSharedNil(0u);
            runtime.FreeSharedNil(nil);
        }
        throw;
    }
    runtime.WriteWord(head + 4u, runtime.SharedNil());
    runtime.WriteWord(head + 32u, 0u);
    runtime.WriteWord(manager + 4u, head);
    runtime.WriteWord(manager + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    return manager;
}

std::uint32_t EffectSystemTextManager::Destruct(
    std::uint32_t manager,
    IEffectSystemTextManagerCleanupRuntime& runtime) {
    runtime.ClearTextElements(manager);
    const auto head = runtime.ReadWord(manager + 4u);
    auto node = runtime.ReadWord(head);
    if (runtime.ReadWord(manager + 12u) != 0u) {
        runtime.DeleteSubtree(runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(manager + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.GetFirstElement(manager);
    } else {
        while (node != head) {
            const auto current = node;
            node = runtime.Next(node);
            runtime.EraseNode(manager, current);
        }
    }
    (void)runtime.Free(head);
    runtime.WriteWord(manager + 4u, 0u);
    runtime.WriteWord(manager + 12u, 0u);
    const auto references = runtime.SharedNilReferences() - 1u;
    runtime.SetSharedNilReferences(references);
    if (references != 0u) return references;
    const auto nil = runtime.SharedNil();
    runtime.SetSharedNil(0u);
    return nil != 0u ? runtime.FreeSharedNil(nil) : 0u;
}

bool EffectSystemTextManager::Contains(
    std::uint32_t manager, std::string_view name,
    IEffectSystemTextManagerLookupRuntime& runtime) {
    const auto head = runtime.ReadWord(manager + 4u);
    const auto node = runtime.LowerBound(manager, name);
    return node != head && runtime.CompareName(name, node + 12u) >= 0;
}

std::uint32_t EffectSystemTextManager::GetProperty(
    std::uint32_t manager, std::string_view name,
    IEffectSystemTextManagerLookupRuntime& runtime) {
    const auto head = runtime.ReadWord(manager + 4u);
    const auto node = runtime.LowerBound(manager, name);
    if (node == head || runtime.CompareName(name, node + 12u) < 0)
        return 0u;
    return runtime.ReadWord(node + 28u);
}

std::uint32_t EffectSystemTextManager::ClearElements(
    std::uint32_t manager,
    IEffectSystemTextManagerClearRuntime& runtime) {
    const auto count = runtime.ReadWord(manager + 12u);
    if (count == 0u) return 0u;

    const auto head = runtime.ReadWord(manager + 4u);
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.Next(node)) {
        const auto property = runtime.ReadWord(node + 28u);
        if (property != 0u) {
            runtime.CleanupProperty(property);
            runtime.WriteWord(node + 28u, 0u);
        }
    }

    if (runtime.ReadWord(manager + 12u) != 0u) {
        auto node = runtime.ReadWord(head + 4u);
        while (node != runtime.SharedNil()) {
            runtime.DeleteSubtree(runtime.ReadWord(node + 8u));
            const auto left = runtime.ReadWord(node);
            runtime.ReleaseText(node + 12u);
            runtime.FreeNode(node);
            node = left;
        }
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(manager + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        return head;
    }
    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto current = node;
        node = runtime.Next(node);
        runtime.EraseNode(manager, current);
    }
    return head;
}

bool EffectSystemTextManager::AddEntry(
    std::uint32_t manager, std::string_view name,
    std::uint32_t source_xml,
    IEffectSystemTextManagerEntryRuntime& runtime) {
    if (Contains(manager, name, runtime)) return false;

    const auto clone = runtime.CloneXmlNode(source_xml);
    if (clone == 0u) return false;
    EffectSystemTextInsertResult inserted;
    try {
        inserted = runtime.Insert(manager, name, clone);
    } catch (...) {
        runtime.DestroyXmlNode(clone);
        throw;
    }
    if (!inserted.inserted) {
        runtime.DestroyXmlNode(clone);
        return false;
    }

    const auto xml_node = runtime.ReadWord(inserted.node + 28u);
    constexpr std::string_view kLabelAttribute = "Attr_Label";
    const auto attribute = runtime.FindAttribute(xml_node, kLabelAttribute);
    if (attribute) {
        runtime.SetAttribute(*attribute, name);
    } else {
        runtime.AddAttribute(xml_node, kLabelAttribute, name);
    }
    return true;
}

bool EffectSystemTextManager::LoadFromXml(
    std::uint32_t manager, std::uint32_t xml_node,
    IEffectSystemTextManagerXmlRuntime& runtime) {
    runtime.ClearElements(manager);
    if (xml_node == 0u) return false;
    constexpr std::string_view kRootName = "OxnList";
    const auto actual = runtime.NodeName(xml_node);
    if (actual.size() != kRootName.size() ||
        !std::equal(actual.begin(), actual.end(), kRootName.begin(),
                    [](unsigned char left, unsigned char right) {
                        return std::tolower(left) == std::tolower(right);
                    }))
        return false;

    const auto count = runtime.ChildCount(xml_node);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto child = runtime.ChildAt(
            xml_node, static_cast<std::uint32_t>(index));
        const auto name = runtime.ReadAttribute(child, "Attr_Label");
        if (!runtime.AddEntry(manager, name, child)) return false;
    }
    return true;
}

}  // namespace pal4
