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

struct Runtime final : pal4::IGameDataMissionEffectRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x1000;
    std::uint32_t reset_result = 0x1234;
    std::uint32_t destruction_result = 0x5678;
    std::uint32_t revised_record_end = 0;

    void Log(const std::string& name, std::uint32_t address) {
        calls.push_back(name + " " + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Log("allocate", size);
        return allocation;
    }
    void Free(std::uint32_t address) override { Log("free", address); }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void EffectConstruct(std::uint32_t object, std::uint32_t name) override {
        calls.push_back("construct " + std::to_string(object) + " " +
                        std::to_string(name));
    }
    void ClearEffectData(std::uint32_t object) override {
        Log("clear effect", object);
    }
    std::uint32_t ClearEffectDataForReset(std::uint32_t object) override {
        Log("clear for reset", object);
        return reset_result;
    }
    void ProcessElements(std::uint32_t address) override {
        Log("process", address);
        if (revised_record_end != 0) words[0x1064] = revised_record_end;
    }
    void DestroyElement(std::uint32_t address, std::uint8_t flags) override {
        calls.push_back("destroy field " + std::to_string(address) + " " +
                        std::to_string(flags));
    }
    void DestroyTextRecord(std::uint32_t address) override {
        Log("destroy text", address);
    }
    std::uint32_t DestroyMissionRecord(std::uint32_t address) override {
        Log("destroy record", address);
        return address;
    }
    void ResetRecordPointers(std::uint32_t address) override {
        Log("reset record", address);
    }
    void SetErrorFlag(std::uint32_t address, std::uint8_t) override {
        Log("set error", address);
    }
    std::uint32_t EffectDestruct(std::uint32_t object) override {
        Log("destruct effect", object);
        return destruction_result;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected runtime call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckCreate() {
    Runtime runtime;
    Check(pal4::GameDataMissionEffectObject::Create(0x1234, runtime) ==
              0x1000,
          "factory returns allocated object");
    Check(runtime.calls[0] == "allocate 140" &&
              runtime.calls[1] == "construct 4096 4660" &&
              runtime.calls.back() == "word 4096 8668620",
          "factory constructs the Effect base before installing Mission vtable");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x34,
              "embedded text seed is the name low byte");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + step] == 0,
                  "embedded text pointer triplet is empty");
    }
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataMissionEffectObject::Create(7, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "allocation failure stops before base construction");
}

void CheckDestroy() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2088;
    runtime.words[0x1070] = 0x3000;
    runtime.words[0x1074] = 0x3078;
    runtime.words[0x1080] = 0x4000;
    runtime.revised_record_end = 0x2044;
    Check(pal4::GameDataMissionEffectObject::Destroy(0x1000, runtime) ==
              runtime.destruction_result,
          "destructor forwards Effect base result");
    Check(runtime.calls.front() == "word 4096 8668620" &&
              At(runtime, "clear effect 4096") <
                  At(runtime, "destroy text 8248") &&
              At(runtime, "destroy text 8248") <
                  At(runtime, "destroy text 8232") &&
              At(runtime, "destroy text 8232") <
                  At(runtime, "destroy text 8216") &&
              At(runtime, "destroy text 8216") <
                  At(runtime, "destroy text 8316") &&
              At(runtime, "destroy text 8284") <
                  At(runtime, "process 4204") &&
              At(runtime, "process 4204") < At(runtime, "free 16384") &&
              At(runtime, "free 16384") <
                  At(runtime, "destroy field 12288 0") &&
              At(runtime, "destroy field 12288 0") <
                  At(runtime, "destroy field 12348 0") &&
              At(runtime, "destroy field 12348 0") <
                  At(runtime, "free 12288") &&
              At(runtime, "free 12288") <
                  At(runtime, "destroy record 8192") &&
              At(runtime, "destroy record 8192") <
                  At(runtime, "free 8192") &&
              runtime.calls.back() == "destruct effect 4096",
          "destructor follows IDA member and base cleanup order");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "destroy record 8260") == 0,
          "record end is read again after processing the auxiliary array");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u, 120u,
                                 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "destructor clears all three pointer triplets");

    Runtime empty;
    Check(pal4::GameDataMissionEffectObject::Destroy(0x1000, empty) ==
              empty.destruction_result &&
              std::count_if(empty.calls.begin(), empty.calls.end(),
                            [](const std::string& call) {
                                return call.starts_with("destroy text ") ||
                                       call.starts_with("destroy record ");
                            }) == 0,
          "empty arrays still release base without record destruction");
}

void CheckStorageAndReset() {
    Runtime storage;
    storage.words[0x5004] = 0x6000;
    storage.words[0x5008] = 0x6088;
    Check(pal4::GameDataMissionRecordStorage::Clear(0x5000, storage) == 0 &&
              storage.calls[0] == "destroy record 24576" &&
              storage.calls[1] == "destroy record 24644" &&
              storage.calls[2] == "free 24576" &&
              storage.words[0x5004] == 0 && storage.words[0x5008] == 0 &&
              storage.words[0x500c] == 0,
          "record storage destroys 68-byte elements before freeing array");
    Runtime empty_storage;
    Check(pal4::GameDataMissionRecordStorage::Clear(0x5000, empty_storage) ==
              0 &&
              empty_storage.calls.front() == "free 0",
          "empty storage still forwards null to allocator free");

    Runtime reset;
    reset.words[0x1060] = 0x2000;
    reset.words[0x1064] = 0x2088;
    Check(pal4::GameDataMissionEffectObject::ResetEffects(0x1000, reset) ==
              0x2044 &&
              reset.calls[0] == "clear for reset 4096" &&
              reset.calls[1] == "destroy record 8192" &&
              reset.calls[2] == "destroy record 8260" &&
              reset.words[0x1064] == 0x2000,
          "reset returns final record destructor result and rewinds end");
    Runtime empty_reset;
    empty_reset.words[0x1060] = 0x2000;
    empty_reset.words[0x1064] = 0x2000;
    Check(pal4::GameDataMissionEffectObject::ResetEffects(0x1000,
                                                          empty_reset) ==
              empty_reset.reset_result &&
              empty_reset.calls[0] == "clear for reset 4096" &&
              empty_reset.calls.size() == 2,
          "empty reset returns effect cleanup result");
}

}  // namespace

int main() {
    try {
        CheckCreate();
        CheckDestroy();
        CheckStorageAndReset();
        std::cout << "game_data_mission_effect_object_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
