#include "pal4/timed_event_queue.h"
#include "pal4/poetry_resume_dialog.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITimedEventQueueRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocations{0x3000u, 0x4000u};
    std::vector<std::string> calls;
    pal4::TimedQueueRecord copied{};
    std::uint32_t copied_to = 0;
    std::uint32_t name_source = 0;
    bool transfer_callback = true;
    bool has_name_data = true;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        Check(!allocations.empty(), "unexpected allocation");
        const auto address = allocations.front();
        allocations.erase(allocations.begin());
        return address;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void InitializeName(pal4::TimedQueueRecord& record,
                        std::uint32_t source) override {
        calls.emplace_back("name");
        name_source = source;
        record.name_data = has_name_data ? 0x5000u : 0u;
        record.name_length = 4u;
    }
    void CopyRecord(std::uint32_t destination,
                    pal4::TimedQueueRecord& source) override {
        calls.emplace_back("copy");
        copied_to = destination;
        copied = source;
        if (transfer_callback) source.owns_callback = 0u;
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flag) override {
        calls.emplace_back("delete");
        Check(callback == 0x3000u && flag == 1u,
              "callback destructor arguments");
    }
    void ReleaseName(pal4::TimedQueueRecord& record) override {
        calls.emplace_back("release");
        Check(record.name_data == 0x5000u, "temporary name release");
    }
    std::uint32_t ReleaseNameResult(
        pal4::TimedQueueRecord& record) override {
        Check(record.name_data == 0x5000u,
              "existing-callback temporary name release");
        calls.emplace_back("release-result");
        return 0x88u;
    }
};

void CheckInsert(std::uint32_t successor, bool transfer) {
    Runtime runtime;
    runtime.transfer_callback = transfer;
    runtime.words[0x1010u] = 0x2000u;
    runtime.words[0x2004u] = successor;
    runtime.words[0x1014u] = 8u;
    pal4::TimedEventQueue::Add(0x1000u, 0x6000u, 37u, 0x7000u,
                               0x8000u, runtime);
    Check(runtime.words[0x3008u] == 0x8000u &&
          runtime.words[0x3000u] ==
              pal4::TimedEventQueue::kCallbackVtable &&
          runtime.words[0x3004u] == 0x7000u,
          "callback record field order and values");
    Check(runtime.words[0x4000u] == (successor ? 0x2000u : 0x4000u) &&
          runtime.words[0x4004u] == (successor ? successor : 0x4000u) &&
          runtime.words[0x2004u] == 0x4000u &&
          runtime.words[successor ? successor : 0x4000u] == 0x4000u,
          "list insertion links");
    Check(runtime.words[0x1014u] == 9u, "queue count increment");
    Check(runtime.copied_to == 0x4008u &&
          runtime.copied.delay == 37u &&
          runtime.copied.callback == 0x3000u &&
          runtime.copied.owns_callback == 1u &&
          runtime.name_source == 0x6000u,
          "copied record fields");
    Check(runtime.calls.back() == "release", "temporary name cleanup last");
    const auto preceding = runtime.calls[runtime.calls.size() - 2u];
    Check(preceding == (transfer ? "write:4116" : "delete"),
          "callback cleanup follows count increment");
    Check((std::find(runtime.calls.begin(), runtime.calls.end(), "delete") !=
           runtime.calls.end()) == !transfer,
          "transferred callback is not deleted");
}

void CheckGates() {
    Runtime no_callback;
    pal4::TimedEventQueue::Add(0x1000u, 0x6000u, 37u, 0x7000u,
                               0u, no_callback);
    Check(no_callback.calls.empty(), "null callback leaves queue untouched");

    Runtime failed_callback;
    failed_callback.allocations = {0u, 0x4000u};
    failed_callback.has_name_data = false;
    failed_callback.words[0x1010u] = 0x2000u;
    pal4::TimedEventQueue::Add(0x1000u, 0x6000u, 37u, 0x7000u,
                               0x8000u, failed_callback);
    Check(failed_callback.copied.callback == 0u &&
          failed_callback.copied.owns_callback == 0u,
          "failed callback allocation still inserts empty record");
    Check(std::find(failed_callback.calls.begin(), failed_callback.calls.end(),
                    "release") == failed_callback.calls.end(),
          "null temporary name requires no release");
}

