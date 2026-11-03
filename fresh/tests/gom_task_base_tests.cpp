#include "pal4/gom_task_base.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IGomTaskBaseRuntime {
public:
    const pal4::GomTaskBaseState* state = nullptr;
    std::vector<std::uint32_t> frees;
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::string copied_name;
    std::uint32_t allocation = 0x8000u;
    std::uint32_t allocated_bytes = 0u;
    bool check_destroy_order = true;
    bool throw_on_copy = false;

    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocated_bytes = bytes;
        events.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        if (check_destroy_order) {
            Check(state != nullptr &&
                      state->vtable == pal4::GomTaskBase::kVtable,
                  "base vtable restored before free");
            if (frees.empty())
                Check(state->previous == 0x3000u &&
                          state->next == 0x4000u &&
                          state->context == 0x5000u,
                      "name is freed before links are cleared");
            else
                Check(state->previous == 0u && state->next == 0u &&
                          state->context == 0u,
                      "object is freed after links are cleared");
        }
        frees.push_back(address);
        events.emplace_back("free");
        return 0x77u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void CopyCString(std::uint32_t destination, const char* source,
                     std::uint32_t bytes) override {
        Check(destination == allocation && bytes == allocated_bytes,
              "copy uses exact allocated destination and byte count");
        events.emplace_back("copy");
        if (throw_on_copy) throw std::runtime_error("copy failed");
        copied_name.assign(source, bytes);
    }
};

void Cases() {
    Runtime runtime;
    pal4::GomTaskBase base(runtime);
    pal4::GomTaskBaseState state;
    std::memset(&state, 0xaa, sizeof(state));
    runtime.state = &state;
    Check(&base.Construct(state) == &state &&
              state.vtable == pal4::GomTaskBase::kVtable &&
              state.active == 0u && state.name == 0u &&
              state.wait_mode == 0u && state.wait_time == 0.0f &&
              state.expect_time == 0.0f &&
              state.total_elapsed == 0.0f &&
              state.wait_elapsed == 0.0f &&
              state.run_elapsed == 0.0f &&
              state.finish_latch == 0.0f && state.phase == 0u &&
              state.reserved == 0u && state.previous == 0u &&
              state.next == 0u && state.context == 0u &&
              state.padding ==
                  std::array<std::uint8_t, 3>{0xaau, 0xaau, 0xaau},
          "constructor clears observed fields and preserves byte padding");

    state.vtable = 0xdeadbeefu;
    state.name = 0x2000u;
    state.previous = 0x3000u;
    state.next = 0x4000u;
    state.context = 0x5000u;
    state.phase = 3u;
    state.run_elapsed = 9.0f;
    Check(base.Destroy(state) == 0x77u &&
              runtime.frees == std::vector<std::uint32_t>{0x2000u} &&
              state.name == 0u && state.previous == 0u &&
              state.next == 0u && state.context == 0u &&
              state.phase == 3u && state.run_elapsed == 9.0f,
          "destructor frees name, clears links and preserves task timing");
    Check(base.Destroy(state) == 0u && runtime.frees.size() == 1u,
          "null name does not call allocator");

    runtime.frees.clear();
    state.name = 0x2000u;
    state.previous = 0x3000u;
    state.next = 0x4000u;
    state.context = 0x5000u;
    Check(base.DestroyDeleting(state, 0x1000u, 2u) == 0x1000u &&
              runtime.frees == std::vector<std::uint32_t>{0x2000u},
          "high deletion flag does not free outer task");
    runtime.frees.clear();
    state.name = 0x2000u;
    state.previous = 0x3000u;
    state.next = 0x4000u;
    state.context = 0x5000u;
    Check(base.DestroyDeleting(state, 0x1000u, 3u) == 0x1000u &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x2000u, 0x1000u},
          "low deletion flag frees object after its name");

    pal4::GomTaskBaseRuntimeStub stub;
    pal4::GomTaskBase stub_base(stub);
    pal4::GomTaskBaseState empty{};
    Check(&stub_base.Construct(empty) == &empty &&
              stub_base.Destroy(empty) == 0u,
          "native allocator stub links");
}

