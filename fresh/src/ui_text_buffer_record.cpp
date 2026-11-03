#include "pal4/ui_text_buffer_record.h"

namespace pal4 {

void UiTextBufferRecord::InitializeAndCopy(
    const std::uint32_t destination, const std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    if (destination == 0u) return;

    InitializeTextOnly(destination, source, runtime);
    runtime.WriteWord(destination + 16u,
                      runtime.ReadWord(source + 16u));
}

void UiTextBufferRecord::InitializeTextOnly(
    const std::uint32_t destination, const std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    const auto flag = runtime.ReadByte(source);
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteByte(destination, flag);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    const auto length = runtime.ReadWord(source + 8u);

    if (destination == source) {
        if (length != 0u) runtime.ThrowRange();
        runtime.DetachAndReset(destination);
        const auto remaining = runtime.ReadWord(destination + 8u);
        if (remaining != 0u) {
            const auto offset = remaining == 0xffffffffu
                                    ? 0xffffffffu : remaining;
            const auto data = runtime.ReadWord(destination + 4u);
            runtime.CopyBytes(data + offset, data + offset,
                              remaining - offset);
            const auto new_length = remaining - offset;
            if (runtime.AllocateText(destination, new_length, 0u))
                runtime.SetTextSize(destination, new_length);
        }
        runtime.DetachAndReset(destination);
    } else {
        auto shared = false;
        if (length != 0u) {
            auto data = runtime.ReadWord(source + 4u);
            if (data == 0u) data = runtime.EmptyText();
            if (runtime.ReadByte(data - 1u) < 0xfeu) {
                runtime.SetErrorFlag(destination, 1u);
                data = runtime.ReadWord(source + 4u);
                if (data == 0u) data = runtime.EmptyText();
                runtime.WriteWord(destination + 4u, data);
                runtime.WriteWord(destination + 8u,
                                  runtime.ReadWord(source + 8u));
                runtime.WriteWord(destination + 12u,
                                  runtime.ReadWord(source + 12u));
                runtime.WriteByte(data - 1u,
                                  static_cast<std::uint8_t>(
                                      runtime.ReadByte(data - 1u) + 1u));
                shared = true;
            }
        }
        if (!shared && runtime.AllocateText(destination, length, 1u)) {
            auto data = runtime.ReadWord(source + 4u);
            if (data == 0u) data = runtime.EmptyText();
            const auto target = runtime.ReadWord(destination + 4u);
            runtime.CopyBytes(target, data, length);
            runtime.WriteWord(destination + 8u, length);
            runtime.WriteByte(target + length, 0u);
        }
    }
}

}  // namespace pal4