void CheckPoetryResumeCallback() {
    Runtime runtime;
    runtime.words[0x1010u] = 0x2000u;
    runtime.words[0x2004u] = 0u;
    runtime.words[0x1014u] = 2u;
    pal4::PoetryResumeDialog::RegisterCallback(
        0x1000u, 0x6000u, 0x3fc00000u, 0x7000u, 0x8000u,
        runtime);
    Check(runtime.words[0x3000u] == 0x844244u &&
          runtime.words[0x3004u] == 0x7000u &&
          runtime.words[0x3008u] == 0x8000u,
          "PoetryResume callback vtable and owner");
    Check(runtime.words[0x4000u] == 0x4000u &&
          runtime.words[0x4004u] == 0x4000u &&
          runtime.words[0x2004u] == 0x4000u &&
          runtime.words[0x1014u] == 3u,
          "PoetryResume list insertion");
    Check(runtime.copied.delay == 0x3fc00000u &&
          runtime.copied.callback == 0x3000u &&
          runtime.copied.owns_callback == 1u &&
          runtime.name_source == 0x6000u,
          "PoetryResume record transfer");

    Runtime no_callback;
    pal4::PoetryResumeDialog::RegisterCallback(
        0x1000u, 0x6000u, 0x40000000u, 0x7000u, 0u,
        no_callback);
    Check(no_callback.calls.empty(), "PoetryResume null callback gate");
}

void CheckExistingCallback() {
    for (const bool transfer : {true, false}) {
        Runtime runtime;
        runtime.transfer_callback = transfer;
        runtime.allocations = {0x4000u};
        runtime.words[0x1010u] = 0x2000u;
        runtime.words[0x2004u] = 0u;
        runtime.words[0x1014u] = 3u;
        Check(pal4::TimedEventQueue::AddExistingCallback(
                  0x1000u, 0x6000u, 0x40800000u, 0x3000u,
                  runtime) == 0x88u &&
                  runtime.words[0x4000u] == 0x4000u &&
                  runtime.words[0x4004u] == 0x4000u &&
                  runtime.words[0x2004u] == 0x4000u &&
                  runtime.words[0x1014u] == 4u &&
                  runtime.copied_to == 0x4008u &&
                  runtime.copied.delay == 0x40800000u &&
                  runtime.copied.callback == 0x3000u &&
                  runtime.copied.owns_callback == 1u &&
                  runtime.calls.back() == "release-result",
              "existing callback list and 28-byte record");
        Check((std::find(runtime.calls.begin(), runtime.calls.end(),
                         "delete") != runtime.calls.end()) == !transfer,
              "only untransferred callback is deleted");
    }

    Runtime no_callback;
    no_callback.allocations = {0x4000u};
    no_callback.has_name_data = false;
    no_callback.words[0x1010u] = 0x2000u;
    Check(pal4::TimedEventQueue::AddExistingCallback(
              0x1000u, 0x6000u, 0u, 0u,
              no_callback) == 0u &&
              no_callback.copied.callback == 0u &&
              no_callback.copied.owns_callback == 0u &&
              no_callback.words[0x4000u] == 0x4000u &&
              std::find(no_callback.calls.begin(), no_callback.calls.end(),
                        "delete") == no_callback.calls.end(),
          "null callback still inserts a timed record");

    Runtime failed;
    failed.allocations = {0u};
    failed.words[0x1010u] = 0x2000u;
    bool raised = false;
    try {
        static_cast<void>(pal4::TimedEventQueue::AddExistingCallback(
            0x1000u, 0x6000u, 0u, 0x3000u, failed));
    } catch (const std::bad_alloc&) {
        raised = true;
    }
    Check(raised && failed.words[0x1014u] == 0u,
          "null node allocation becomes explicit failure before count update");
}

}  // namespace

int main() {
    try {
        CheckInsert(0u, true);
        CheckInsert(0x2100u, false);
        CheckGates();
        CheckPoetryResumeCallback();
        CheckExistingCallback();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
