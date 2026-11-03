#include "pal4/xml_node_factory.h"

#include "pal4/compact_string_buffer.h"
#include "pal4/xml_character_codec.h"
#include "pal4/xml_document_error.h"
#include "pal4/xml_runtime_text.h"

namespace pal4 {

std::uint32_t XmlNodeFactory::Create(
    const std::uint32_t owner, const std::uint32_t source,
    const int mode, IXmlNodeFactoryRuntime& runtime) {
    const auto text = XmlRuntimeText::SkipWhitespaceAndBom(source, mode, runtime);
    if (text == 0u || runtime.ReadByte(text) != '<') return 0u;
    const auto parent = XmlNode::GetParent(owner, runtime);
    std::uint32_t bytes = 44u;
    std::uint32_t kind = 3u;
    std::uint32_t vtable = 0x849b30u;
    bool declaration = false;
    bool cdata = false;
    bool element = false;
    if (XmlRuntimeText::HasPrefix(text, "<?xml", true, runtime)) {
        bytes = 56u;
        kind = 5u;
        vtable = 0x849ae8u;
        declaration = true;
    } else if (XmlRuntimeText::HasPrefix(text, "<!--", false, runtime)) {
        kind = 2u;
        vtable = 0x849a58u;
    } else if (XmlRuntimeText::HasPrefix(text, "<![CDATA[", false, runtime)) {
        bytes = 48u;
        kind = 4u;
        vtable = 0x849aa0u;
        cdata = true;
    } else if (!XmlRuntimeText::HasPrefix(text, "<!", false, runtime)) {
        const auto after_open = runtime.ReadByte(text + 1u);
        if (XmlCharacterCodec::IsAlphaOrExtended(after_open) != 0 ||
            after_open == '_') {
            bytes = 80u;
            element = true;
        }
    }

    const auto storage = runtime.Allocate(bytes);
    std::uint32_t node = storage;
    if (storage != 0u) {
        if (element) {
            node = runtime.ConstructElement(storage);
        } else {
            (void)XmlParserLifecycle::Initialize(storage, kind, runtime);
            runtime.WriteWord(storage, vtable);
            if (declaration) {
                runtime.WriteWord(storage + 44u, CompactStringBuffer::kEmptyBlock);
                runtime.WriteWord(storage + 48u, CompactStringBuffer::kEmptyBlock);
                runtime.WriteWord(storage + 52u, CompactStringBuffer::kEmptyBlock);
            } else if (cdata) {
                (void)runtime.AssignString(storage + 32u, 0x8dc780u, 0u);
                runtime.WriteByte(storage + 44u, 1u);
            }
        }
    }
    if (node == 0u) {
        if (parent != 0u)
            (void)XmlDocumentError::Set(parent, 3u, 0u, 0u, 0, runtime);
        return 0u;
    }
    runtime.WriteWord(node + 16u, owner);
    return node;
}

}  // namespace pal4
