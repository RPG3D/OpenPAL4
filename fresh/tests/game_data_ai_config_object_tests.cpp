#include "pal4/game_data_ai_config_object.h"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataEquipEffectObjectRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x1000;
    std::uint32_t destruction_result = 0xface;

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
        calls.push_back("word " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    void EffectConstruct(std::uint32_t object, std::uint32_t name) override {
        calls.push_back("construct " + std::to_string(object) + " " +
                        std::to_string(name));
    }
    void ClearEffectData(std::uint32_t object) override {
        calls.push_back("clear effect " + std::to_string(object));
    }
    void ProcessElements(std::uint32_t array) override {
        calls.push_back("process " + std::to_string(array));
    }
    void DestroyElement(std::uint32_t element, std::uint8_t flags) override {
        calls.push_back("destroy " + std::to_string(element) + " " +
                        std::to_string(flags));
    }
    void DestroyTextRecord(std::uint32_t text) override {
        calls.push_back("destroy equip text " + std::to_string(text));
    }
    void ResetRecordPointers(std::uint32_t element) override {
        calls.push_back("reset record " + std::to_string(element));
    }
    void SetErrorFlag(std::uint32_t text, std::uint8_t flag) override {
        calls.push_back("reset text " + std::to_string(text) + " " +
                        std::to_string(flag));
    }
    std::uint32_t EffectDestruct(std::uint32_t object) override {
        calls.push_back("destruct effect " + std::to_string(object));
        return destruction_result;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected runtime call was omitted");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

std::string EquipTextCall(std::uint32_t record, std::uint32_t index) {
    return "destroy equip text " +
           std::to_string(record + 152 + index * 16);
}

void CheckCreation() {
    Runtime runtime;
    Check(pal4::GameDataAiConfigObject::Create(0x1234, runtime) == 0x1000,
          "allocation address is returned");
    Check(runtime.calls.front() == "allocate 140" &&
              runtime.calls[1] == "construct 4096 4660" &&
              runtime.calls.back() == "word 4096 8668404",
          "allocation, base construction and derived vtable order");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x34,
              "each embedded text starts with the input low byte");
        for (std::uint32_t word_offset : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + word_offset] == 0,
                  "each embedded text has zeroed pointer, size and capacity");
    }
    Check(runtime.calls.size() == 15, "constructor has exact write count");

    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataAiConfigObject::Create(7, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "failed allocation has no constructor side effects");
}

void CheckMonsterEffectCreation() {
    Runtime runtime;
    Check(pal4::GameDataMonsterEffectObject::Create(0x1234, runtime) ==
              0x1000 &&
              runtime.calls.front() == "allocate 140" &&
              runtime.calls[1] == "construct 4096 4660" &&
              runtime.words[0x1000] == 0x8445ec &&
              runtime.calls.size() == 15,
          "Monster Effect allocation, base construction and vtable match IDA");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x34,
              "Monster Effect text seed uses input pointer low byte");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + step] == 0,
                  "Monster Effect text pointer triplet is cleared");
    }
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataMonsterEffectObject::Create(0x1234, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "Monster Effect allocation failure has no construction side effects");
}

void CheckDestruction() {
    Runtime runtime;
    runtime.words[0x1000 + 96] = 0x2000;
    runtime.words[0x1000 + 112] = 0x3000;
    runtime.words[0x1000 + 116] = 0x3078;
    runtime.words[0x1000 + 128] = 0x4000;
    Check(pal4::GameDataAiConfigObject::Destroy(0x1000, runtime) ==
              runtime.destruction_result,
          "base destructor result is forwarded");
    Check(runtime.calls.front() == "word 4096 8668404" &&
              At(runtime, "clear effect 4096") < At(runtime, "process 4204") &&
              At(runtime, "process 4204") < At(runtime, "free 16384") &&
              At(runtime, "free 16384") < At(runtime, "destroy 12288 0") &&
              At(runtime, "destroy 12288 0") < At(runtime, "destroy 12348 0") &&
              At(runtime, "destroy 12348 0") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "free 8192") &&
              runtime.calls.back() == "destruct effect 4096",
          "three arrays and their members are released in IDA order");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u, 120u,
                                 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "all derived array pointers are reset");

    Runtime empty;
    Check(pal4::GameDataAiConfigObject::Destroy(0x1000, empty) ==
              empty.destruction_result &&
              std::none_of(empty.calls.begin(), empty.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("destroy ");
                           }),
          "empty arrays skip element destruction");
}

