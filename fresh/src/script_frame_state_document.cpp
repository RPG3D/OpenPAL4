#include "pal4/script_frame_state_document.h"

namespace pal4 {

std::uint32_t ScriptFrameStateDocument::Enter(
    IScriptFrameStateRuntime& runtime) {
    const auto document = runtime.InitializeDocument();
    try {
        const auto frame = runtime.ResolveFrame();
        runtime.BeginFrameName("frame");
        ScriptFrameAttributeLookup attribute;
        try {
            attribute = runtime.FindAttribute(document, "frame");
        } catch (...) {
            runtime.EndFrameName();
            throw;
        }
        runtime.EndFrameName();
        if (attribute.position == attribute.end ||
            attribute.position == 0xffffffe4u) {
            const auto created = runtime.CreateFrameAttribute("frame", frame);
            try {
                runtime.AddAttribute(document, created);
            } catch (...) {
                runtime.DestroyAttribute(created);
                throw;
            }
            runtime.DestroyAttribute(created);
        } else {
            runtime.SetExistingAttribute(attribute.position + 0x1cu, frame);
        }
        runtime.InitializeStateAndUi(runtime.Game(), 10u, document);
    } catch (...) {
        (void)runtime.DestroyDocument(document);
        throw;
    }
    return runtime.DestroyDocument(document);
}

}  // namespace pal4
