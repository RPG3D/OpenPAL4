#include "pal4/script_command_queue.h"

#include <algorithm>

namespace pal4 {

ScriptCommandRecord* ScriptCommandQueue::Push(
    std::uint32_t command, std::uint32_t argument) {
    if (commands_.size() == commands_.capacity()) {
        const auto size = commands_.size();
        commands_.reserve(size + std::max(size, std::size_t{1}));
    }
    commands_.push_back({command, argument, 0u});
    return commands_.data() + commands_.size();
}

ScriptCommandRecord* ScriptCommandDispatcher::Push(
    std::uint32_t command, std::uint32_t argument,
    IScriptCommandRuntime& runtime) {
    return runtime.ScriptExecutorQueue().Push(command, argument);
}

}  // namespace pal4