void CheckStorage() {
    Runtime runtime;
    runtime.words[0x1004] = 0x2000;
    runtime.words[0x1008] = 0x2078;
    Check(pal4::GameDataAiConfigStorage::ClearRecords(0x1000, runtime) == 0 &&
              runtime.calls[0] == "reset record 8192" &&
              runtime.calls[1] == "reset record 8252" &&
              runtime.calls[2] == "free 8192",
          "record cleanup uses the 0x4cd5b0 operation in forward order");
    for (std::uint32_t offset : {4u, 8u, 12u})
        Check(runtime.words[0x1000 + offset] == 0,
              "record array pointers are reset");

    runtime = Runtime{};
    runtime.words[0x1004] = 0x2000;
    runtime.words[0x1008] = 0x2078;
    runtime.words[0x1014] = 0x3000;
    Check(pal4::GameDataAiConfigStorage::ClearFlags(0x1000, runtime) == 0 &&
              runtime.calls[0] == "process 4096" &&
              runtime.calls[1] == "free 12288" &&
              At(runtime, "reset text 8228 1") <
                  At(runtime, "reset text 8212 1") &&
              At(runtime, "reset text 8212 1") <
                  At(runtime, "reset text 8192 1") &&
              At(runtime, "reset text 8192 1") <
                  At(runtime, "reset text 8288 1") &&
              At(runtime, "reset text 8252 1") < At(runtime, "free 8192"),
          "each 60-byte record releases three text fields in reverse order");
    for (std::uint32_t offset : {4u, 8u, 12u, 20u, 24u, 28u})
        Check(runtime.words[0x1000 + offset] == 0,
              "both storage arrays are reset");

    Runtime empty;
    Check(pal4::GameDataAiConfigStorage::ClearFlags(0x1000, empty) == 0 &&
              std::none_of(empty.calls.begin(), empty.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("reset text ");
                           }),
          "empty storage has no text records");
}

void CheckReset() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2050;
    Check(pal4::GameDataAiConfigObject::ResetEffects(0x1000, runtime) ==
              0x2050 &&
              runtime.calls == std::vector<std::string>{
                                   "clear effect 4096", "word 4196 8192"} &&
              runtime.words[0x1064] == 0x2000,
          "reset returns old end and sets it to the first element");

    runtime.calls.clear();
    runtime.words[0x1064] = 0x205c;
    Check(pal4::GameDataUpgradeEffectObject::ResetEffects(0x1000, runtime) ==
              0x205c &&
              runtime.calls == std::vector<std::string>{
                                   "clear effect 4096", "word 4196 8192"} &&
              runtime.words[0x1064] == 0x2000,
          "UpgradeParam reset returns the previous 92-byte array end");
}

void CheckDropObjectCreation() {
    Runtime runtime;
    Check(pal4::GameDataDropEffectObject::Create(0x1234, runtime) ==
              0x1000 &&
              runtime.calls.front() == "allocate 140" &&
              runtime.calls[1] == "construct 4096 4660" &&
              runtime.words[0x1000] == 0x844554,
          "DropProperty uses the shared Effect layout and its own vtable");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x34,
              "all three DropProperty text seeds use name low byte");
        for (std::uint32_t word_offset : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + word_offset] == 0,
                  "DropProperty array triplets start empty");
    }
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataDropEffectObject::Create(7, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "failed DropProperty allocation has no constructor effects");
}

