#include "pal4/game_data_ai_config_object.h"

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

struct Runtime final : pal4::IGameDataParameterEffectRuntime {
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
    void EffectConstruct(std::uint32_t object, std::uint32_t name) override {
        calls.push_back("construct " + std::to_string(object) + " " +
                        std::to_string(name));
    }
    void ClearEffectData(std::uint32_t object) override {
        calls.push_back("clear " + std::to_string(object));
    }
    void ProcessElements(std::uint32_t array) override {
        calls.push_back("process " + std::to_string(array));
        if (refill_after_processing)
            words[array - 8] = words[array - 12] + 328;
    }
    void DestroyElement(std::uint32_t address, std::uint8_t flags) override {
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
    Check(it != runtime.calls.end(), "expected lifecycle call missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckCreate() {
    Runtime runtime;
    Check(pal4::GameDataParameterEffectObject::Create(
              0x12345678, runtime) == 0x1000 &&
              runtime.calls.front() == "allocate 140" &&
              runtime.calls[1] == "construct 4096 305419896" &&
              runtime.words[0x1000] == 0x84460c,
          "factory allocates 140 bytes, constructs Effect and writes vtable");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x78,
              "three array headers receive low byte of name");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + step] == 0,
                  "all three array pointer triplets start empty");
    }
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataParameterEffectObject::Create(
              0x12345678, failed) == 0 &&
              failed.calls.size() == 1 && failed.words.empty(),
          "allocation failure does not construct the Effect object");
}

void CheckDestroy() {
    Runtime runtime;
    runtime.refill_after_processing = true;
    runtime.words[0x1000 + 96] = 0x2000;
    runtime.words[0x1000 + 100] = 0x2000 + 2 * 328;
    runtime.words[0x1000 + 112] = 0x3000;
    runtime.words[0x1000 + 116] = 0x3000 + 2 * 60;
    runtime.words[0x1000 + 128] = 0x4000;
    Check(pal4::GameDataParameterEffectObject::Destroy(
              0x1000, runtime) == 0xbeef &&
              runtime.words[0x1000] == 0x84460c &&
              runtime.destroyed_texts.size() == 18,
          "destructor forwards base result and observes a second record pass");
    Check(runtime.destroyed_texts[0] == 0x2000 + 84 &&
              runtime.destroyed_texts[5] == 0x2000 + 4 &&
              runtime.destroyed_texts[6] == 0x2000 + 328 + 84 &&
              runtime.destroyed_texts[12] == 0x2000 + 84,
          "six text members are destroyed backward at 328-byte stride");
    Check(At(runtime, "clear 4096") < At(runtime, "text 8276") &&
              At(runtime, "process 4204") < At(runtime, "free 16384") &&
              At(runtime, "field 12288") < At(runtime, "field 12348") &&
              At(runtime, "field 12348") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "free 8192") &&
              runtime.calls.back() == "base 4096",
          "destructor releases Effect, fields, records and base in order");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u,
                                 120u, 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "destructor zeros all array pointers");
}

void CheckResetAndStorage() {
    Runtime reset;
    reset.words[0x1000 + 96] = 0x2000;
    reset.words[0x1000 + 100] = 0x2000 + 2 * 328;
    pal4::GameDataParameterEffectObject::ResetEffects(0x1000, reset);
    Check(reset.destroyed_texts.size() == 12 &&
              reset.destroyed_texts.front() == 0x2000 + 84 &&
              reset.destroyed_texts[6] == 0x2000 + 328 + 84 &&
              reset.words[0x1000 + 100] == 0x2000 &&
              reset.calls.front() == "clear 4096",
          "reset clears Effect then destroys both live records");

    Runtime storage;
    storage.words[0x5004] = 0x6000;
    storage.words[0x5008] = 0x6000 + 2 * 328;
    Check(pal4::GameDataParameterRecordStorage::Clear(
              0x5000, storage) == 0 &&
              storage.destroyed_texts.size() == 12 &&
              storage.destroyed_texts.front() == 0x6000 + 84 &&
              storage.destroyed_texts[6] == 0x6000 + 328 + 84 &&
              At(storage, "text " + std::to_string(0x6000 + 328 + 4)) <
                  At(storage, "free 24576") &&
              storage.words[0x5004] == 0 &&
              storage.words[0x5008] == 0 &&
              storage.words[0x500c] == 0,
          "array storage destroys records before freeing and zeroing pointers");
}

}  // namespace

int main() {
    try {
        CheckCreate();
        CheckDestroy();
        CheckResetAndStorage();
        std::cout << "game_data_parameter_effect_object_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
