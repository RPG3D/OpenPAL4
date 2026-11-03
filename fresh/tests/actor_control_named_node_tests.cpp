#include "pal4/actor_control_named_node.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using std::uint32_t;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Memory final : pal4::ActorControlNamedNodeRuntimeStub {
    std::uint8_t ReadByte(uint32_t address) override {
        return bytes[address];
    }
    uint32_t ReadDword(uint32_t address) override {
        uint32_t result = 0u;
        for (uint32_t offset = 0u; offset < 4u; ++offset)
            result |= static_cast<uint32_t>(ReadByte(address + offset))
                      << (8u * offset);
        return result;
    }
    void WriteByte(uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteDword(uint32_t address, uint32_t value) override {
        for (uint32_t offset = 0u; offset < 4u; ++offset)
            WriteByte(address + offset,
                      static_cast<std::uint8_t>(value >> (8u * offset)));
    }
    uint32_t TextLength(uint32_t address) override {
        events.emplace_back("length");
        if (forced_length != 0u) return forced_length;
        uint32_t length = 0u;
        while (ReadByte(address + length) != 0u) ++length;
        return length;
    }
    void SetErrorFlag(uint32_t record, std::uint8_t value) override {
        events.emplace_back("flag");
        WriteByte(record, value);
    }
    void ReallocateText(uint32_t record, uint32_t length) override {
        events.emplace_back("allocate");
        Check(length > 0u, "nonempty names request storage");
        WriteDword(record + 4u, 0x5001u);
        WriteDword(record + 12u, length);
        WriteByte(0x5000u, 0u);
    }
    void CopyBytes(uint32_t destination, uint32_t source,
                   uint32_t length) override {
        events.emplace_back("copy");
        for (uint32_t i = 0u; i < length; ++i)
            WriteByte(destination + i, ReadByte(source + i));
    }
    uint32_t FreeAddress(uint32_t address) override {
        freed.push_back(address);
        return 0x7777u;
    }
    uint32_t InitializeActorControl() override {
        events.emplace_back("actor");
        return 0x7000u;
    }
    uint32_t FloorAttributeInstance() override {
        events.emplace_back("floor");
        return 0x8000u;
    }
    std::int32_t CompareBytes(uint32_t left, uint32_t right,
                              uint32_t length) override {
        events.emplace_back("compare");
        comparisons.push_back({left, right, length});
        for (uint32_t i = 0u; i < length; ++i) {
            if (ReadByte(left + i) != ReadByte(right + i))
                return static_cast<std::int32_t>(ReadByte(left + i)) -
                       static_cast<std::int32_t>(ReadByte(right + i));
        }
        return 0;
    }
    uint32_t AudioManager() override {
        events.emplace_back("audio manager");
        return 0xa000u;
    }
    void PlaySound(uint32_t manager, uint32_t node, uint32_t sound,
                   uint32_t position, uint32_t mode,
                   uint32_t max_distance,
                   uint32_t min_distance) override {
        events.emplace_back("play");
        sound_calls.push_back({manager, node, sound, position,
                               mode, max_distance, min_distance});
        if (replace_sound_on_play)
            WriteDword(0x8000u + 44u + 4u, 0x9200u);
    }
    void InvokeSoundCallback(uint32_t callback,
                             uint32_t sound) override {
        events.emplace_back("callback");
        callback_calls.push_back({callback, sound});
    }
    void PutString(uint32_t address, const char* text) {
        while (*text != '\0') WriteByte(address++, static_cast<std::uint8_t>(*text++));
        WriteByte(address, 0u);
    }
    std::map<uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<uint32_t> freed;
    uint32_t forced_length{};
    bool replace_sound_on_play{};
    std::vector<std::vector<uint32_t>> sound_calls;
    std::vector<std::vector<uint32_t>> callback_calls;
    std::vector<std::vector<uint32_t>> comparisons;
};