void CheckDropObjectReset() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x200c;
    Check(pal4::GameDataDropEffectObject::ResetEffects(0x1000, runtime) ==
              0x200c &&
              runtime.calls == std::vector<std::string>{
                                   "clear effect 4096", "word 4196 8192"} &&
              runtime.words[0x1064] == 0x2000,
          "DropProperty reset returns prior scalar end and clears effects");
}

void CheckDropObjectDestruction() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1070] = 0x3000;
    runtime.words[0x1074] = 0x3078;
    runtime.words[0x1080] = 0x4000;
    Check(pal4::GameDataDropEffectObject::Destroy(0x1000, runtime) ==
              runtime.destruction_result &&
              runtime.words[0x1000] == 0x844554 &&
              runtime.calls.front() == "word 4096 8668500" &&
              At(runtime, "clear effect 4096") < At(runtime, "process 4204") &&
              At(runtime, "process 4204") < At(runtime, "free 16384") &&
              At(runtime, "free 16384") < At(runtime, "destroy 12288 0") &&
              At(runtime, "destroy 12288 0") < At(runtime, "destroy 12348 0") &&
              At(runtime, "destroy 12348 0") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "free 8192") &&
              runtime.calls.back() == "destruct effect 4096",
          "DropProperty destroys records and arrays before Effect base");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u, 120u,
                                 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "all DropProperty array pointers are reset");
}

void CheckPropertyObject() {
    Runtime runtime;
    Check(pal4::GameDataAiPropertyObject::Create(0x2255, runtime) == 0x1000 &&
              runtime.words[0x1000] == 0x844514 &&
              runtime.bytes[0x105c] == 0x55 &&
              runtime.bytes[0x106c] == 0x55 &&
              runtime.bytes[0x107c] == 0x55,
          "AiProperty allocates the shared layout with its own vtable");
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataAiPropertyObject::Create(0x55, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "AiProperty allocation failure has no side effects");

    runtime.calls.clear();
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1070] = 0x3000;
    runtime.words[0x1074] = 0x3078;
    runtime.words[0x1080] = 0x4000;
    Check(pal4::GameDataAiPropertyObject::Destroy(0x1000, runtime) ==
              runtime.destruction_result &&
              runtime.calls.front() == "word 4096 8668436" &&
              At(runtime, "free 16384") < At(runtime, "destroy 12288 0") &&
              At(runtime, "destroy 12288 0") <
                  At(runtime, "destroy 12348 0") &&
              At(runtime, "destroy 12348 0") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "free 8192") &&
              runtime.calls.back() == "destruct effect 4096",
          "AiProperty releases arrays and Effect base in original order");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u, 120u,
                                 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "AiProperty destructor zeroes all array pointers");

    Runtime reset;
    reset.words[0x1060] = 0x2000;
    reset.words[0x1064] = 0x2010;
    Check(pal4::GameDataAiPropertyObject::ResetEffects(0x1000, reset) ==
              0x2010 &&
              reset.words[0x1064] == 0x2000 &&
              reset.calls == std::vector<std::string>{
                                 "clear effect 4096", "word 4196 8192"},
          "AiProperty reset returns previous end after clearing effects");
}

struct UpgradeNameRuntime final : pal4::IGameDataUpgradeEffectNameRuntime {
    std::vector<std::string> calls;
    bool allocated = true;
    std::uint8_t seed = 0;
    std::uint32_t object = 0;
    std::string effect_name;

    bool PrepareTemporaryName(std::uint8_t input_seed,
                               std::string_view name) override {
        seed = input_seed;
        Check(name == "ITEM_ID", "effect name literal");
        calls.emplace_back("prepare");
        return allocated;
    }
    void CreateEffect(std::uint32_t input_object,
                      std::string_view temporary_name,
                      std::uint32_t flags) override {
        Check(flags == 0, "ITEM_ID effect uses zero flags");
        object = input_object;
        effect_name = temporary_name;
        calls.emplace_back("create");
    }
    void ReleaseTemporaryName() override { calls.emplace_back("release"); }
};

