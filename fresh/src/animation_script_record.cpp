#include "pal4/animation_script_record.h"

namespace pal4 {

namespace {

std::uint32_t ReleaseText(std::uint32_t text,
                          IAnimationScriptRecordRuntime& runtime) {
    if (text == 0) return 0;
    const auto header = text - 1u;
    const auto reference = runtime.ReadReference(header);
    if (reference == 0u || reference == 0xffu)
        return runtime.Free(header);
    runtime.WriteReference(header,
                           static_cast<std::uint8_t>(reference - 1u));
    return header;
}

}  // namespace

std::uint32_t AnimationScriptRecord::Cleanup(
    AnimationScriptRecordState& record,
    IAnimationScriptRecordRuntime& runtime) {
    (void)ReleaseText(record.second_text, runtime);
    record.second_text = 0;
    record.second_length = 0;
    record.second_capacity = 0;

    const auto result = ReleaseText(record.first_text, runtime);
    record.first_text = 0;
    record.first_length = 0;
    record.first_capacity = 0;
    return result;
}

void AnimationScriptRecord::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IAnimationScriptRecordCopyRuntime& runtime) {
    if (destination == 0) return;
    auto& target = runtime.RecordAt(destination);
    auto& original = runtime.RecordAt(source);
    target.first_header = (target.first_header & 0xffffff00u) |
                          (original.first_header & 0xffu);
    target.first_text = 0;
    target.first_length = 0;
    target.first_capacity = 0;
    runtime.AssignString(destination, source);

    target.second_header = (target.second_header & 0xffffff00u) |
                           (original.second_header & 0xffu);
    target.second_text = 0;
    target.second_length = 0;
    target.second_capacity = 0;
    const auto length = original.second_length;
    if (destination == source) {
        runtime.EraseString(destination + 16u, length, 0xffffffffu);
        runtime.EraseString(destination + 16u, 0u, 0u);
        return;
    }

    if (length > 0) {
        const auto source_text = original.second_text != 0
                                     ? original.second_text : runtime.EmptyText();
        if (source_text != 0) {
            const auto reference = runtime.ReadReference(source_text - 1u);
            if (reference < 0xfeu) {
                target.second_text = source_text;
                target.second_length = original.second_length;
                target.second_capacity = original.second_capacity;
                runtime.WriteReference(source_text - 1u,
                                       static_cast<std::uint8_t>(reference + 1u));
                return;
            }
        }
    }

    if (!runtime.AllocateText(destination + 16u, length, true)) return;
    const auto copy_source = original.second_text != 0
                                 ? original.second_text : runtime.EmptyText();
    runtime.CopyText(target.second_text, copy_source, length);
    runtime.SetTextSize(destination + 16u, length);
}

}  // namespace pal4
