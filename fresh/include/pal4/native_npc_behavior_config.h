#pragma once
#include <cstdint>

namespace pal4 {
struct BinaryXmlNode;
class NativeEventDocumentRuntime;

// Translates the shared NPC configuration algorithm's output into existing
// native event documents. The source node must outlive the NPC binding.
void LoadNativeNpcBehaviorConfig(std::uint32_t owner,
    const BinaryXmlNode* node, NativeEventDocumentRuntime& documents);
}
