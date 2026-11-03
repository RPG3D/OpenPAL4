#include "pal4/xml_parser_document.h"

#include "pal4/compact_string_buffer.h"
#include "pal4/xml_buffer_release.h"

namespace pal4 {

std::uint32_t XmlDocumentRuntimeStub::ReadWord(std::uint32_t) { return 0u; }
void XmlDocumentRuntimeStub::WriteWord(std::uint32_t, std::uint32_t) {}
std::uint8_t XmlDocumentRuntimeStub::ReadByte(std::uint32_t) { return 0u; }
void XmlDocumentRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
std::uint32_t XmlDocumentRuntimeStub::Free(std::uint32_t) { return 0u; }
std::uint32_t XmlDocumentRuntimeStub::DeleteNode(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t XmlDocumentRuntimeStub::StringLength(
    std::uint32_t) { return 0u; }
std::uint32_t XmlDocumentRuntimeStub::AssignString(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::int32_t XmlDocumentRuntimeStub::CompareText(
    std::uint32_t, std::uint32_t) { return 0; }
std::uint32_t XmlDocumentRuntimeStub::CallVirtual(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t XmlDocumentRuntimeStub::AttributeSetup(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t XmlDocumentRuntimeStub::Allocate(std::uint32_t) { return 0u; }
void XmlDocumentRuntimeStub::DestroyAttribute(std::uint32_t) {}
std::uint32_t XmlDocumentRuntimeStub::NextSibling(
    std::uint32_t) { return 0u; }
std::uint32_t XmlDocumentRuntimeStub::CloneChild(
    std::uint32_t, std::uint32_t) { return 0u; }
bool XmlDocumentRuntimeStub::VisitorEnter(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t) { return false; }
bool XmlDocumentRuntimeStub::VisitChild(
    std::uint32_t, std::uint32_t, std::uint32_t) { return false; }
std::uint32_t XmlDocumentRuntimeStub::VisitorLeave(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t XmlParserDocument::Parse(
    const std::uint32_t parser, const std::uint32_t source,
    IXmlDocumentRuntime& runtime) {
    (void)XmlParserLifecycle::Initialize(parser, 1u, runtime);
    try {
        (void)XmlAttributeRoot::Initialize(parser + 44u, runtime);
        try {
            runtime.WriteWord(parser + 28u, 0u);
            runtime.WriteWord(parser + 24u, 0u);
            runtime.WriteWord(parser, 0x8499c0u);
            (void)runtime.AssignString(parser + 32u, source,
                                       runtime.StringLength(source));
        } catch (...) {
            runtime.DestroyAttribute(parser + 44u);
            throw;
        }
    } catch (...) {
        XmlParserLifecycle::Destroy(parser, runtime);
        throw;
    }
    return parser;
}

void XmlParserDocument::Cleanup(
    const std::uint32_t parser, IXmlDocumentRuntime& runtime) {
    runtime.WriteWord(parser, 0x8499c0u);
    (void)CleanupNodes(parser, runtime);
    runtime.DestroyAttribute(parser + 44u);
    XmlParserLifecycle::Destroy(parser, runtime);
}

std::uint32_t XmlParserDocument::CleanupNodes(
    const std::uint32_t parser, IXmlDocumentRuntime& runtime) {
    (void)XmlNode::Cleanup(parser, runtime);
    const auto root = parser + 44u;
    for (;;) {
        const auto child = runtime.ReadWord(root + 32u);
        if (child == root || child == 0u) return child;
        (void)XmlAttributeRoot::RemoveChild(root, child, runtime);
        (void)runtime.DeleteNode(child, 1u, 0u);
    }
}

std::uint32_t XmlParserDocument::GetAttribute(
    const std::uint32_t parser, const std::uint32_t name,
    IXmlDocumentRuntime& runtime) {
    const auto attribute = XmlAttributeRoot::FindByName(
        parser + 44u, name, runtime);
    return attribute == 0u ? 0u :
           runtime.ReadWord(attribute + 24u) + 8u;
}

std::uint32_t XmlParserDocument::SetAttribute(
    const std::uint32_t parser, const std::uint32_t name,
    const std::uint32_t value, IXmlDocumentRuntime& runtime) {
    const auto root = parser + 44u;
    const auto existing = XmlAttributeRoot::FindByName(root, name, runtime);
    if (existing != 0u)
        return runtime.AssignString(existing + 24u, value,
                                    runtime.StringLength(value));

    const auto attribute = runtime.Allocate(36u);
    if (attribute == 0u) {
        const auto parent = XmlNode::GetParent(parser, runtime);
        if (parent == 0u) return 0u;
        const auto result = runtime.AttributeSetup(parent, 3u, 0u, 0u, 0u);
        return (parent & 0xffffff00u) | (result & 0xffu);
    }

    (void)XmlAttributeLifecycle::Initialize(attribute, runtime);
    try {
        (void)runtime.AssignString(attribute + 20u, name,
                                   runtime.StringLength(name));
        (void)runtime.AssignString(attribute + 24u, value,
                                   runtime.StringLength(value));
        runtime.WriteWord(attribute + 16u, 0u);
        runtime.WriteWord(attribute + 32u, 0u);
        runtime.WriteWord(attribute + 28u, 0u);
    } catch (...) {
        runtime.DestroyAttribute(attribute);
        (void)runtime.Free(attribute);
        throw;
    }
    return XmlAttributeRoot::AddChild(root, attribute, runtime);
}

std::uint32_t XmlParserDocument::CloneInto(
    const std::uint32_t source, const std::uint32_t destination,
    IXmlDocumentRuntime& runtime) {
    (void)XmlNode::SetAttribute(source, destination, runtime);
    const auto root = source + 44u;
    auto attribute = runtime.ReadWord(source + 76u);
    std::uint32_t result = root;
    if (attribute != root && attribute != 0u) {
        do {
            const auto value = runtime.ReadWord(attribute + 24u) + 8u;
            const auto name = runtime.ReadWord(attribute + 20u) + 8u;
            (void)SetAttribute(destination, name, value, runtime);
            result = runtime.NextSibling(attribute);
            attribute = result;
        } while (attribute != 0u);
    }
    auto child = runtime.ReadWord(source + 24u);
    while (child != 0u) {
        const auto clone = runtime.CloneChild(child, 60u);
        result = XmlNode::AddChild(destination, clone, runtime);
        child = runtime.ReadWord(child + 40u);
    }
    return result;
}

std::uint32_t XmlAttributeLifecycle::Cleanup(
    const std::uint32_t attribute, IXmlParserNodeRuntime& runtime) {
    (void)XmlBufferRelease::Release(attribute + 24u, runtime);
    const auto result = XmlBufferRelease::Release(attribute + 20u, runtime);
    runtime.WriteWord(attribute, 0x8499b4u);
    return result;
}

std::uint32_t XmlAttributeLifecycle::Initialize(
    const std::uint32_t attribute, IXmlParserNodeRuntime& runtime) {
    runtime.WriteWord(attribute + 8u, 0xffffffffu);
    runtime.WriteWord(attribute + 4u, 0xffffffffu);
    runtime.WriteWord(attribute + 12u, 0u);
    runtime.WriteWord(attribute + 20u, CompactStringBuffer::kEmptyBlock);
    runtime.WriteWord(attribute + 24u, CompactStringBuffer::kEmptyBlock);
    runtime.WriteWord(attribute + 16u, 0u);
    runtime.WriteWord(attribute + 32u, 0u);
    runtime.WriteWord(attribute + 28u, 0u);
    runtime.WriteWord(attribute, 0x849a04u);
    return attribute;
}

std::uint32_t XmlAttributeLifecycle::CleanupAlternate(
    const std::uint32_t attribute, IXmlParserNodeRuntime& runtime) {
    return Cleanup(attribute, runtime);
}

std::uint32_t XmlAttributeRoot::Initialize(
    const std::uint32_t root, IXmlParserNodeRuntime& runtime) {
    runtime.WriteWord(root + 8u, 0xffffffffu);
    runtime.WriteWord(root + 4u, 0xffffffffu);
    runtime.WriteWord(root + 12u, 0u);
    runtime.WriteWord(root + 20u, CompactStringBuffer::kEmptyBlock);
    runtime.WriteWord(root + 24u, CompactStringBuffer::kEmptyBlock);
    runtime.WriteWord(root, 0x849a04u);
    runtime.WriteWord(root + 16u, 0u);
    runtime.WriteWord(root + 32u, root);
    runtime.WriteWord(root + 28u, root);
    return root;
}

std::uint32_t XmlAttributeRoot::AddChild(
    const std::uint32_t root, const std::uint32_t child,
    IXmlParserNodeRuntime& runtime) {
    runtime.WriteWord(child + 32u, root);
    const auto previous = runtime.ReadWord(root + 28u);
    runtime.WriteWord(child + 28u, previous);
    runtime.WriteWord(previous + 32u, child);
    runtime.WriteWord(root + 28u, child);
    return child;
}

std::uint32_t XmlAttributeRoot::RemoveChild(
    const std::uint32_t root, const std::uint32_t child,
    IXmlParserNodeRuntime& runtime) {
    auto current = runtime.ReadWord(root + 32u);
    while (current != root) {
        if (current == child) {
            const auto previous = runtime.ReadWord(current + 28u);
            const auto next = runtime.ReadWord(current + 32u);
            runtime.WriteWord(previous + 32u, next);
            runtime.WriteWord(next + 28u, previous);
            runtime.WriteWord(current + 32u, 0u);
            runtime.WriteWord(current + 28u, 0u);
            return current;
        }
        current = runtime.ReadWord(current + 32u);
    }
    return root;
}

std::uint32_t XmlAttributeRoot::FindByName(
    const std::uint32_t root, const std::uint32_t name,
    IXmlParserNodeRuntime& runtime) {
    auto current = runtime.ReadWord(root + 32u);
    while (current != root) {
        const auto block = runtime.ReadWord(current + 20u);
        if (runtime.CompareText(block + 8u, name) == 0) return current;
        current = runtime.ReadWord(current + 32u);
    }
    return 0u;
}

}  // namespace pal4
