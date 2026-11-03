#include "pal4/xml_declaration.h"

#include "pal4/compact_string_buffer.h"
#include "pal4/xml_leaf_runtime.h"
#include "pal4/xml_parser_document.h"
#include "pal4/xml_parser_node.h"

#include <initializer_list>
#include <utility>

namespace pal4 {

void XmlDeclaration::Cleanup(const std::uint32_t declaration,
                             IXmlParserNodeRuntime& runtime) {
    runtime.WriteWord(declaration, kVtable);
    for (const auto offset : {52u, 48u, 44u}) {
        const auto block = runtime.ReadWord(declaration + offset);
        if (block != CompactStringBuffer::kEmptyBlock)
            (void)runtime.Free(block);
    }
    XmlParserLifecycle::Destroy(declaration, runtime);
}

int XmlDeclaration::Write(
    const std::uint32_t declaration, std::FILE* stream, const int depth,
    const std::uint32_t output, IXmlDeclarationRuntime& runtime) {
    (void)depth;
    if (stream != nullptr) (void)std::fprintf(stream, "<?xml ");
    if (output != 0u) (void)runtime.Append(output, "<?xml ");

    int result = 0;
    for (const auto& field : {
             std::pair{44u, "version=\""},
             std::pair{48u, "encoding=\""},
             std::pair{52u, "standalone=\""}}) {
        const auto block = runtime.ReadWord(declaration + field.first);
        const auto length = runtime.ReadWord(block);
        if (field.first == 52u) result = static_cast<int>(block);
        if (length == 0u) continue;
        const auto value = runtime.ReadText(block + 8u, length);
        if (stream != nullptr)
            result = std::fprintf(stream, "%s%s\" ", field.second,
                                  value.c_str());
        if (output != 0u) {
            (void)runtime.Append(output, field.second);
            (void)runtime.Append(output, value);
            result = static_cast<int>(runtime.Append(output, "\" "));
        }
    }
    if (stream != nullptr) result = std::fprintf(stream, "?>");
    if (output != 0u)
        result = static_cast<int>(runtime.Append(output, "?>"));
    return result;
}

std::uint32_t XmlDeclaration::CloneInto(
    const std::uint32_t source, const std::uint32_t destination,
    IXmlParserNodeRuntime& runtime) {
    (void)XmlNode::SetAttribute(source, destination, runtime);
    std::uint32_t result = 0u;
    for (const auto offset : {44u, 48u, 52u}) {
        const auto block = runtime.ReadWord(source + offset);
        const auto length = runtime.ReadWord(block);
        result = runtime.AssignString(destination + offset,
                                      block + 8u, length);
    }
    return result;
}

std::uint32_t XmlDeclaration::AcceptVisitor(
    const std::uint32_t declaration, const std::uint32_t visitor,
    IXmlLeafRuntime& runtime) {
    return runtime.CallVisitor(visitor, declaration, 32u);
}

std::uint32_t XmlDeclaration::Create(
    const std::uint32_t source, IXmlDocumentRuntime& runtime) {
    const auto declaration = runtime.Allocate(56u);
    if (declaration == 0u) return 0u;
    (void)XmlParserLifecycle::Initialize(declaration, 5u, runtime);
    for (const auto offset : {44u, 48u, 52u})
        runtime.WriteWord(declaration + offset,
                          CompactStringBuffer::kEmptyBlock);
    runtime.WriteWord(declaration, kVtable);
    try {
        (void)CloneInto(source, declaration, runtime);
    } catch (...) {
        Cleanup(declaration, runtime);
        (void)runtime.Free(declaration);
        throw;
    }
    return declaration;
}

}  // namespace pal4
