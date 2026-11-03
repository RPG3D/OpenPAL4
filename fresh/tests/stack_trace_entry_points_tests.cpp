#include "pal4/stack_trace_entry_points.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IStackTraceEntryRuntime {
public:
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
        writes.push_back(address);
    }

    [[nodiscard]] std::uint32_t GenerateStackTraceString(
        const std::uint32_t first_arg,
        const std::uint32_t exception_pointers) override {
        ++generate_calls;
        Check(first_arg == 0x77u && exception_pointers == 0x1000u,
              "stack trace generator receives both original arguments");
        frame_pc_at_generate = Read(pal4::StackTraceEntryPoints::kFrameAddress);
        return generate_result;
    }

    [[nodiscard]] std::uint32_t GetCurrentProcess() override {
        ++current_process_calls;
        return 0x5000u;
    }

    [[nodiscard]] std::uint32_t ReadProcessMemory(
        const std::uint32_t process, const std::uint32_t source,
        const std::uint32_t destination, const std::uint32_t bytes,
        const std::uint32_t bytes_read_pointer) override {
        ++read_memory_calls;
        Check(process == 0x5000u && source == 0x6000u &&
                  destination == 0x7000u && bytes == 32u &&
                  bytes_read_pointer == 0x8000u,
              "read callback passes current process and original buffer fields");
        return read_memory_result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> writes;
    std::uint32_t frame_pc_at_generate = 0u;
    std::uint32_t generate_result = 0x9000u;
    std::uint32_t read_memory_result = 7u;
    int generate_calls = 0;
    int current_process_calls = 0;
    int read_memory_calls = 0;
};

void CheckPreparationAndDelegation() {
    Runtime runtime;
    constexpr std::uint32_t frame = pal4::StackTraceEntryPoints::kFrameAddress;
    for (std::uint32_t offset = 0u;
         offset < pal4::StackTraceEntryPoints::kFrameBytes; offset += 4u) {
        runtime.words[frame + offset] = 0xffffffffu;
    }
    runtime.words[0x1004u] = 0x3000u;
    runtime.words[0x3000u + 184u] = 0x401234u;
    runtime.words[0x3000u + 196u] = 0x2000u;
    runtime.words[0x3000u + 180u] = 0x1000u;
    Check(pal4::StackTraceEntryPoints::Prepare(0x77u, 0x1000u, runtime) ==
              0x9000u && runtime.generate_calls == 1 &&
              runtime.frame_pc_at_generate == 0x401234u,
          "prepare initializes frame before tail-calling generator");
    Check(runtime.Read(frame) == 0x401234u &&
              runtime.Read(frame + 8u) == 3u &&
              runtime.Read(frame + 24u) == 0x1000u &&
              runtime.Read(frame + 32u) == 3u &&
              runtime.Read(frame + 36u) == 0x2000u &&
              runtime.Read(frame + 44u) == 3u,
          "PC, frame and stack offsets and flat modes match IDA");
    Check(runtime.writes.size() == 47u &&
              runtime.writes.front() == frame &&
              runtime.writes[40] == frame + 160u &&
              runtime.Read(frame + 160u) == 0u &&
              runtime.Read(frame + 48u) == 0u,
          "all 41 frame DWORDs clear before six observed fields are written");
}

void CheckForwardAndMemoryCallback() {
    Runtime runtime;
    Check(pal4::StackTraceEntryPoints::Forward(0x77u, 0x1000u, runtime) ==
              0x9000u &&
              runtime.generate_calls == 1 && runtime.writes.empty(),
          "thunk forwards without modifying frame state");
    Check(pal4::StackTraceEntryPoints::ReadCurrentProcessMemory(
              0xdeadbeefu, 0x6000u, 0x7000u, 32u, 0x8000u,
              runtime) == 7u &&
              runtime.current_process_calls == 1 &&
              runtime.read_memory_calls == 1,
          "read callback ignores caller process and propagates API result");
    runtime.read_memory_result = 0u;
    Check(pal4::StackTraceEntryPoints::ReadCurrentProcessMemory(
              0u, 0x6000u, 0x7000u, 32u, 0x8000u,
              runtime) == 0u,
          "read failure is returned without conversion");
}

}  // namespace

int main() {
    CheckPreparationAndDelegation();
    CheckForwardAndMemoryCallback();
    std::cout << "2 stack trace entry point test cases passed\n";
}
