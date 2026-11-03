#include "pal4/xml_parser_node.h"

#include "pal4/compact_string_buffer.h"

namespace pal4 {

std::uint32_t XmlParserNodeRuntimeStub::ReadWord(std::uint32_t) { return 0u; }
void XmlParserNodeRuntimeStub::WriteWord(std::uint32_t, std::uint32_t) {}
std::uint8_t XmlParserNodeRuntimeStub::ReadByte(std::uint32_t) { return 0u; }
void XmlParserNodeRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
std::uint32_t XmlParserNodeRuntimeStub::Free(std::uint32_t) { return 0u; }
std::uint32_t XmlParserNodeRuntimeStub::DeleteNode(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t XmlParserNodeRuntimeStub::StringLength(
    std::uint32_t) { return 0u; }
std::uint32_t XmlParserNodeRuntimeStub::AssignString(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::int32_t XmlParserNodeRuntimeStub::CompareText(
    std::uint32_t, std::uint32_t) { return 0; }
std::uint32_t XmlParserNodeRuntimeStub::CallVirtual(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t XmlParserNodeRuntimeStub::AttributeSetup(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t XmlParserLifecycle::Initialize(
    const std::uint32_t parser, const std::uint32_t source,
    IXmlParserNodeRuntime& runtime) {
    runtime.WriteWord(parser + 8u, 0xffffffffu);
    runtime.WriteWord(parser + 4u, 0xffffffffu);
    runtime.WriteWord(parser + 12u, 0u);
    runtime.WriteWord(parser + 32u, CompactStringBuffer::kEmptyBlock);
    runtime.WriteWord(parser, 0x849970u);
    runtime.WriteWord(parser + 16u, 0u);
    runtime.WriteWord(parser + 20u, source);
    runtime.WriteWord(parser + 24u, 0u);
    runtime.WriteWord(parser + 28u, 0u);
    runtime.WriteWord(parser + 36u, 0u);
    runtime.WriteWord(parser + 40u, 0u);
    return parser;
}

void XmlParserLifecycle::Destroy(const std::uint32_t parser,
                                 IXmlParserNodeRuntime& runtime) {
    runtime.WriteWord(parser, 0x849970u);
    auto node = runtime.ReadWord(parser + 24u);
    while (node != 0u) {
        const auto next = runtime.ReadWord(node + 40u);
        (void)runtime.DeleteNode(node, 1u, 0u);
        node = next;
    }
    const auto block = runtime.ReadWord(parser + 32u);
    if (block != CompactStringBuffer::kEmptyBlock)
        (void)runtime.Free(block);
    runtime.WriteWord(parser, 0x8499b4u);
}

std::uint32_t XmlParserLifecycle::Delete(
    const std::uint32_t parser, const std::uint32_t flags,
    IXmlParserNodeRuntime& runtime) {
    Destroy(parser, runtime);
    if ((flags & 1u) != 0u) (void)runtime.Free(parser);
    return parser;
}

void XmlNode::Initialize(const std::uint32_t node,
                         IXmlParserNodeRuntime& runtime) {
    runtime.WriteWord(node, 0x8499b4u);
}

std::uint32_t XmlNode::SetAttribute(
    const std::uint32_t source, const std::uint32_t destination,
    IXmlParserNodeRuntime& runtime) {
    const auto text = runtime.ReadWord(source + 32u) + 8u;
    (void)runtime.AssignString(destination + 32u, text,
                               runtime.StringLength(text));
    const auto value = runtime.ReadWord(source + 12u);
    runtime.WriteWord(destination + 12u, value);
    return value;
}

std::uint32_t XmlNode::SetAttributeThunk(
    const std::uint32_t source, const std::uint32_t destination,
    IXmlParserNodeRuntime& runtime) {
    return SetAttribute(source, destination, runtime);
}

std::uint8_t XmlNode::SetAttributeWithFlag(
    const std::uint32_t source, const std::uint32_t destination,
    IXmlParserNodeRuntime& runtime) {
    (void)SetAttribute(source, destination, runtime);
    const auto flag = runtime.ReadByte(source + 44u);
    runtime.WriteByte(destination + 44u, flag);
    return flag;
}

std::uint32_t XmlNode::Cleanup(const std::uint32_t node,
                               IXmlParserNodeRuntime& runtime) {
    auto current = runtime.ReadWord(node + 24u);
    std::uint32_t result = 0u;
    while (current != 0u) {
        const auto next = runtime.ReadWord(current + 40u);
        result = runtime.DeleteNode(current, 1u, 0u);
        current = next;
    }
    runtime.WriteWord(node + 24u, 0u);
    runtime.WriteWord(node + 28u, 0u);
    return result;
}

std::uint32_t XmlNode::AddChild(
    const std::uint32_t parent, const std::uint32_t child,
    IXmlParserNodeRuntime& runtime) {
    if (runtime.ReadWord(child + 20u) == 0u) {
        (void)runtime.DeleteNode(child, 1u, 0u);
        if (GetParent(parent, runtime) != 0u) {
            const auto ancestor = GetParent(parent, runtime);
            (void)runtime.AttributeSetup(ancestor, 16u, 0u, 0u, 0u);
        }
        return 0u;
    }
    runtime.WriteWord(child + 16u, parent);
    const auto previous = runtime.ReadWord(parent + 28u);
    runtime.WriteWord(child + 36u, previous);
    runtime.WriteWord(child + 40u, 0u);
    if (previous != 0u)
        runtime.WriteWord(previous + 40u, child);
    else
        runtime.WriteWord(parent + 24u, child);
    runtime.WriteWord(parent + 28u, child);
    return child;
}

namespace {

std::uint32_t FindNamed(const std::uint32_t first,
                        const std::uint32_t name,
                        IXmlParserNodeRuntime& runtime) {
    auto current = first;
    while (current != 0u) {
        const auto block = runtime.ReadWord(current + 32u);
        if (runtime.CompareText(block + 8u, name) == 0) return current;
        current = runtime.ReadWord(current + 40u);
    }
    return 0u;
}

std::uint32_t ResolveNamed(const std::uint32_t first,
                           const std::uint32_t name,
                           IXmlParserNodeRuntime& runtime) {
    auto current = first;
    while (current != 0u) {
        const auto value = runtime.CallVirtual(current, 24u);
        if (value != 0u) return runtime.CallVirtual(current, 24u);
        current = FindNamed(runtime.ReadWord(current + 40u), name, runtime);
    }
    return 0u;
}

}  // namespace

std::uint32_t XmlNode::FindChildByName(
    const std::uint32_t node, const std::uint32_t name,
    IXmlParserNodeRuntime& runtime) {
    return FindNamed(runtime.ReadWord(node + 24u), name, runtime);
}

std::uint32_t XmlNode::FindFollowingSiblingByName(
    const std::uint32_t node, const std::uint32_t name,
    IXmlParserNodeRuntime& runtime) {
    return FindNamed(runtime.ReadWord(node + 40u), name, runtime);
}

std::uint32_t XmlNode::GetFirstChild(
    const std::uint32_t node, IXmlParserNodeRuntime& runtime) {
    auto current = runtime.ReadWord(node + 24u);
    while (current != 0u) {
        if (runtime.CallVirtual(current, 24u) != 0u)
            return runtime.CallVirtual(current, 24u);
        current = runtime.ReadWord(current + 40u);
    }
    return 0u;
}

std::uint32_t XmlNode::GetChildByName(
    const std::uint32_t node, const std::uint32_t name,
    IXmlParserNodeRuntime& runtime) {
    return ResolveNamed(FindChildByName(node, name, runtime), name, runtime);
}

std::uint32_t XmlNode::GetChildByNameFromNext(
    const std::uint32_t node, const std::uint32_t name,
    IXmlParserNodeRuntime& runtime) {
    return ResolveNamed(FindFollowingSiblingByName(node, name, runtime),
                        name, runtime);
}

std::uint32_t XmlNode::GetParent(
    const std::uint32_t node, IXmlParserNodeRuntime& runtime) {
    auto current = node;
    while (current != 0u) {
        if (runtime.CallVirtual(current, 16u) != 0u)
            return runtime.CallVirtual(current, 16u);
        current = runtime.ReadWord(current + 16u);
    }
    return 0u;
}

std::uint32_t XmlNode::GetNextSibling(
    const std::uint32_t node, IXmlParserNodeRuntime& runtime) {
    const auto next = runtime.ReadWord(node + 32u);
    const auto value_block = runtime.ReadWord(next + 24u);
    if (runtime.ReadWord(value_block) == 0u) {
        const auto name_block = runtime.ReadWord(next + 20u);
        if (runtime.ReadWord(name_block) == 0u) return 0u;
    }
    return next;
}

std::uint32_t XmlNode::CallVirtual68(
    const std::uint32_t node, const std::uint32_t first,
    const std::uint32_t second, IXmlVirtualDispatchRuntime& runtime) {
    return runtime.Call(node, 68u, first, second, 0u);
}

}  // namespace pal4