void TimingAndLinks() {
    Runtime runtime;
    runtime.check_destroy_order = false;
    pal4::GomTaskBase base(runtime);
    pal4::GomTaskBaseState state{};
    state.vtable = pal4::GomTaskBase::kVtable;
    state.name = 0x7000u;
    state.phase = 3u;
    state.reserved = 0x1234u;
    state.total_elapsed = 8.0f;
    state.wait_elapsed = 9.0f;
    state.run_elapsed = 10.0f;
    state.finish_latch = 11.0f;
    pal4::GomTaskTimingConfig timing{2u, 1.25f, 3.5f};
    Check(base.InitializeTimingAndLinks(
              state, "Wait", timing, 0x1000u, 0x2000u, 0x3000u) ==
              0x2000u &&
              runtime.events ==
                  std::vector<std::string>{"free", "allocate", "copy"} &&
              runtime.frees == std::vector<std::uint32_t>{0x7000u} &&
              runtime.allocated_bytes == 5u &&
              runtime.copied_name == std::string("Wait\0", 5) &&
              state.name == runtime.allocation && state.wait_mode == 2u &&
              state.wait_time == 1.25f && state.expect_time == 3.5f &&
              state.previous == 0x1000u && state.next == 0x2000u &&
              state.context == 0x3000u && state.total_elapsed == 0.0f &&
              state.wait_elapsed == 0.0f && state.run_elapsed == 0.0f &&
              state.finish_latch == 0.0f && state.active == 1u &&
              state.phase == 3u && state.reserved == 0x1234u,
          "initializer frees old name, copies new text and sets timing/links");

    runtime.events.clear();
    state.name = 0u;
    Check(base.InitializeTimingAndLinks(
              state, "", timing, 0u, 0u, 0u) == 0u &&
              runtime.events ==
                  std::vector<std::string>{"allocate", "copy"} &&
              runtime.allocated_bytes == 1u &&
              runtime.copied_name == std::string("\0", 1),
          "empty name allocates and copies the terminator");

    runtime.events.clear();
    runtime.allocation = 0u;
    state.name = 0x7000u;
    state.wait_mode = 7u;
    bool threw = false;
    try {
        static_cast<void>(base.InitializeTimingAndLinks(
            state, "Failure", timing, 1u, 2u, 3u));
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && state.name == 0u && state.wait_mode == 7u &&
              runtime.events ==
                  std::vector<std::string>{"free", "allocate"},
          "allocation failure follows old-name release and stops before copy");

    runtime.events.clear();
    runtime.allocation = 0x9000u;
    runtime.throw_on_copy = true;
    threw = false;
    try {
        static_cast<void>(base.InitializeTimingAndLinks(
            state, "Copy", timing, 1u, 2u, 3u));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.name == 0x9000u && state.wait_mode == 7u &&
              runtime.events ==
                  std::vector<std::string>{"allocate", "copy"},
          "copy failure leaves the new name published before timing writes");
}

void ElapsedTime() {
    Runtime runtime;
    pal4::GomTaskBase base(runtime);
    pal4::GomTaskBaseState state{};
    state.total_elapsed = 2.0f;
    state.wait_elapsed = 3.0f;
    state.run_elapsed = 4.0f;
    state.finish_latch = -9.0f;
    base.AccumulateTotal(state, 0.5f);
    Check(state.total_elapsed == 2.5f && state.wait_elapsed == 3.0f &&
              state.run_elapsed == 4.0f && state.finish_latch == -9.0f,
          "total accumulator changes only offset 0x18");
    base.AccumulateWait(state, -1.25f);
    Check(state.total_elapsed == 2.5f && state.wait_elapsed == 1.75f &&
              state.run_elapsed == 4.0f,
          "wait accumulator changes only offset 0x1c");
    base.AccumulateRun(state, 3.5f);
    Check(state.wait_elapsed == 1.75f && state.run_elapsed == 7.5f,
          "run accumulator changes only offset 0x20");
    Check(base.LatchFinishTime(state) ==
              std::bit_cast<std::uint32_t>(2.5f) &&
              state.finish_latch == 2.5f,
          "finish latch copies total elapsed and returns its raw word");
    state.total_elapsed = std::bit_cast<float>(0x7fc01234u);
    Check(base.LatchFinishTime(state) == 0x7fc01234u &&
              std::bit_cast<std::uint32_t>(state.finish_latch) == 0x7fc01234u,
          "finish latch preserves NaN payload bits");
}

