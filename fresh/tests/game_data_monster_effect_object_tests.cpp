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

struct Runtime final : pal4::IGameDataMonsterEffectRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed_texts;
    bool refill_after_processing = false;

    std::uint32_t Allocate(std::uint32_t) override { return 0; }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void EffectConstruct(std::uint32_t, std::uint32_t) override {}
    void ClearEffectData(std::uint32_t object) override {
        calls.push_back("clear " + std::to_string(object));
    }
    void ProcessElements(std::uint32_t array) override {
        calls.push_back("process " + std::to_string(array));
        if (refill_after_processing)
            words[array - 8] = words[array - 12] + 596;
    }
    void DestroyElement(std::uint32_t element, std::uint8_t flags) override {
        Check(flags == 0, "field destructor receives zero flags");
        calls.push_back("field " + std::to_string(element));
    }
    void ResetRecordPointers(std::uint32_t) override {}
    void SetErrorFlag(std::uint32_t, std::uint8_t) override {}
    std::uint32_t EffectDestruct(std::uint32_t object) override {
        calls.push_back("base " + std::to_string(object));
        return 0xbeef;
    }
    void DestroyText(std::uint32_t address) override {
        destroyed_texts.push_back(address);
        calls.push_back("text " + std::to_string(address));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected lifecycle operation missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckDestroy() {
    Runtime runtime;
    runtime.refill_after_processing = true;
    runtime.words[0x1000 + 96] = 0x2000;
    runtime.words[0x1000 + 100] = 0x2000 + 2 * 596;
    runtime.words[0x1000 + 112] = 0x3000;
    runtime.words[0x1000 + 116] = 0x3000 + 2 * 60;
    runtime.words[0x1000 + 128] = 0x4000;
    Check(pal4::GameDataMonsterEffectObject::Destroy(0x1000, runtime) ==
              0xbeef &&
              runtime.words[0x1000] == 0x8445ec &&
              runtime.destroyed_texts.size() == 33,
          "Monster Effect destructor forwards base result and runs both passes");
    Check(runtime.destroyed_texts[0] == 0x2000 + 336 + 232 &&
              runtime.destroyed_texts[11] == 0x2000 + 596 + 336 + 232 &&
              runtime.destroyed_texts[22] == 0x2000 + 336 + 232,
          "first pass handles two subrecords, second pass rereads one");
    Check(At(runtime, "clear 4096") <
              At(runtime, "text " + std::to_string(0x2000 + 336 + 232)) &&
              At(runtime, "process 4204") < At(runtime, "free 16384") &&
              At(runtime, "free 16384") < At(runtime, "field 12288") &&
              At(runtime, "field 12348") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "free 8192") &&
              runtime.calls.back() == "base 4096",
          "Effect, record, auxiliary array, fields and base cleanup order");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u,
                                 120u, 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "Monster Effect clears all three array triplets");
}

void CheckResetAndStorage() {
    Runtime reset;
    reset.words[0x1000 + 96] = 0x2000;
    reset.words[0x1000 + 100] = 0x2000 + 2 * 596;
    pal4::GameDataMonsterEffectObject::ResetEffects(0x1000, reset);
    Check(reset.destroyed_texts.size() == 22 &&
              reset.destroyed_texts[0] == 0x2000 + 336 + 232 &&
              reset.destroyed_texts[11] ==
                  0x2000 + 596 + 336 + 232 &&
              reset.words[0x1000 + 100] == 0x2000 &&
              reset.calls.front() == "clear 4096",
          "reset clears Effect then destroys every live 596-byte record");

    Runtime storage;
    storage.words[0x5004] = 0x6000;
    storage.words[0x5008] = 0x6000 + 2 * 596;
    Check(pal4::GameDataMonsterRecordStorage::Clear(0x5000, storage) == 0 &&
              storage.destroyed_texts.size() == 22 &&
              storage.destroyed_texts[0] == 0x6000 + 336 + 232 &&
              storage.destroyed_texts[11] ==
                  0x6000 + 596 + 336 + 232 &&
              At(storage, "text " +
                              std::to_string(0x6000 + 596 + 336 + 72)) <
                  At(storage, "free 24576") &&
              storage.words[0x5004] == 0 && storage.words[0x5008] == 0 &&
              storage.words[0x500c] == 0,
          "record storage destroys before free and zeroes its triplet");
}

}  // namespace

int main() {
    try {
        CheckDestroy();
        CheckResetAndStorage();
        std::cout << "game_data_monster_effect_object_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
