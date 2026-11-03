#include "pal4/game_data_role_table_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataRoleTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 140, "Role table allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void ConstructEffect(std::uint32_t object,
                         std::uint32_t name) override {
        Check(object == 0x2000 && name == 0x1234,
              "base effect constructor arguments");
        calls.emplace_back("construct");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte:" + std::to_string(address));
    }
    void ClearEffectData(std::uint32_t object) override {
        Check(object == 0x2000, "clear effect object");
        calls.emplace_back("clear");
    }
    void CleanupTextRecords(std::uint32_t record) override {
        calls.push_back("text:" + std::to_string(record));
    }
    void ProcessElements(std::uint32_t elements) override {
        Check(elements == 0x2000 + 108, "element collection offset");
        calls.emplace_back("process");
    }
    void DestroyRecord(std::uint32_t record) override {
        calls.push_back("record:" + std::to_string(record));
    }
    void DestroyAuxRecord(std::uint32_t record) override {
        calls.push_back("aux:" + std::to_string(record));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t DestructEffect(std::uint32_t object) override {
        Check(object == 0x2000, "base effect destructor object");
        calls.emplace_back("destruct");
        return 0x55;
    }
};

void CheckConstruct() {
    Runtime runtime;
    using Lifecycle = pal4::GameDataRoleTableLifecycle;
    Check(Lifecycle::Construct(0x1234, runtime) == 0x2000 &&
              runtime.calls.size() == 15 &&
              runtime.calls[0] == "allocate" &&
              runtime.calls[1] == "construct" &&
              runtime.calls.back() == "word:8192",
          "object allocation and vtable assignment order");
    for (const std::uint32_t offset : {92u, 108u, 124u})
        Check(runtime.bytes[0x2000 + offset] == 0x34,
              "three embedded byte flags capture low name byte");
    for (const std::uint32_t offset : {
             96u, 100u, 104u, 112u, 116u, 120u,
             128u, 132u, 136u})
        Check(runtime.words[0x2000 + offset] == 0,
              "three array triplets initialized empty");
    Check(runtime.words[0x2000] == 0x84464c,
          "Role table native vtable");
    Runtime fail;
    fail.allocation = 0;
    Check(Lifecycle::Construct(0x1234, fail) == 0 &&
              fail.calls == std::vector<std::string>{"allocate"},
          "allocation failure skips effect construction");
}

void CheckDestroy() {
    Runtime runtime;
    runtime.words[0x2000 + 96] = 0x5000;
    runtime.words[0x2000 + 100] = 0x5000 + 640;
    runtime.words[0x2000 + 104] = 0x6000;
    runtime.words[0x2000 + 112] = 0x7000;
    runtime.words[0x2000 + 116] = 0x7000 + 120;
    runtime.words[0x2000 + 120] = 0x8000;
    runtime.words[0x2000 + 128] = 0x9000;
    runtime.words[0x2000 + 132] = 0x9010;
    runtime.words[0x2000 + 136] = 0x9020;
    Check(pal4::GameDataRoleTableLifecycle::Destroy(0x2000, runtime) ==
              0x55 && runtime.calls[0] == "word:8192" &&
              runtime.calls[1] == "clear" &&
              runtime.calls[2] == "text:20624" &&
              runtime.calls[3] == "text:20944" &&
              runtime.calls[5] == "process" &&
              runtime.calls[6] == "free:36864" &&
              runtime.calls.back() == "destruct",
          "destructor cleans records before pointer and auxiliary arrays");
    const auto find = [&runtime](const std::string& value) {
        for (std::size_t i = 0; i < runtime.calls.size(); ++i)
            if (runtime.calls[i] == value) return i;
        return runtime.calls.size();
    };
    Check(find("aux:28672") < find("aux:28732") &&
              find("aux:28732") < find("free:28672") &&
              find("free:28672") < find("free:20480") &&
              find("free:20480") < find("destruct"),
          "auxiliary records and buffers freed in native order");
    for (const std::uint32_t offset : {
             96u, 100u, 104u, 112u, 116u, 120u,
             128u, 132u, 136u})
        Check(runtime.words[0x2000 + offset] == 0,
              "destructor clears all three triplets");
}

void CheckArrays() {
    Runtime runtime;
    runtime.words[0x3000 + 4] = 0x4000;
    runtime.words[0x3000 + 8] = 0x4000 + 640;
    runtime.words[0x3000 + 12] = 0x5000;
    Check(pal4::GameDataRoleTableLifecycle::DestroyRecordArray(
              0x3000, runtime) == 0 &&
              runtime.calls[0] == "record:16384" &&
              runtime.calls[1] == "record:16704" &&
              runtime.calls[2] == "free:16384" &&
              runtime.words[0x3004] == 0 &&
              runtime.words[0x3008] == 0 &&
              runtime.words[0x300c] == 0,
          "record array destroys each 320-byte entry then frees storage");
    Runtime empty;
    Check(pal4::GameDataRoleTableLifecycle::DestroyRecordArray(
              0x3000, empty) == 0 &&
              empty.calls.front() == "free:0",
          "empty array still forwards null pointer to free");

    Runtime loaded;
    loaded.words[0x2000 + 96] = 0x6000;
    loaded.words[0x2000 + 100] = 0x6000 + 640;
    pal4::GameDataRoleTableLifecycle::ClearLoadedRecords(0x2000, loaded);
    Check(loaded.calls[0] == "clear" &&
              loaded.calls[1] == "record:24576" &&
              loaded.calls[2] == "record:24896" &&
              loaded.words[0x2000 + 100] == 0x6000 &&
              loaded.words[0x2000 + 96] == 0x6000,
          "clear loaded records retains allocated capacity");

    Runtime range;
    pal4::GameDataRoleTableLifecycle::DestroyRecordRange(
        0x9000, 0x9000 + 960, range);
    Check(range.calls == std::vector<std::string>{
              "record:36864", "record:37184", "record:37504"},
          "range cleanup visits each 320-byte record in order");
    range.calls.clear();
    pal4::GameDataRoleTableLifecycle::DestroyRecordRange(
        0x9000, 0x9000, range);
    Check(range.calls.empty(), "empty range skips record destruction");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckDestroy();
        CheckArrays();
        std::cout << "game_data_role_table_lifecycle_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
