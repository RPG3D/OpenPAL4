#include "pal4/animation_event_queue.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IAnimationEventQueueRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> finished;
    std::vector<std::uint32_t> deleted;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> advanced;
    std::vector<std::uint32_t> events;
    std::vector<std::uint32_t> event_arguments;
    std::vector<std::string> calls;
    std::uint32_t next_allocation = 0x9000u;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word " + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 16u, "queue nodes use 16-byte allocations");
        const auto address = next_allocation;
        next_allocation += 0x100u;
        calls.emplace_back("allocate");
        return address;
    }
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free " + std::to_string(address));
        return 0xde000000u | address;
    }
    void DeleteAnimation(std::uint32_t animation) override {
        deleted.push_back(animation);
        calls.emplace_back("delete " + std::to_string(animation));
    }
    void AdvanceAnimation(std::uint32_t animation,
                          float delta) override {
        Check(delta == 0.1f || delta == 0.25f,
              "animation receives full frame delta");
        advanced.push_back(animation);
        calls.emplace_back("advance " + std::to_string(animation));
    }
    void CallAnimationEvent(std::uint32_t animation,
                            std::uint32_t argument) override {
        events.push_back(animation);
        event_arguments.push_back(argument);
        calls.emplace_back("event " + std::to_string(animation));
    }
    std::uint32_t AnimationFinished(std::uint32_t animation) override {
        calls.emplace_back("finished " + std::to_string(animation));
        return finished[animation];
    }
};

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::AnimationEventQueue::Construct(
              0x5000u, 0xa5u, runtime) == 0x5000u,
          "constructor returns object");
    Check(runtime.bytes.at(0x5000u) == 0xa5u &&
              runtime.bytes.at(0x500cu) == 0xa5u &&
              runtime.words.at(0x5004u) == 0x9000u &&
              runtime.words.at(0x9000u) == 0x9000u &&
              runtime.words.at(0x9004u) == 0x9000u,
          "seed and circular list head");
    for (const auto offset : {8u, 16u, 20u, 24u, 28u, 32u, 36u,
                              40u, 44u, 48u, 52u, 56u, 64u})
        Check(runtime.words.at(0x5000u + offset) == 0u,
              "queue storage initialized to zero");
    Check(runtime.words.at(0x503cu) == 0x3e4ccccdu,
          "default interval is float 0.2");
}

void CheckAddAndDelay() {
    Runtime runtime;
    (void)pal4::AnimationEventQueue::Construct(0x5000u, 1u, runtime);
    Check(pal4::AnimationEventQueue::AddWithDelay(
              0x5000u, 1u, 0xb000u, runtime) == 0x9108u &&
              runtime.words.find(0xb034u) == runtime.words.end(),
          "first animation has no computed delay");
    runtime.words[0xb030u] = Bits(0.05f);
    Check(pal4::AnimationEventQueue::AddWithDelay(
              0x5000u, 0u, 0xb100u, runtime) == 0x9208u &&
              runtime.words.at(0xb134u) == Bits(0.2f - 0.05f),
          "next animation delay uses last elapsed time");
    Check(runtime.words.at(0x5008u) == 2u &&
              runtime.words.at(0x9000u) == 0x9100u &&
              runtime.words.at(0x9004u) == 0x9200u &&
              runtime.words.at(0x9100u) == 0x9200u &&
              runtime.words.at(0x9104u) == 0x9000u &&
              runtime.words.at(0x9200u) == 0x9000u &&
              runtime.words.at(0x9204u) == 0x9100u &&
              runtime.bytes.at(0x9108u) == 1u &&
              runtime.bytes.at(0x9208u) == 0u,
          "tail insertion preserves both list links and ownership");
}

void CheckUpdate() {
    Runtime runtime;
    (void)pal4::AnimationEventQueue::Construct(0x5000u, 0u, runtime);
    (void)pal4::AnimationEventQueue::Add(
        0x5000u, 1u, 0xb000u, runtime);
    runtime.words[0xb034u] = Bits(0.0f);
    runtime.words[0xb030u] = Bits(0.0f);
    runtime.finished[0xb000u] = 1u;
    Check(pal4::AnimationEventQueue::Update(
              0x5000u, 0.1f, runtime) == 0xde009100u &&
              runtime.advanced == std::vector<std::uint32_t>{0xb000u} &&
              runtime.deleted == std::vector<std::uint32_t>{0xb000u} &&
              runtime.words.at(0xb030u) == Bits(0.1f) &&
              runtime.words.at(0x5008u) == 0u &&
              runtime.words.at(0x9000u) == 0x9000u &&
              runtime.words.at(0x9004u) == 0x9000u,
          "zero delay runs immediately and finished owned animation is retired");

    (void)pal4::AnimationEventQueue::Add(
        0x5000u, 0u, 0xb100u, runtime);
    runtime.words[0xb134u] = Bits(-1.0f);
    runtime.words[0xb130u] = Bits(1.0f);
    runtime.finished[0xb100u] = 0u;
    Check(pal4::AnimationEventQueue::Update(
              0x5000u, 0.25f, runtime) == 0u &&
              runtime.words.at(0xb130u) == Bits(1.25f) &&
              runtime.words.at(0x5008u) == 1u &&
              runtime.deleted.size() == 1u,
          "unfinished unowned animation stays linked");
    runtime.words[0xb134u] =
        Bits(std::numeric_limits<float>::quiet_NaN());
    (void)pal4::AnimationEventQueue::Update(0x5000u, 0.25f, runtime);
    Check(runtime.advanced.size() == 3u,
          "unordered delay follows active branch");
    runtime.words[0xb134u] = Bits(-0.0f);
    (void)pal4::AnimationEventQueue::Update(0x5000u, 0.25f, runtime);
    Check(runtime.advanced.size() == 4u && runtime.words.at(0xb134u)==Bits(-0.0f),
          "negative zero follows equal active branch without modifying delay");
}