void TestEmptyConstruction() {
    Memory memory;
    constexpr uint32_t node = 0x1000u;
    constexpr uint32_t source = 0x3123u;
    for (uint32_t offset = 0u; offset < 28u; ++offset)
        memory.WriteByte(node + offset, 0xa5u);
    memory.PutString(source, "");
    Check(pal4::ActorControlNamedNode::Construct(node, source, memory) ==
              node &&
              memory.ReadDword(node) ==
                  pal4::ActorControlNamedNode::kDerivedVtable &&
              memory.ReadByte(node + 4u) == 1u &&
              memory.ReadDword(node + 8u) == 0u &&
              memory.ReadDword(node + 12u) == 0u &&
              memory.ReadDword(node + 16u) == 0u &&
              memory.ReadDword(node + 20u) == 0u &&
              memory.ReadDword(node + 24u) == 0u &&
              memory.events == std::vector<std::string>{"length", "flag"},
          "empty name initializes all observed fields without allocating");
}

void TestNamedConstructionAndDestruction() {
    Memory memory;
    constexpr uint32_t node = 0x1000u;
    constexpr uint32_t source = 0x3123u;
    memory.PutString(source, "FL");
    Check(pal4::ActorControlNamedNode::Construct(node, source, memory) ==
              node &&
              memory.events == std::vector<std::string>{
                  "length", "flag", "allocate", "copy"} &&
              memory.ReadDword(node) ==
                  pal4::ActorControlNamedNode::kDerivedVtable &&
              memory.ReadDword(node + 8u) == 0x5001u &&
              memory.ReadDword(node + 12u) == 2u &&
              memory.ReadDword(node + 16u) == 2u &&
              memory.ReadByte(0x5001u) == 'F' &&
              memory.ReadByte(0x5002u) == 'L' &&
              memory.ReadByte(0x5003u) == 0u,
          "nonempty name reallocates, copies and terminates text");
    Check(pal4::ActorControlNamedNode::Destruct(node, memory) == 0x7777u &&
              memory.freed == std::vector<uint32_t>{0x5000u} &&
              memory.ReadDword(node) ==
                  pal4::ActorControlNamedNode::kBaseVtable &&
              memory.ReadDword(node + 8u) == 0u &&
              memory.ReadDword(node + 12u) == 0u &&
              memory.ReadDword(node + 16u) == 0u,
          "destructor frees unshared text and restores base vtable");
}

void TestOversizeAndReferenceRelease() {
    Memory memory;
    constexpr uint32_t node = 0x1000u;
    memory.forced_length = 0xfffffffeu;
    try {
        static_cast<void>(pal4::ActorControlNamedNode::Construct(
            node, 0x3100u, memory));
        Check(false, "oversize name must throw");
    } catch (const std::length_error&) {
        Check(memory.ReadDword(node) ==
                  pal4::ActorControlNamedNode::kBaseVtable &&
                  memory.events == std::vector<std::string>{"length"},
              "length error occurs before allocation");
    }

    memory.WriteDword(node + 8u, 0x6001u);
    memory.WriteDword(node + 12u, 4u);
    memory.WriteDword(node + 16u, 4u);
    memory.WriteByte(0x6000u, 2u);
    Check(pal4::ActorControlNamedNode::Destruct(node, memory) == 0x6000u &&
              memory.ReadByte(0x6000u) == 1u && memory.freed.empty(),
          "shared text decrements reference byte without freeing");
    memory.WriteDword(node + 8u, 0x6001u);
    memory.WriteByte(0x6000u, 0xffu);
    Check(pal4::ActorControlNamedNode::Destruct(node, memory) == 0x7777u &&
              memory.freed == std::vector<uint32_t>{0x6000u},
          "saturated reference byte frees text storage");
    Check(pal4::ActorControlNamedNode::Destruct(node, memory) == 0u,
          "empty destructor returns zero");
}