void CheckUpgradeEffectObject() {
    UpgradeNameRuntime names;
    Check(pal4::GameDataUpgradeEffectObject::InitializeItemId(
              0x1234, names) == 1 &&
              names.seed == 0x34 && names.object == 0x1234 &&
              names.effect_name == "ITEM_ID" &&
              names.calls == std::vector<std::string>{"prepare", "create",
                                                     "release"},
          "ITEM_ID wrapper creates effect before releasing temporary text");
    UpgradeNameRuntime failed_name;
    failed_name.allocated = false;
    Check(pal4::GameDataUpgradeEffectObject::InitializeItemId(
              0x3456, failed_name) == 1 &&
              failed_name.seed == 0x56 && failed_name.effect_name.empty() &&
              failed_name.calls ==
                  std::vector<std::string>{"prepare", "create"},
          "failed temporary allocation still calls Effect with empty text");

    Runtime runtime;
    Check(pal4::GameDataUpgradeEffectObject::Create(0x1234, runtime) ==
              0x1000 &&
              runtime.words[0x1000] == 0x844534 &&
              runtime.calls.front() == "allocate 140" &&
              runtime.calls[1] == "construct 4096 4660",
          "upgrade Effect allocates shared layout and sets its vtable");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x34,
              "upgrade text records use the input low byte");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + step] == 0,
                  "upgrade text pointer triplets start empty");
    }
    Runtime no_memory;
    no_memory.allocation = 0;
    Check(pal4::GameDataUpgradeEffectObject::Create(7, no_memory) == 0 &&
              no_memory.calls ==
                  std::vector<std::string>{"allocate 140"},
          "upgrade Effect allocation failure has no member writes");

    runtime.calls.clear();
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1070] = 0x3000;
    runtime.words[0x1074] = 0x3078;
    runtime.words[0x1080] = 0x4000;
    Check(pal4::GameDataUpgradeEffectObject::Destroy(0x1000, runtime) ==
              runtime.destruction_result &&
              runtime.calls.front() == "word 4096 8668468" &&
              At(runtime, "clear effect 4096") <
                  At(runtime, "process 4204") &&
              At(runtime, "process 4204") < At(runtime, "free 16384") &&
              At(runtime, "free 16384") < At(runtime, "destroy 12288 0") &&
              At(runtime, "destroy 12288 0") <
                  At(runtime, "destroy 12348 0") &&
              At(runtime, "destroy 12348 0") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "free 8192") &&
              runtime.calls.back() == "destruct effect 4096",
          "upgrade Effect destructor releases three arrays in IDA order");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u, 120u,
                                 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "upgrade Effect destructor clears all array pointers");
}

void CheckEquipRelationObject() {
    Runtime runtime;
    Check(pal4::GameDataEquipRelationEffectObject::Create(
              0x1234, runtime) == 0x1000 &&
              runtime.words[0x1000] == 0x84458c &&
              runtime.calls.front() == "allocate 140" &&
              runtime.calls[1] == "construct 4096 4660",
          "EquipRelation uses 140-byte Effect object and its own vtable");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x34,
              "EquipRelation text seed comes from name low byte");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + step] == 0,
                  "EquipRelation text triplets begin empty");
    }
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataEquipRelationEffectObject::Create(
              0x1234, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "failed EquipRelation allocation leaves base and members untouched");

    runtime.calls.clear();
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2024;
    Check(pal4::GameDataEquipRelationEffectObject::ResetEffects(
              0x1000, runtime) == 0x2024 &&
              runtime.words[0x1064] == 0x2000 &&
              runtime.calls.front() == "clear effect 4096",
          "EquipRelation reset clears effects and returns old record end");

    runtime.calls.clear();
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1070] = 0x3000;
    runtime.words[0x1074] = 0x3078;
    runtime.words[0x1080] = 0x4000;
    Check(pal4::GameDataEquipRelationEffectObject::Destroy(
              0x1000, runtime) == runtime.destruction_result &&
              runtime.calls.front() ==
                  "word 4096 " + std::to_string(0x84458c) &&
              At(runtime, "clear effect 4096") <
                  At(runtime, "process 4204") &&
              At(runtime, "process 4204") < At(runtime, "free 16384") &&
              At(runtime, "free 16384") < At(runtime, "destroy 12288 0") &&
              At(runtime, "destroy 12288 0") <
                  At(runtime, "destroy 12348 0") &&
              At(runtime, "destroy 12348 0") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "free 8192") &&
              runtime.calls.back() == "destruct effect 4096",
          "EquipRelation destructor keeps member and Effect cleanup order");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u, 120u,
                                 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "EquipRelation destructor clears all three array triplets");
}

