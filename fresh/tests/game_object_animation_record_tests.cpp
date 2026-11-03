#include "pal4/game_object_animation_record.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

void ConstructPreservesUntouchedBytes() {
    pal4::GameObjectAnimationRecordState record;
    record.words.fill(0xaabbccddu);
    pal4::GameObjectAnimationRecord::Construct(record, 0x12345600u, 0x5au);
    for (const auto index : {0, 4, 9})
        Check(record.words[index] == 0xaabbcc5au,
              "source stack byte replaces only low byte");
    for (const auto index : {1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 14, 15, 16})
        Check(record.words[index] == 0,
              "constructor clears exactly listed record fields");
    Check(record.words[13] == 0xaabbcc12u,
          "manager first byte comes from high byte of address plus 52");

    record.words.fill(0xffffffffu);
    pal4::GameObjectAnimationRecord::Construct(record, 0xfe000000u, 0);
    Check(record.words[0] == 0xffffff00u &&
              record.words[4] == 0xffffff00u &&
              record.words[9] == 0xffffff00u &&
              record.words[13] == 0xfffffffeu,
          "zero stack byte and second manager address preserve upper bytes");
}

class Runtime final : public pal4::IGameObjectAnimationRecordRuntime {
public:
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint8_t> references;

    void DestroyEvent(std::uint32_t address) override {
        calls.emplace_back("destroy:" + std::to_string(address));
    }
    int FreeAnimationResource(std::uint32_t address) override {
        calls.emplace_back("free-animation:" + std::to_string(address));
        return -7;
    }
    void CleanupTailBuffer2(
        pal4::GameObjectAnimationRecordState&) override {
        calls.emplace_back("tail2");
    }
    void CleanupTailBuffer(
        pal4::GameObjectAnimationRecordState&) override {
        calls.emplace_back("tail");
    }
    void FreeMemory(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(references.contains(address), "string reference byte exists");
        calls.emplace_back("read:" + std::to_string(address));
        return references.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
        references[address] = value;
    }
};

void CleanupAndDestruct() {
    Runtime runtime;
    pal4::GameObjectAnimationRecordState record;
    record.words[10] = 0x2000u;
    record.words[11] = 0x2000u + 2u * 0x14cu;
    record.words[12] = 0x3000u;
    record.words[8] = 0x7000u;
    Check(pal4::GameObjectAnimationRecord::Cleanup(record, runtime) == -7 &&
              record.words[8] == 0 &&
              record.words[11] == record.words[10] &&
              record.words[12] == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "destroy:8192", "destroy:8524", "free-animation:28672"},
          "cleanup destroys event range before releasing animation resource");
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationRecord::Cleanup(record, runtime) == 0 &&
              runtime.calls.empty(),
          "repeated cleanup has no event or resource effects");

    record.words[0] = 0x111u;
    record.words[1] = 0x5000u;
    record.words[2] = 4;
    record.words[3] = 5;
    record.words[5] = 0x6000u;
    record.words[6] = 6;
    record.words[7] = 7;
    record.words[8] = 0x7000u;
    record.words[11] = 0x214cu;
    runtime.references[0x5fffu] = 0;
    runtime.references[0x4fffu] = 3;
    pal4::GameObjectAnimationRecord::Destruct(record, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "destroy:8192", "free-animation:28672", "tail2", "tail",
              "free:8192", "read:24575", "free:24575", "read:20479",
              "write:20479:2"} &&
              record.words[0] == 0x111u &&
              record.words[1] == 0 && record.words[2] == 0 &&
              record.words[3] == 0 && record.words[5] == 0 &&
              record.words[6] == 0 && record.words[7] == 0 &&
              record.words[8] == 0 && record.words[10] == 0 &&
              record.words[11] == 0 && record.words[12] == 0 &&
              runtime.references[0x4fffu] == 2,
          "destructor cleans tail, event storage and two refcounted strings");

    runtime.calls.clear();
    record.words[1] = 0x5000u;
    runtime.references[0x4fffu] = 0xffu;
    pal4::GameObjectAnimationRecord::Destruct(record, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "tail2", "tail", "free:0", "read:20479", "free:20479"},
          "refcount 255 frees and null event storage still reaches MemFree");
}

class EventMemory final : public pal4::IGameObjectAnimationEventMemory {
public:
    std::map<std::uint32_t, pal4::AnimationEventData> events;
    std::vector<std::string> calls;

    pal4::AnimationEventData ReadEvent(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return events.at(address);
    }
    void WriteEvent(std::uint32_t address,
                    const pal4::AnimationEventData& event) override {
        calls.emplace_back("write:" + std::to_string(address));
        events[address] = event;
    }
};

void ResetEventTimes() {
    EventMemory memory;
    pal4::GameObjectAnimationRecordState record;
    record.words[10] = 0x2000u;
    record.words[11] = 0x2000u;
    Check(pal4::GameObjectAnimationRecord::ResetEventTimes(record, memory) ==
                  0x2000u && memory.calls.empty(),
          "empty event range returns end without reads");
    record.words[11] = 0x2000u + 2u * 0x14cu;
    for (std::uint32_t index = 0; index < 2; ++index) {
        pal4::AnimationEventData event{};
        const auto trigger = 0x3f800000u + index;
        const std::uint32_t fired = 1;
        std::memcpy(event.data() + 192, &trigger, 4);
        std::memcpy(event.data() + 196, &fired, 4);
        memory.events[0x2000u + index * 0x14cu] = event;
    }
    Check(pal4::GameObjectAnimationRecord::ResetEventTimes(record, memory) ==
                  record.words[11] &&
              memory.calls == std::vector<std::string>{
                  "read:8192", "write:8192", "read:8524", "write:8524"},
          "record walks every 332-byte event and returns current end");
    for (std::uint32_t index = 0; index < 2; ++index) {
        const auto& event = memory.events.at(0x2000u + index * 0x14cu);
        std::uint32_t fired = 1;
        std::uint32_t end_time = 0;
        std::memcpy(&fired, event.data() + 196, 4);
        std::memcpy(&end_time, event.data() + 200, 4);
        Check(fired == 0 && end_time == 0x3f800000u + index,
              "record reset delegates event time normalization");
    }
}

}  // namespace

int main() {
    ConstructPreservesUntouchedBytes();
    CleanupAndDestruct();
    ResetEventTimes();
    std::cout << "game_object_animation_record_tests passed\n";
}
