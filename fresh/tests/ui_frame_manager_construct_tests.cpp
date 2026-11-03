#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerConstructRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t next = 0x10000u;
    bool null_collection = false;
    bool null_tree = false;
    bool null_sound = false;

    void ConstructEventTree(std::uint32_t address) override {
        Check(address == 0x5104u, "event tree offset");
        events.emplace_back("events");
    }
    void ConstructTree(std::uint32_t address,
                       std::uint8_t seed) override {
        Check(address == 0x5114u && seed == 0x5au,
              "embedded tree offset and seed");
        events.emplace_back("tree");
    }
    void SetErrorFlag(std::uint32_t address,
                      std::uint8_t flag) override {
        Check(address == 0x5124u && flag == 0u,
              "name buffer flag");
        events.emplace_back("name-init");
    }
    void InitializeEffects(std::uint32_t manager) override {
        Check(manager == 0x5000u, "effects owner");
        events.emplace_back("effects");
    }
    void InitializeXml(std::uint32_t address) override {
        Check(address == 0x514cu, "XML offset");
        events.emplace_back("xml");
    }
    void InitializeStruct(std::uint32_t address) override {
        Check(address == 0x51dcu, "late struct offset");
        events.emplace_back("late-struct");
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        events.emplace_back("allocate:" + std::to_string(size));
        if ((size == 0xfcu && null_collection) ||
            (size == 0x18u && null_tree) ||
            (size == 0x14u && null_sound))
            return 0u;
        const auto address = next;
        next += 0x1000u;
        return address;
    }
    std::uint32_t ConstructCollection(
        std::uint32_t address) override {
        events.emplace_back("collection");
        return address + 4u;
    }
    std::uint32_t AllocateAndInitializeNode(
        std::uint32_t owner) override {
        Check(owner == 0x11000u || owner == 0x10000u,
              "tree node owner");
        events.emplace_back("nil-node");
        return 0x9000u;
    }
    std::uint32_t InitializeSounds(
        std::uint32_t address) override {
        events.emplace_back("sounds");
        return address + 8u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void CopyDwordIfDestinationNonNull(
        std::uint32_t, std::uint32_t) override {
        events.emplace_back("copy-element");
    }
    void FinishRange(std::uint32_t begin,
                     std::uint32_t end) override {
        Check(begin == 0u && end == 0u,
              "initial pointer array is empty");
        events.emplace_back("finish-range");
    }
    void Free(std::uint32_t address) override {
        Check(address == 0u, "free initial null array");
        events.emplace_back("free-old-array");
    }
    std::uint32_t CollectionSize(
        std::uint32_t address) override {
        Check(address == 0x5188u, "pointer array size owner");
        events.emplace_back("array-size");
        return 0u;
    }
};

void CheckNormal() {
    Runtime runtime;
    Check(pal4::UiFrameManagerSingleton::Construct(
              0x5000u, 0x5au, runtime) == 0x5000u,
          "constructor returns this");
    Check(runtime.events == std::vector<std::string>{
              "events", "tree", "name-init", "effects", "xml",
              "allocate:252", "collection", "allocate:24",
              "nil-node", "allocate:36", "late-struct",
              "allocate:128", "finish-range", "free-old-array",
              "array-size", "allocate:20", "sounds"},
          "constructor stage and allocation order");
    Check(runtime.words.at(0x5000u) == 0x8443a4u &&
              runtime.words.at(0x519cu) == 0x10004u &&
              runtime.words.at(0x51a0u) == 0x11000u &&
              runtime.words.at(0x11004u) == 0x9000u &&
              runtime.words.at(0x11010u) == 0x12000u &&
              runtime.words.at(0x12000u) == 0x12000u &&
              runtime.words.at(0x12004u) == 0x12000u,
          "collection and tree layout");
    Check(runtime.words.at(0x518cu) == 0x13000u &&
              runtime.words.at(0x5190u) == 0x13000u &&
              runtime.words.at(0x5194u) == 0x13080u &&
              runtime.words.at(0x51a4u) == 0x14008u,
          "32-slot array and sound object");
    Check(runtime.bytes.at(0x5124u) == 0x5au &&
              runtime.bytes.at(0x5134u) == 0x5au &&
              runtime.bytes.at(0x5220u) == 0u &&
              runtime.bytes.at(0x5221u) == 0u &&
              runtime.bytes.at(0x5223u) == 0u,
          "seed and tail flags");
    for (std::uint32_t offset = 0x1a8u; offset <= 0x1d8u;
         offset += 4u)
        Check(runtime.words.at(0x5000u + offset) == 0u,
              "reserved fields zeroed");
}

void CheckNullAllocations() {
    Runtime runtime;
    runtime.null_collection = true;
    runtime.null_tree = true;
    runtime.null_sound = true;
    Check(pal4::UiFrameManagerSingleton::Construct(
              0x5000u, 0x5au, runtime) == 0x5000u,
          "constructor returns this after nullable allocations");
    Check(runtime.words.at(0x519cu) == 0u &&
              runtime.words.at(0x51a0u) == 0u &&
              runtime.words.at(0x51a4u) == 0u &&
              runtime.words.at(0x5000u) == 0x8443a4u,
          "nullable collection tree and sound fields");
    Check(runtime.events == std::vector<std::string>{
              "events", "tree", "name-init", "effects", "xml",
              "allocate:252", "allocate:24", "late-struct",
              "allocate:128", "finish-range", "free-old-array",
              "array-size", "allocate:20"},
          "null allocations skip their constructors");
}

}  // namespace

int main() {
    try {
        CheckNormal();
        CheckNullAllocations();
        std::cout << "ui_frame_manager_construct_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