void CheckEquipObject() {
    Runtime runtime;
    Check(pal4::GameDataEquipEffectObject::Create(0x1234, runtime) ==
              0x1000 &&
              runtime.words[0x1000] == 0x8445ac &&
              runtime.calls.front() == "allocate 140",
          "Equip factory initializes its own Effect vtable");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x1000 + offset] == 0x34,
              "Equip embedded text seed uses name low byte");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[0x1000 + offset + step] == 0,
                  "Equip text records begin empty");
    }
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataEquipEffectObject::Create(7, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "Equip allocation failure performs no initialization");

    runtime.calls.clear();
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2000 + 800;
    pal4::GameDataEquipEffectObject::ResetEffects(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x2000 &&
              runtime.calls.front() == "clear effect 4096" &&
              At(runtime, EquipTextCall(0x2000, 12)) <
                  At(runtime, EquipTextCall(0x2000, 0)) &&
              At(runtime, EquipTextCall(0x2000, 0)) <
                  At(runtime, EquipTextCall(0x2000 + 400, 12)) &&
              At(runtime, EquipTextCall(0x2000 + 400, 0)) <
                  At(runtime, "word 4196 8192"),
          "Equip reset destroys each record's 13 text fields in reverse");

    runtime.calls.clear();
    runtime.words[0x1064] = 0x2000 + 800;
    runtime.words[0x1070] = 0x3000;
    runtime.words[0x1074] = 0x3000 + 120;
    runtime.words[0x1080] = 0x4000;
    Check(pal4::GameDataEquipEffectObject::Destroy(0x1000, runtime) ==
              runtime.destruction_result &&
              runtime.calls.front() ==
                  "word 4096 " + std::to_string(0x8445ac) &&
              At(runtime, "clear effect 4096") <
                  At(runtime, EquipTextCall(0x2000, 12)) &&
              At(runtime, EquipTextCall(0x2000 + 400, 0)) <
                  At(runtime, "process 4204") &&
              At(runtime, "process 4204") < At(runtime, "free 16384") &&
              At(runtime, "free 16384") < At(runtime, "destroy 12288 0") &&
              At(runtime, "destroy 12348 0") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "free 8192") &&
              runtime.calls.back() == "destruct effect 4096",
          "Equip destructor releases records, fields and Effect in order");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u, 120u,
                                 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "Equip destructor zeroes all array triplets");

    Runtime storage;
    storage.words[0x5004] = 0x6000;
    storage.words[0x5008] = 0x6000 + 800;
    Check(pal4::GameDataEquipRecordStorage::Clear(0x5000, storage) == 0 &&
              storage.calls.front() == EquipTextCall(0x6000, 12) &&
              At(storage, EquipTextCall(0x6000 + 400, 0)) <
                  At(storage, "free 24576") &&
              storage.words[0x5004] == 0 &&
              storage.words[0x5008] == 0 &&
              storage.words[0x500c] == 0,
          "Equip 400-byte array cleanup destroys elements before free");
}

}  // namespace

int main() {
    try {
        CheckCreation();
        CheckMonsterEffectCreation();
        CheckDestruction();
        CheckStorage();
        CheckReset();
        CheckDropObjectCreation();
        CheckDropObjectReset();
        CheckDropObjectDestruction();
        CheckPropertyObject();
        CheckUpgradeEffectObject();
        CheckEquipRelationObject();
        CheckEquipObject();
        std::cout << "game_data_ai_config_object_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
