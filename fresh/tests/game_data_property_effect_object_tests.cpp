#include "pal4/game_data_property_effect_object.h"

#include <algorithm>
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

struct Runtime final : pal4::IGameDataPropertyEffectRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed_texts;
    std::uint32_t allocation = 0x1000;
    bool refill_after_processing = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate " + std::to_string(size));
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void EffectConstruct(std::uint32_t object,
                         std::uint32_t name) override {
        calls.push_back("construct " + std::to_string(object) + " " +
                        std::to_string(name));
    }
    void ClearEffectData(std::uint32_t object) override {
        calls.push_back("clear " + std::to_string(object));
    }
    void ProcessElements(std::uint32_t array) override {
        calls.push_back("process " + std::to_string(array));
        if (refill_after_processing)
            words[array - 8] = words[array - 12] + 168;
    }
    void DestroyElement(std::uint32_t address,
                        std::uint8_t flags) override {
        Check(flags == 0, "field destruction flags are zero");
        calls.push_back("field " + std::to_string(address));
    }
    void ResetRecordPointers(std::uint32_t) override {}
    void SetErrorFlag(std::uint32_t, std::uint8_t) override {}
    std::uint32_t EffectDestruct(std::uint32_t object) override {
        calls.push_back("base " + std::to_string(object));
        return 0xbeef;
    }
    void DestroyTextRecord(std::uint32_t address) override {
        destroyed_texts.push_back(address);
        calls.push_back("text " + std::to_string(address));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected lifecycle operation missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckCreate() {
    Runtime runtime;
    Check(pal4::GameDataPropertyEffectObject::Create(
              0x12345678, runtime) == 0x1000 &&
              runtime.calls.front() == "allocate 140" &&
              runtime.calls[1] == "construct 4096 305419896" &&
              runtime.words[0x1000] == 0x84462c,
          "factory allocates and constructs the Property Effect object");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x78,
              "array headers receive the low name byte");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + step] == 0,
                  "all three pointer triplets start empty");
    }
    Check(runtime.calls.back() == "write 4096",
          "vtable is installed after member initialization");
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataPropertyEffectObject::Create(
              0x12345678, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "allocation failure skips Effect construction");
}

void CheckDestroy() {
    Runtime runtime;
    runtime.refill_after_processing = true;
    runtime.words[0x1000 + 96] = 0x2000;
    runtime.words[0x1000 + 100] = 0x2000 + 2 * 168;
    runtime.words[0x1000 + 112] = 0x3000;
    runtime.words[0x1000 + 116] = 0x3000 + 2 * 60;
    runtime.words[0x1000 + 128] = 0x4000;
    Check(pal4::GameDataPropertyEffectObject::Destroy(
              0x1000, runtime) == 0xbeef &&
              runtime.words[0x1000] == 0x84462c &&
              runtime.destroyed_texts.size() == 21,
          "destructor releases original records and a second-pass record");
    Check(runtime.destroyed_texts[0] == 0x2000 + 112 &&
              runtime.destroyed_texts[6] == 0x2000 + 16 &&
              runtime.destroyed_texts[7] == 0x2000 + 168 + 112 &&
              runtime.destroyed_texts[14] == 0x2000 + 112,
          "seven texts per record are destroyed in reverse order");
    Check(At(runtime, "clear 4096") < At(runtime, "text 8304") &&
              At(runtime, "process 4204") < At(runtime, "free 16384") &&
              At(runtime, "field 12288") < At(runtime, "field 12348") &&
              At(runtime, "field 12348") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "free 8192") &&
              runtime.calls.back() == "base 4096",
          "Effect, fields and records are released before base destructor");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u,
                                 120u, 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "destructor clears all array pointers");
}

void CheckStorageAndReset() {
    Runtime storage;
    storage.words[0x5004] = 0x6000;
    storage.words[0x5008] = 0x6000 + 2 * 168;
    Check(pal4::GameDataPropertyRecordStorage::Clear(
              0x5000, storage) == 0 &&
              storage.destroyed_texts.size() == 14 &&
              storage.destroyed_texts[0] == 0x6000 + 112 &&
              storage.destroyed_texts[7] == 0x6000 + 168 + 112 &&
              At(storage, "text 24856") < At(storage, "free 24576") &&
              storage.words[0x5004] == 0 &&
              storage.words[0x5008] == 0 &&
              storage.words[0x500c] == 0,
          "storage clears records before freeing and zeroing pointers");

    Runtime reset;
    reset.words[0x1000 + 96] = 0x2000;
    reset.words[0x1000 + 100] = 0x2000 + 2 * 168;
    pal4::GameDataPropertyEffectObject::ResetEffects(0x1000, reset);
    Check(reset.calls.front() == "clear 4096" &&
              reset.destroyed_texts.size() == 14 &&
              reset.destroyed_texts[0] == 0x2000 + 112 &&
              reset.destroyed_texts[7] == 0x2000 + 168 + 112 &&
              reset.words[0x1000 + 100] == 0x2000 &&
              reset.calls.back() == "write 4196",
          "reset clears Effect, destroys records and rewinds array end");

    Runtime direct;
    pal4::GameDataPropertyRecordStorage::DestroyRange(
        0x7000, 0x7000 + 2 * 168, direct);
    Check(direct.destroyed_texts.size() == 14 &&
              direct.destroyed_texts.front() == 0x7000 + 112 &&
              direct.destroyed_texts.back() == 0x7000 + 168 + 16,
          "range helper destroys two records with reverse text members");
    pal4::GameDataPropertyRecordStorage::DestroyRange(0x8000, 0x8000, direct);
    Check(direct.destroyed_texts.size() == 14,
          "empty record range has no destruction calls");
}

}  // namespace

int main() {
    try {
        CheckCreate();
        CheckDestroy();
        CheckStorageAndReset();
        std::cout << "game_data_property_effect_object_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
