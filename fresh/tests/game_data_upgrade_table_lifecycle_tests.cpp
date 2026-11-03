#include "pal4/game_data_upgrade_table_lifecycle.h"

#include <algorithm>
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

struct Runtime final : pal4::IGameDataEffectTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 140, "UpgradeData object size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void ConstructEffect(std::uint32_t object,
                         std::uint32_t name) override {
        Check(object == 0x2000 && name == 0x1234,
              "Effect constructor address and source");
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
        Check(object == 0x2000, "Effect clear target");
        calls.emplace_back("clear");
    }
    void CleanupTextRecords(std::uint32_t address) override {
        calls.push_back("text:" + std::to_string(address));
    }
    void ProcessElements(std::uint32_t elements) override {
        Check(elements == 0x2000 + 108, "Effect binding offset");
        calls.emplace_back("process");
    }
    void DestroyRecord(std::uint32_t address) override {
        calls.push_back("record:" + std::to_string(address));
    }
    void DestroyAuxRecord(std::uint32_t address) override {
        calls.push_back("aux:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t DestructEffect(std::uint32_t object) override {
        Check(object == 0x2000, "Effect destructor target");
        calls.emplace_back("destruct");
        return 0x5a;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected lifecycle call missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckConstruct() {
    Runtime runtime;
    using Lifecycle = pal4::GameDataUpgradeTableLifecycle;
    Check(Lifecycle::Construct(0x1234, runtime) == 0x2000 &&
              runtime.calls.size() == 15 &&
              runtime.calls[0] == "allocate" &&
              runtime.calls[1] == "construct" &&
              runtime.calls.back() == "word:8192" &&
              runtime.words[0x2000] == 0x84466c,
          "UpgradeData constructor size, order and vtable");
    for (std::uint32_t group : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x2000 + group] == 0x34,
              "embedded flag receives low name byte");
        for (std::uint32_t offset : {4u, 8u, 12u})
            Check(runtime.words[0x2000 + group + offset] == 0,
                  "embedded array triplet begins empty");
    }
    Runtime fail;
    fail.allocation = 0;
    Check(Lifecycle::Construct(0x1234, fail) == 0 &&
              fail.calls == std::vector<std::string>{"allocate"},
          "allocation failure skips constructor and writes");
}

void CheckDestroy() {
    Runtime runtime;
    runtime.words[0x2000 + 96] = 0x5000;
    runtime.words[0x2000 + 100] = 0x5000 + 320;
    runtime.words[0x2000 + 104] = 0x6000;
    runtime.words[0x2000 + 112] = 0x7000;
    runtime.words[0x2000 + 116] = 0x7000 + 120;
    runtime.words[0x2000 + 120] = 0x8000;
    runtime.words[0x2000 + 128] = 0x9000;
    runtime.words[0x2000 + 132] = 0x9010;
    runtime.words[0x2000 + 136] = 0x9020;
    Check(pal4::GameDataUpgradeTableLifecycle::Destroy(0x2000, runtime) ==
              0x5a && runtime.calls[0] == "word:8192" &&
              runtime.calls[1] == "clear" &&
              runtime.calls[2] == "word:8292" &&
              At(runtime, "process") < At(runtime, "free:36864") &&
              At(runtime, "aux:28672") < At(runtime, "aux:28732") &&
              At(runtime, "aux:28732") < At(runtime, "free:28672") &&
              At(runtime, "free:28672") < At(runtime, "free:20480") &&
              runtime.calls.back() == "destruct",
          "UpgradeData cleanup order and result");
    for (const auto& call : runtime.calls)
        Check(!call.starts_with("record:") && !call.starts_with("text:"),
              "UpgradeData destructor skips per-record cleanup");
    for (std::uint32_t offset : {
             96u, 100u, 104u, 112u, 116u, 120u,
             128u, 132u, 136u})
        Check(runtime.words[0x2000 + offset] == 0,
              "UpgradeData destructor zeros all three array triplets");
}

void CheckClear() {
    Runtime runtime;
    runtime.words[0x2000 + 96] = 0x5000;
    runtime.words[0x2000 + 100] = 0x5000 + 320;
    runtime.words[0x2000 + 104] = 0x5000 + 640;
    Check(pal4::GameDataUpgradeTableLifecycle::ClearLoadedRecords(
              0x2000, runtime) == 0x5000 + 320 &&
              runtime.words[0x2000 + 96] == 0x5000 &&
              runtime.words[0x2000 + 100] == 0x5000 &&
              runtime.words[0x2000 + 104] == 0x5000 + 640 &&
              runtime.calls == std::vector<std::string>{
                  "clear", "word:8292"},
          "clear returns old end and retains allocation without record calls");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckDestroy();
        CheckClear();
        std::cout << "game_data_upgrade_table_lifecycle_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