void TestFloorSoundTrigger() {
    Memory memory;
    constexpr uint32_t node = 0x1000u;
    constexpr uint32_t floor_text = 0x8000u + 44u;
    memory.PutString(pal4::ActorControlNamedNode::kEmptyErrorString, "");
    Check(pal4::ActorControlNamedNode::TriggerFloorSound(node, memory) ==
              0u &&
              memory.events == std::vector<std::string>{"actor"},
          "uninitialized actor control skips floor and audio access");

    memory.WriteDword(0x7000u, 1u);
    memory.events.clear();
    Check(pal4::ActorControlNamedNode::TriggerFloorSound(node, memory) ==
              1u &&
              memory.events == std::vector<std::string>{
                  "actor", "floor", "length", "compare"} &&
              memory.comparisons.back() ==
                  std::vector<uint32_t>{0u,
                      pal4::ActorControlNamedNode::kEmptyErrorString, 0u} &&
              memory.sound_calls.empty(),
          "empty floor sound is compared but not played");

    memory.PutString(0x9100u, "foot.wav");
    memory.WriteDword(floor_text + 4u, 0x9100u);
    memory.WriteDword(floor_text + 8u, 8u);
    memory.events.clear();
    Check(pal4::ActorControlNamedNode::TriggerFloorSound(node, memory) ==
              1u && memory.sound_calls.empty(),
          "missing world object suppresses nonempty sound");

    memory.WriteDword(node + 20u, 0xb000u);
    memory.WriteDword(node + 24u, 0xc000u);
    memory.events.clear();
    Check(pal4::ActorControlNamedNode::TriggerFloorSound(node, memory) ==
              1u &&
              memory.events == std::vector<std::string>{
                  "actor", "floor", "length", "compare",
                  "audio manager", "play", "callback"} &&
              memory.sound_calls.back() ==
                  std::vector<uint32_t>{0xa000u, node, 0x9100u,
                                        0xb000u + 172u, 1u,
                                        0x44160000u, 0x43960000u} &&
              memory.callback_calls.back() ==
                  std::vector<uint32_t>{0xc000u, 0x9100u},
          "floor sound forwards audio geometry and callback text");

    memory.WriteDword(floor_text + 4u, 0u);
    memory.WriteDword(floor_text + 8u, 1u);
    memory.WriteDword(node + 24u, 0u);
    memory.events.clear();
    Check(pal4::ActorControlNamedNode::TriggerFloorSound(node, memory) ==
              1u &&
              memory.sound_calls.back()[2] ==
                  pal4::ActorControlNamedNode::kEmptyText &&
              memory.events.back() == "play",
          "null text pointer uses empty-text fallback without callback");

    memory.WriteDword(floor_text + 4u, 0x9100u);
    memory.WriteDword(floor_text + 8u, 8u);
    memory.WriteDword(node + 24u, 0xc000u);
    memory.replace_sound_on_play = true;
    Check(pal4::ActorControlNamedNode::TriggerFloorSound(node, memory) ==
              1u && memory.sound_calls.back()[2] == 0x9100u &&
              memory.callback_calls.back() ==
                  std::vector<uint32_t>{0xc000u, 0x9200u},
          "callback rereads floor text after audio call");
}

void TestBaseDeletionAndOwner() {
    Memory memory;
    constexpr uint32_t node = 0x1000u;
    memory.WriteDword(node, pal4::ActorControlNamedNode::kDerivedVtable);
    Check(pal4::ActorControlNamedNode::SetOwner(node, 0xb000u,
                                                 memory) == 0xb000u &&
              memory.ReadDword(node + 20u) == 0xb000u,
          "owner setter writes field at byte offset twenty");
    Check(pal4::ActorControlNamedNode::DeleteBase(node, 2u,
                                                   memory) == node &&
              memory.ReadDword(node) ==
                  pal4::ActorControlNamedNode::kBaseVtable &&
              memory.freed.empty(),
          "base deleting wrapper retains storage when bit zero is clear");
    Check(pal4::ActorControlNamedNode::DeleteBase(node, 3u,
                                                   memory) == node &&
              memory.freed == std::vector<uint32_t>{node},
          "base deleting wrapper frees storage when bit zero is set");
}

}  // namespace

int main() {
    TestEmptyConstruction();
    TestNamedConstructionAndDestruction();
    TestOversizeAndReferenceRelease();
    TestFloorSoundTrigger();
    TestBaseDeletionAndOwner();
}
