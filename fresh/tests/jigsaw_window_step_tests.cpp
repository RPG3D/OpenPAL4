#include "pal4/jigsaw_window_step.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct FakeEntryRuntime final : pal4::ITimerQueueEntryRuntime {
    std::vector<std::string>& calls;
    std::unordered_map<std::uint32_t, std::uint8_t> counts{
        {0x6001u, 0xffu}, {0x7001u, 0xffu}};
    std::uint32_t released_callback = 0u;
    std::vector<std::uint32_t> freed_names;

    explicit FakeEntryRuntime(std::vector<std::string>& log) : calls(log) {}
    void EraseName(pal4::TimerQueueEntryState&,
                   std::uint32_t, std::uint32_t) override {
        Check(false, "entry name erase is not used");
    }
    void SetTextFlag(pal4::TimerQueueEntryState&,
                     std::uint8_t) override {
        Check(false, "entry text flag set is not used");
    }
    std::uint8_t ReadReferenceCount(std::uint32_t data) override {
        calls.emplace_back("count");
        return counts.at(data);
    }
    void WriteReferenceCount(std::uint32_t data,
                             std::uint8_t count) override {
        calls.emplace_back("decrement");
        counts[data] = count;
    }
    bool AllocateName(pal4::TimerQueueEntryState&,
                      std::uint32_t, std::uint8_t) override {
        Check(false, "entry name allocation is not used");
        return false;
    }
    void CopyName(std::uint32_t, std::uint32_t,
                  std::uint32_t) override {
        Check(false, "entry name copy is not used");
    }
    void SetNameSize(pal4::TimerQueueEntryState&,
                     std::uint32_t) override {
        Check(false, "entry name size is not used");
    }
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(deleting == 1u, "deleting callback flag");
        released_callback = callback;
        calls.emplace_back("release-callback");
    }
    std::uint32_t FreeName(std::uint32_t header) override {
        freed_names.push_back(header);
        calls.emplace_back("free-name");
        return header;
    }
};

struct Runtime final : pal4::IJigsawWindowStepRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t queue = 0x3000u;
    static constexpr std::uint32_t callback = 0x5000u;
    std::vector<std::string> calls;
    FakeEntryRuntime entry_runtime{calls};
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint8_t puzzle_flag = 0u;
    std::uint32_t processed_index = 0u;
    std::uint32_t inserted_step = 0u;
    std::uint32_t inserted_cursor = 0u;
    std::uint32_t inserted_callback = 0u;
    bool inserted_owns_callback = false;
    std::uint8_t name_flag = 0u;
    std::uint32_t allocation = callback;
    bool throw_insert = false;
    bool throw_copy = false;

    void ProcessPuzzleEvent(std::uint32_t puzzle,
                            std::uint32_t index) override {
        Check(puzzle == frame + 0x2cu, "puzzle subobject");
        processed_index = index;
        calls.emplace_back("process");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == frame + 0x78u, "puzzle gate offset");
        calls.emplace_back("gate");
        return puzzle_flag;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        if (address == 0x21a0u) return queue;
        if (address == queue + 4u) return 0x4444u;
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write-word");
    }
    pal4::StringBufferState BuildStepName(
        std::uint8_t flag, std::string_view name) override {
        Check(name == "PuzzleStep", "timer name literal");
        name_flag = flag;
        calls.emplace_back("build-name");
        return {0x7001u, 10u, 10u};
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x2000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x10u, "timer callback allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void CopyFloorAttribute(pal4::TimerQueueEntryState& entry,
                            const pal4::TimerQueueEntryState& source) override {
        Check(source.text_flag == name_flag &&
                  source.name.data == 0x7001u,
              "source timer text");
        entry.text_flag = source.text_flag;
        entry.name = {0x6001u, 10u, 10u};
        calls.emplace_back("copy-entry");
        if (throw_copy) throw std::runtime_error("copy failed");
    }
    void InsertTimer(std::uint32_t target,
                     std::uint32_t step,
                     std::uint32_t cursor,
                     const pal4::TimerQueueEntryState& entry) override {
        Check(target == queue, "timer queue");
        inserted_step = step;
        inserted_cursor = cursor;
        inserted_callback = entry.callback;
        inserted_owns_callback = entry.owns_callback;
        Check(entry.text_flag == name_flag &&
                  entry.name.data == 0x6001u &&
                  entry.initial_period_bits == 0x3f800000u &&
                  entry.current_period_bits == 0x3f800000u,
              "timer entry fields");
        calls.emplace_back("insert");
        if (throw_insert) throw std::runtime_error("insert failed");
    }
    pal4::ITimerQueueEntryRuntime& EntryRuntime() override {
        calls.emplace_back("entry-runtime");
        return entry_runtime;
    }
    std::uint32_t UpdateCounter(std::uint32_t owner) override {
        Check(owner == frame, "counter frame");
        calls.emplace_back("counter");
        return 0xa5u;
    }
};

