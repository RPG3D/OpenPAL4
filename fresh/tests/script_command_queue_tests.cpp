#include "pal4/script_command_queue.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptCommandRuntime {
    pal4::ScriptCommandQueue queue;
    std::size_t accesses = 0;
    pal4::ScriptCommandQueue& ScriptExecutorQueue() override {
        ++accesses;
        return queue;
    }
};

void AppendAndGrow() {
    Runtime runtime;
    const std::vector<std::size_t> expected_capacity = {
        1u, 2u, 4u, 4u, 8u, 8u, 8u, 8u, 16u};
    for (std::uint32_t i = 0; i < expected_capacity.size(); ++i) {
        const auto* end = pal4::ScriptCommandDispatcher::Push(
            0x80000000u + i, 0xffffffffu - i, runtime);
        const auto& commands = runtime.queue.Commands();
        Check(end == commands.data() + commands.size() &&
              commands.size() == static_cast<std::size_t>(i + 1u) &&
              runtime.queue.Capacity() == expected_capacity[i] &&
              runtime.accesses == static_cast<std::size_t>(i + 1u),
              "end pointer, singleton count and doubling capacity");
        for (std::uint32_t j = 0; j <= i; ++j) {
            Check(commands[j].command == 0x80000000u + j &&
                  commands[j].argument == 0xffffffffu - j &&
                  commands[j].reserved == 0u,
                  "reallocation preserves all prior 12-byte records");
        }
    }
}

void IndependentQueues() {
    Runtime first;
    Runtime second;
    (void)pal4::ScriptCommandDispatcher::Push(1u, 2u, first);
    (void)pal4::ScriptCommandDispatcher::Push(3u, 4u, second);
    Check(first.queue.Commands().size() == 1u &&
          second.queue.Commands().size() == 1u &&
          first.queue.Commands()[0].command == 1u &&
          second.queue.Commands()[0].command == 3u,
          "runtime selects its own script executor queue");
}

}  // namespace

int main() {
    try {
        AppendAndGrow();
        IndependentQueues();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