void CompletionPredicates() {
    Runtime runtime;
    pal4::GomTaskBase base(runtime);
    pal4::GomTaskBaseState state{};
    state.wait_time = 2.0f;
    state.wait_elapsed = 2.0f;
    state.phase = 1u;
    state.wait_mode = 3u;
    Check(!base.IsWaitComplete(state), "non-wait phase blocks every mode");
    state.phase = 2u;
    Check(base.IsWaitComplete(state), "mode 3 completes immediately");
    state.wait_mode = 0u;
    Check(!base.IsWaitComplete(state), "unknown mode does not complete");
    state.wait_mode = 2u;
    Check(base.IsWaitComplete(state), "mode 2 accepts equality");
    state.wait_elapsed = 1.0f;
    Check(!base.IsWaitComplete(state), "mode 2 rejects shorter wait");
    state.wait_mode = 1u;
    state.wait_elapsed = 2.0f;
    Check(base.IsWaitComplete(state) && runtime.reads.empty(),
          "mode 1 without predecessor uses inclusive wait time");

    state.previous = 0x1000u;
    runtime.words[0x1028u] = 3u;
    Check(!base.IsWaitComplete(state) &&
              runtime.reads == std::vector<std::uint32_t>{0x1028u},
          "incomplete predecessor blocks latch read");
    runtime.reads.clear();
    runtime.words[0x1028u] = 4u;
    runtime.words[0x1024u] = std::bit_cast<std::uint32_t>(1.0f);
    state.wait_elapsed = 3.0f;
    Check(!base.IsWaitComplete(state),
          "predecessor branch rejects equal elapsed difference");
    state.wait_elapsed = 3.25f;
    Check(base.IsWaitComplete(state) &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1028u, 0x1024u, 0x1028u, 0x1024u},
          "completed predecessor reads phase then latch and uses strict comparison");
    state.wait_elapsed = std::numeric_limits<float>::quiet_NaN();
    Check(!base.IsWaitComplete(state), "NaN elapsed wait never completes");
    state.wait_elapsed = 4.0f;
    state.wait_time = std::numeric_limits<float>::quiet_NaN();
    Check(!base.IsWaitComplete(state), "NaN wait threshold never completes");
    state.wait_mode = 2u;
    Check(!base.IsWaitComplete(state), "mode 2 rejects NaN threshold");
    state.wait_time = 2.0f;
    state.wait_elapsed = std::numeric_limits<float>::infinity();
    Check(base.IsWaitComplete(state), "infinite elapsed time exceeds finite wait");

    state.expect_time = -1.0f;
    state.run_elapsed = 100.0f;
    Check(!pal4::GomTaskBase::IsDurationComplete(state),
          "negative duration never auto-completes");
    state.expect_time = 0.0f;
    state.run_elapsed = 0.0f;
    Check(pal4::GomTaskBase::IsDurationComplete(state),
          "zero duration completes at equality without phase gate");
    state.expect_time = 2.0f;
    state.run_elapsed = 1.0f;
    Check(!pal4::GomTaskBase::IsDurationComplete(state),
          "short run does not complete");
    state.run_elapsed = 2.0f;
    Check(pal4::GomTaskBase::IsDurationComplete(state),
          "duration accepts equality");
    state.expect_time = std::numeric_limits<float>::quiet_NaN();
    Check(!pal4::GomTaskBase::IsDurationComplete(state),
          "NaN expected duration does not complete");
    state.expect_time = 2.0f;
    state.run_elapsed = std::numeric_limits<float>::quiet_NaN();
    Check(!pal4::GomTaskBase::IsDurationComplete(state),
          "NaN run duration does not complete");
    state.run_elapsed = std::numeric_limits<float>::infinity();
    Check(pal4::GomTaskBase::IsDurationComplete(state),
          "infinite run duration exceeds finite expectation");
    Check(!pal4::GomTaskBase::ReturnFalse(),
          "default task predicate returns false");
}

}  // namespace

int main() {
    try {
        Cases();
        TimingAndLinks();
        ElapsedTime();
        CompletionPredicates();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