void CheckClearAndDestroy() {
    Runtime runtime;
    (void)pal4::AnimationEventQueue::Construct(0x5000u, 2u, runtime);
    (void)pal4::AnimationEventQueue::Add(
        0x5000u, 1u, 0xb000u, runtime);
    (void)pal4::AnimationEventQueue::Add(
        0x5000u, 0u, 0xb100u, runtime);
    runtime.words[0x5010u] = 0xc000u;
    runtime.words[0x5014u] = 0xd000u;
    runtime.words[0x5018u] = 0xc000u;
    runtime.words[0x501cu] = 0xe000u;
    runtime.words[0x5030u] = 0xe000u;
    runtime.words[0x5038u] = 1u;
    runtime.words[0xe000u] = 0xc000u;
    runtime.bytes[0xc000u] = 1u;
    runtime.words[0xc004u] = 0xb200u;
    Check(pal4::AnimationEventQueue::Clear(0x5000u, runtime) == 0u &&
              runtime.words.at(0x5008u) == 0u &&
              runtime.words.at(0x9000u) == 0x9000u &&
              runtime.words.at(0x9004u) == 0x9000u &&
              runtime.words.at(0x5038u) == 0u &&
              runtime.words.at(0x5030u) == 0xe000u &&
              runtime.deleted ==
                  (std::vector<std::uint32_t>{0xb000u, 0xb200u}),
          "clear releases owned list/deque payloads and leaves map pointer");
    Check(runtime.freed ==
              (std::vector<std::uint32_t>{0x9100u, 0x9200u,
                                           0xc000u, 0xe000u}),
          "clear frees nodes, deque block and pointer map in order");
    Check(pal4::AnimationEventQueue::Destroy(0x5000u, runtime) ==
              0xde009000u &&
              runtime.freed.back() == 0x9000u &&
              runtime.words.at(0x5004u) == 0u &&
              runtime.words.at(0x5008u) == 0u,
          "destructor clears then frees list sentinel");
}

void CheckBlockTransition() {
    Runtime runtime;
    (void)pal4::AnimationEventQueue::Construct(0x5000u, 0u, runtime);
    runtime.words[0x5010u] = 0xc000u;
    runtime.words[0x5014u] = 0xd000u;
    runtime.words[0x5018u] = 0xcff8u;
    runtime.words[0x501cu] = 0xe000u;
    runtime.words[0x5030u] = 0xe000u;
    runtime.words[0x5038u] = 2u;
    runtime.words[0xe000u] = 0xc000u;
    runtime.words[0xe004u] = 0xd000u;
    runtime.bytes[0xcff8u] = 0u;
    runtime.bytes[0xd000u] = 0u;
    Check(pal4::AnimationEventQueue::Clear(0x5000u, runtime) == 0u &&
              runtime.freed ==
                  (std::vector<std::uint32_t>{0xc000u, 0xd000u,
                                               0xe000u}) &&
              runtime.words.at(0x5018u) == 0u &&
              runtime.words.at(0x501cu) == 0u,
          "deque cursor crosses 4096-byte block boundary");
}

void CheckListHelpers() {
    Runtime runtime;
    (void)pal4::AnimationEventQueue::Construct(0x5000u, 0u, runtime);
    (void)pal4::AnimationEventQueue::Add(
        0x5000u, 1u, 0xb000u, runtime);
    (void)pal4::AnimationEventQueue::Add(
        0x5000u, 0u, 0xb100u, runtime);
    Check(pal4::AnimationEventQueue::Dispatch(
              0x5000u, 0x12345678u, runtime) == 0x9000u &&
              runtime.events ==
                  (std::vector<std::uint32_t>{0xb000u, 0xb100u}) &&
              runtime.event_arguments ==
                  (std::vector<std::uint32_t>{0x12345678u,
                                               0x12345678u}),
          "event dispatch visits list order and forwards full argument");

    runtime.words[0x7000u] = 0x9100u;
    Check(pal4::AnimationEventQueue::AdvanceCursor(
              0x7000u, 0x7004u, runtime) == 0x7004u &&
              runtime.words.at(0x7000u) == 0x9200u &&
              runtime.words.at(0x7004u) == 0x9100u,
          "cursor helper outputs old node and advances to next");
    Check(pal4::AnimationEventQueue::EraseAt(
              0x5000u, 0x7004u, 0x9100u, runtime) == 0x7004u &&
              runtime.words.at(0x7004u) == 0x9200u &&
              runtime.words.at(0x9000u) == 0x9200u &&
              runtime.words.at(0x9204u) == 0x9000u &&
              runtime.words.at(0x5008u) == 1u &&
              runtime.deleted == std::vector<std::uint32_t>{0xb000u},
          "erase helper unlinks, deletes and returns cursor address");

    runtime.words[0x5038u] = 3u;
    Check(pal4::AnimationEventQueue::DestroyList(
              0x5000u, runtime) == 0u &&
              runtime.words.at(0x5004u) == 0u &&
              runtime.words.at(0x5008u) == 0u &&
              runtime.words.at(0x5038u) == 3u &&
              runtime.deleted == std::vector<std::uint32_t>{0xb000u} &&
              runtime.freed ==
                  (std::vector<std::uint32_t>{0x9100u, 0x9200u,
                                               0x9000u}),
          "list-only cleanup leaves deque storage untouched");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckAddAndDelay();
        CheckUpdate();
        CheckClearAndDestroy();
        CheckBlockTransition();
        CheckListHelpers();
        std::cout << "animation_event_queue_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
