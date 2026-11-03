#include "pal4/ui_frame_timer.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kQueue = 0x1000u;
constexpr std::uint32_t kHead = 0x2000u;
constexpr std::uint32_t kCallback = 0x3000u;
constexpr std::uint32_t kNode = 0x4000u;
constexpr std::uint32_t kName = 0x6000u;
constexpr std::uint32_t kOwner = 0x7000u;
constexpr std::uint32_t kFunction = 0x8000u;

struct QueueRuntime final : pal4::ITimedEventQueueRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kQueue + 16u, kHead}, {kHead + 4u, 0u},
        {kQueue + 20u, 2u}};
    std::vector<std::uint32_t> allocations{kCallback, kNode};
    std::vector<std::string> calls;
    pal4::TimedQueueRecord copied{};
    std::uint32_t name_source = 0u;
    std::uint32_t destination = 0u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(!allocations.empty(), "unexpected allocation");
        Check(bytes == (allocations.size() == 2u ? 12u : 36u),
              "callback and node allocation sizes");
        const auto result = allocations.front();
        allocations.erase(allocations.begin());
        calls.emplace_back("allocate");
        return result;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void InitializeName(pal4::TimedQueueRecord& record,
                        std::uint32_t source) override {
        name_source = source;
        record.name_data = 0x5000u;
        calls.emplace_back("name");
    }
    void CopyRecord(std::uint32_t address,
                    pal4::TimedQueueRecord& source) override {
        destination = address;
        copied = source;
        source.owns_callback = 0u;
        calls.emplace_back("copy");
    }
    void DeleteCallback(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("transferred callback must not be deleted");
    }
    void ReleaseName(pal4::TimedQueueRecord& record) override {
        Check(record.name_data == 0x5000u, "temporary name release");
        calls.emplace_back("release");
    }
};

void CheckAdd(bool second) {
    QueueRuntime runtime;
    if (second)
        pal4::UiFrameTimer::AddSecond(
            kQueue, kName, 37u, kOwner, kFunction, runtime);
    else
        pal4::UiFrameTimer::Add(
            kQueue, kName, 37u, kOwner, kFunction, runtime);
    Check(runtime.words.at(kCallback) ==
              (second ? pal4::UiFrameTimer::kSecondCallbackVtable
                      : pal4::UiFrameTimer::kFirstCallbackVtable) &&
              runtime.words.at(kCallback + 4u) == kOwner &&
              runtime.words.at(kCallback + 8u) == kFunction,
          "variant callback fields");
    Check(runtime.words.at(kNode) == kNode &&
              runtime.words.at(kNode + 4u) == kNode &&
              runtime.words.at(kHead + 4u) == kNode &&
              runtime.words.at(kQueue + 20u) == 3u,
          "queue node links and count");
    Check(runtime.destination == kNode + 8u &&
              runtime.name_source == kName &&
              runtime.copied.delay == 37u &&
              runtime.copied.callback == kCallback &&
              runtime.copied.owns_callback == 1u &&
              runtime.calls.back() == "release",
          "timer record copy and ownership transfer");

    QueueRuntime no_callback;
    if (second)
        pal4::UiFrameTimer::AddSecond(
            kQueue, kName, 37u, kOwner, 0u, no_callback);
    else
        pal4::UiFrameTimer::Add(
            kQueue, kName, 37u, kOwner, 0u, no_callback);
    Check(no_callback.calls.empty() &&
              no_callback.words.at(kQueue + 20u) == 2u,
          "null callback skips queue insertion");
}

struct CallbackRuntime final : pal4::IUiTimerCallbackRuntime {
    std::vector<std::string> calls;
    std::uint32_t address = 0u;
    std::uint32_t vtable = 0u;
    void WriteWord(std::uint32_t target, std::uint32_t value) override {
        address = target;
        vtable = value;
        calls.emplace_back("vtable");
    }
    void Free(std::uint32_t target) override {
        Check(target == address, "delete target");
        calls.emplace_back("free");
    }
};

void CheckDestructors() {
    CallbackRuntime direct;
    pal4::UiTimerCallback::Destruct(kCallback, direct);
    Check(direct.address == kCallback &&
              direct.vtable == pal4::UiTimerCallback::kBaseVtable &&
              direct.calls == std::vector<std::string>{"vtable"},
          "destructor writes base vtable");
    for (const std::uint8_t flags : {0u, 1u, 2u, 3u}) {
        CallbackRuntime runtime;
        Check(pal4::UiTimerCallback::Delete(kCallback, flags, runtime) ==
                  kCallback &&
                  runtime.calls ==
                      (flags & 1u ? std::vector<std::string>{"vtable", "free"}
                                  : std::vector<std::string>{"vtable"}),
              "deleting destructor flag bit");
    }
}

}  // namespace

int main() {
    try {
        CheckAdd(false);
        CheckAdd(true);
        CheckDestructors();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