void CheckNoTimer() {
    Runtime runtime;
    runtime.puzzle_flag = 1u;
    Check(pal4::JigsawWindowStep::SetWithTimer(
              Runtime::frame, 7u, runtime) == 0xa5u &&
              runtime.processed_index == 13u &&
              runtime.calls == (std::vector<std::string>{
                  "process", "gate", "counter"}),
          "active puzzle gate skips timer but updates count");
}

void CheckTimer() {
    Runtime runtime;
    Check(pal4::JigsawWindowStep::SetWithTimer(
              Runtime::frame, 0x1234u, runtime) == 0xa5u &&
              runtime.processed_index == 0x123au &&
              runtime.name_flag == 0x34u &&
              runtime.words.at(Runtime::callback) == 0x843e74u &&
              runtime.words.at(Runtime::callback + 4u) == Runtime::frame &&
              runtime.words.at(Runtime::callback + 8u) == 0x4a0490u &&
              runtime.words.at(Runtime::callback + 0xcu) == 0u &&
              runtime.inserted_step == 0x1234u &&
              runtime.inserted_cursor == 0x4444u &&
              runtime.inserted_callback == Runtime::callback &&
              runtime.inserted_owns_callback &&
              runtime.entry_runtime.released_callback == Runtime::callback &&
              runtime.entry_runtime.freed_names ==
                  (std::vector<std::uint32_t>{0x6000u, 0x7000u}),
          "timer callback and entry fields, insertion and cleanup");
    Check(runtime.calls == (std::vector<std::string>{
              "process", "gate", "build-name", "entry-runtime",
              "manager", "read-word", "allocate", "write-word",
              "write-word", "write-word", "write-word", "copy-entry",
              "read-word", "insert", "release-callback", "count",
              "free-name", "count", "free-name", "counter"}),
          "timer construction and cleanup order");
}

void CheckAllocationAndInsertFailure() {
    Runtime missing;
    missing.allocation = 0u;
    Check(pal4::JigsawWindowStep::SetWithTimer(
              Runtime::frame, 2u, missing) == 0xa5u &&
              missing.inserted_callback == 0u &&
              !missing.inserted_owns_callback &&
              missing.entry_runtime.released_callback == 0u,
          "callback allocation failure still inserts timer");
    Runtime throwing;
    throwing.throw_insert = true;
    bool caught = false;
    try {
        (void)pal4::JigsawWindowStep::SetWithTimer(
            Runtime::frame, 2u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
              throwing.entry_runtime.released_callback == Runtime::callback &&
              throwing.entry_runtime.freed_names ==
                  (std::vector<std::uint32_t>{0x6000u, 0x7000u}) &&
              throwing.calls.back() == "free-name",
          "insertion failure destroys temporary entry and text");
    Runtime copy_failure;
    copy_failure.throw_copy = true;
    caught = false;
    try {
        (void)pal4::JigsawWindowStep::SetWithTimer(
            Runtime::frame, 2u, copy_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
              copy_failure.entry_runtime.released_callback ==
                  Runtime::callback &&
              copy_failure.entry_runtime.freed_names ==
                  (std::vector<std::uint32_t>{0x6000u, 0x7000u}),
          "partial entry copy releases name and callback");
}

}  // namespace

int main() {
    try {
        CheckNoTimer();
        CheckTimer();
        CheckAllocationAndInsertFailure();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
