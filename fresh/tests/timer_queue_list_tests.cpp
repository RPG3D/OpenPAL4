#include "pal4/timer_queue_list.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITimerQueueListRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool throw_destroy = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write " + std::to_string(address) + "=" +
                           std::to_string(value));
        words[address] = value;
    }
    void DestroyEntry(std::uint32_t entry) override {
        calls.emplace_back("destroy " + std::to_string(entry));
        if (throw_destroy) throw std::runtime_error("entry failed");
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free " + std::to_string(address));
    }
};

void Configure(Runtime& runtime, unsigned count) {
    runtime.words[0x5004u] = 0x6000u;
    runtime.words[0x5008u] = count;
    runtime.words[0x6000u] = count == 0u ? 0x6000u : 0x7000u;
    runtime.words[0x6004u] = count == 0u ? 0x6000u
                             : count == 1u ? 0x7000u : 0x8000u;
    if (count != 0u) {
        runtime.words[0x7000u] = count == 1u ? 0x6000u : 0x8000u;
        runtime.words[0x7004u] = 0x6000u;
    }
    if (count == 2u) {
        runtime.words[0x8000u] = 0x6000u;
        runtime.words[0x8004u] = 0x7000u;
    }
}

void CheckEmpty() {
    Runtime runtime;
    Configure(runtime, 0u);
    Check(pal4::TimerQueueList::Destroy(0x5000u, runtime) == 0u,
          "empty list return");
    Check(runtime.calls == (std::vector<std::string>{
              "read 20484", "read 24576", "read 20484", "free 24576",
              "write 20484=0", "write 20488=0"}),
          "empty list frees only sentinel then clears fields");
}

void CheckTwoNodes() {
    Runtime runtime;
    Configure(runtime, 2u);
    Check(pal4::TimerQueueList::Destroy(0x5000u, runtime) == 0u,
          "two-node list return");
    Check(runtime.words[0x6000u] == 0x6000u &&
              runtime.words[0x6004u] == 0x6000u &&
              runtime.words[0x5004u] == 0u &&
              runtime.words[0x5008u] == 0u,
          "list unlinked and cleared");
    const auto first_destroy = std::find(runtime.calls.begin(),
                                          runtime.calls.end(), "destroy 28680");
    const auto first_free = std::find(runtime.calls.begin(),
                                       runtime.calls.end(), "free 28672");
    const auto second_destroy = std::find(runtime.calls.begin(),
                                           runtime.calls.end(), "destroy 32776");
    const auto second_free = std::find(runtime.calls.begin(),
                                        runtime.calls.end(), "free 32768");
    const auto head_free = std::find(runtime.calls.begin(),
                                      runtime.calls.end(), "free 24576");
    Check(first_destroy < first_free && first_free < second_destroy &&
              second_destroy < second_free && second_free < head_free,
          "entries destroyed and nodes freed before sentinel");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "write 20488=1") == 1 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "write 20488=0") == 2,
          "count decremented after each node then reset");
}

void CheckException() {
    Runtime runtime;
    Configure(runtime, 1u);
    runtime.throw_destroy = true;
    bool caught = false;
    try {
        (void)pal4::TimerQueueList::Destroy(0x5000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.words[0x6000u] == 0x6000u &&
              runtime.words[0x5008u] == 1u,
          "entry exception occurs after unlink but before count update");
    Check(runtime.calls.back() == "destroy 28680" &&
              runtime.calls.end() == std::find(runtime.calls.begin(),
                                               runtime.calls.end(),
                                               "free 28672"),
          "exception stops node free and later cleanup");
}

}  // namespace

int main() {
    try {
        CheckEmpty();
        CheckTwoNodes();
        CheckException();
        std::cout << "timer_queue_list_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
