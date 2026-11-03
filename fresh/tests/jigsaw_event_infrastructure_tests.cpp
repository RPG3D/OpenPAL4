#include "pal4/jigsaw_event_infrastructure.h"
#include "pal4/crt_noop_atexit_registration.h"

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

struct VectorRuntime final : pal4::IJigsawRecordVectorRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::uint32_t> freed;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void DestroyString(std::uint32_t address) override {
        destroyed.push_back(address);
    }
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        return 0xa5u;
    }
};

struct QueueRuntime final : pal4::IEventQueueRuntime {
    static constexpr std::uint32_t queue = 0x1000u;
    static constexpr std::uint32_t head = 0x2000u;
    static constexpr std::uint32_t node = 0x3000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {queue + 4u, head}, {queue + 8u, 0u}, {head + 4u, 0u}};
    std::uint32_t allocation = node;
    std::uint32_t copied_destination = 0u;
    std::uint32_t copied_source = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x24u, "event node size");
        return allocation;
    }
    void CopyEventData(std::uint32_t destination,
                       std::uint32_t source) override {
        copied_destination = destination;
        copied_source = source;
    }
};

struct CallbackRuntime final : pal4::IJigsawEventCallbackRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> writes;
    std::uint32_t invoked_object = 0u;
    std::uint32_t invoked_slot = 0u;

    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        writes.push_back(address);
    }
    std::uint32_t InvokeVirtual(std::uint32_t object,
                                std::uint32_t slot) override {
        invoked_object = object;
        invoked_slot = slot;
        return 0x55aau;
    }
};

struct AtExitRuntime final : pal4::ICrtNoopAtExitRuntime {
    std::uint32_t callback = 0u;
    int RegisterAtExit(std::uint32_t value) override {
        callback = value;
        return 17;
    }
};

void CheckVector() {
    VectorRuntime runtime;
    constexpr std::uint32_t vector = 0x1000u;
    constexpr std::uint32_t begin = 0x2000u;
    runtime.words[vector + 4u] = begin;
    runtime.words[vector + 8u] = begin + 2u * 0xc8u;
    runtime.words[vector + 0xcu] = begin + 3u * 0xc8u;
    Check(pal4::JigsawRecordVector::Clear(vector, runtime) == 0u &&
              runtime.destroyed == (std::vector<std::uint32_t>{
                  begin + 0x10u, begin + 0xc8u + 0x10u}) &&
              runtime.freed == (std::vector<std::uint32_t>{begin}) &&
              runtime.words[vector + 4u] == 0u &&
              runtime.words[vector + 8u] == 0u &&
              runtime.words[vector + 0xcu] == 0u,
          "record stride, string cleanup and vector reset");
    pal4::JigsawRecordVector::DestroyRecord(begin, runtime);
    Check(runtime.destroyed.back() == begin + 0x10u,
          "single record destructor");
    Check(pal4::JigsawRecordVector::FreeBuffer(begin, 99u, runtime) ==
              0xa5u && runtime.freed.back() == begin,
          "free wrapper discards second argument");

    VectorRuntime empty;
    empty.words[vector + 4u] = 0u;
    empty.words[vector + 8u] = 0u;
    Check(pal4::JigsawRecordVector::Clear(vector, empty) == 0u &&
              empty.destroyed.empty() && empty.freed.size() == 1u &&
              empty.freed[0] == 0u,
          "empty vector still frees begin pointer");
}

void CheckQueue() {
    QueueRuntime empty;
    Check(pal4::EventQueue::Add(
              QueueRuntime::queue, 0x9000u, empty) == 1u &&
              empty.words[QueueRuntime::head + 4u] == QueueRuntime::node &&
              empty.words[QueueRuntime::node] == QueueRuntime::node &&
              empty.words[QueueRuntime::node + 4u] == QueueRuntime::node &&
              empty.words[QueueRuntime::queue + 8u] == 1u &&
              empty.copied_destination == QueueRuntime::node + 8u &&
              empty.copied_source == 0x9000u,
          "first event links a self-referencing node");

    QueueRuntime occupied;
    constexpr std::uint32_t successor = 0x4000u;
    occupied.words[QueueRuntime::head + 4u] = successor;
    occupied.words[QueueRuntime::queue + 8u] = 3u;
    Check(pal4::EventQueue::Add(
              QueueRuntime::queue, 0x9000u, occupied) == 4u &&
              occupied.words[QueueRuntime::node] == QueueRuntime::head &&
              occupied.words[QueueRuntime::node + 4u] == successor &&
              occupied.words[successor] == QueueRuntime::node &&
              occupied.words[QueueRuntime::head + 4u] == QueueRuntime::node,
          "event insertion rewires predecessor and successor");

    QueueRuntime no_memory;
    no_memory.allocation = 0u;
    bool caught = false;
    try {
        (void)pal4::EventQueue::Add(
            QueueRuntime::queue, 0x9000u, no_memory);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && no_memory.words[QueueRuntime::head + 4u] == 0u,
          "allocation failure preserves the list");
}

void CheckCallbackAndCrt() {
    CallbackRuntime runtime;
    constexpr std::uint32_t object = 0x1000u;
    Check(pal4::JigsawEventCallback::Construct(
              object, 4u, 5u, 6u, runtime) == object &&
              runtime.words[object + 8u] == 4u &&
              runtime.words[object + 0xcu] == 5u &&
              runtime.words[object + 0x10u] == 6u &&
              runtime.words[object] ==
                  pal4::JigsawEventCallback::kVtable &&
              runtime.writes == (std::vector<std::uint32_t>{
                  object + 8u, object + 0xcu, object + 0x10u, object}),
          "callback construction and vtable write order");
    Check(pal4::JigsawEventCallback::Invoke(object, runtime) == 0x55aau &&
              runtime.invoked_object == object &&
              runtime.invoked_slot == 4u,
          "callback dispatches virtual slot one");

    CallbackRuntime puzzle;
    Check(pal4::PuzzleEventCallback::Construct(
              object, 0x1250u, 0u, 0x7000u, puzzle) == object &&
              puzzle.words.at(object) == pal4::PuzzleEventCallback::kVtable &&
              puzzle.words.at(object + 8u) == 0x1250u &&
              puzzle.words.at(object + 0xcu) == 0u &&
              puzzle.words.at(object + 0x10u) == 0x7000u &&
              puzzle.writes == (std::vector<std::uint32_t>{
                  object + 8u, object + 0xcu, object + 0x10u, object}),
          "puzzle callback retains its distinct vtable and write order");

    std::uint8_t guard = 2u;
    AtExitRuntime at_exit;
    Check(pal4::CrtNoopAtExitRegistration::Run(guard, at_exit) == 17 &&
              guard == 3u &&
              at_exit.callback == pal4::CrtNoopAtExitRegistration::kCallback,
          "CRT helper reuses the shared guard registration");
}

}  // namespace

int main() {
    try {
        CheckVector();
        CheckQueue();
        CheckCallbackAndCrt();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
