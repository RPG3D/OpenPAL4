#include "pal4/ai_action_task.h"

namespace pal4 {

std::uint32_t AiActionEventLogger::Log(
    const std::uint32_t source, const char* format,
    const std::vector<std::uint32_t>& arguments,
    IAiActionEventLoggerRuntime& runtime) {
    if (format != nullptr)
        runtime.FormatToGlobalBuffer(kMessageBuffer, format, arguments);
    const auto key = runtime.ReadWord(kEventKeyPointer);
    return runtime.DispatchEvent(key, source, kMessageBuffer, 3u);
}

}  // namespace pal4
