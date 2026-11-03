#include "pal4/recovered_combat_entries.h"

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

struct BootRuntime final : pal4::IMatFxScriptBootRuntime {
    std::vector<std::string> order;
    std::uint32_t address = 0, value = 1;
    void PrepareScripts() override { order.emplace_back("prepare"); }
    void ExecuteScript(std::string_view path) override {
        Check(path == "gamedata\\MatFX\\Shader\\ltMap.fx", "MatFX path");
        order.emplace_back("execute");
    }
    void CleanupScripts() override { order.emplace_back("cleanup"); }
    std::uint32_t SetupCombatCamera(std::uint32_t owner) override {
        Check(owner == 0x4000u, "camera owner");
        order.emplace_back("camera");
        return 0x8000u;
    }
    void WriteWord(std::uint32_t target, std::uint32_t written) override {
        address = target; value = written;
        order.emplace_back("reset");
    }
};

struct FloatRuntime final : pal4::IActorManagerFloatTaskRuntime {
    std::uint32_t queue = 0;
    float value = 0;
    int Process(std::uint32_t target, float input) override {
        queue = target; value = input;
        return -12;
    }
};

struct ScoreRuntime final : pal4::ICombatEndingScoreElementRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> order;
    std::uint32_t freed = 0;
    std::uint32_t collection = 0;
    std::uint32_t destroyed = 0;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        order.emplace_back("word");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        order.emplace_back("byte");
    }
    void CleanupCollection(std::uint32_t address) override {
        collection = address;
        order.emplace_back("cleanup");
    }
    void Free(std::uint32_t address) override {
        freed = address;
        order.emplace_back("free");
    }
    int DestroyContainer(std::uint32_t address) override {
        destroyed = address;
        order.emplace_back("destroy");
        return -31;
    }
};

struct ConsoleRuntime final : pal4::ICombatConsoleInitThunkRuntime {
    int calls = 0;
    std::uint32_t InitializeCombatConsole() override {
        ++calls;
        return 0x1234u;
    }
};

void BootAndFloat() {
    BootRuntime boot;
    Check(pal4::MatFxScriptBoot::Initialize(0x4000u, boot) == 0x8000u &&
          boot.address == 0x4044u && boot.value == 0u &&
          boot.order == std::vector<std::string>{"prepare", "execute",
              "cleanup", "camera", "reset"},
          "MatFX script/camera/tail order and result");
    FloatRuntime conversion;
    Check(pal4::ActorManagerFloatTask::ConvertAndProcess(
              0x5000u, 1.0 / 10.0, conversion) == -12 &&
          conversion.queue == 0x502cu &&
          conversion.value == static_cast<float>(1.0 / 10.0),
          "double narrows to float at manager+44 and forwards result");
}

void ScoreElementAndThunk() {
    constexpr std::uint32_t kElement = 0x6000u;
    ScoreRuntime score;
    score.words[kElement + 36u] = 0x7654u;
    Check(pal4::CombatEndingScoreElement::ResultWord(kElement, score) ==
          0x7654u, "score element offset 36 accessor");
    score.words[kElement + 24u] = 0x7001u;
    score.bytes[0x7000u] = 2u;
    Check(pal4::CombatEndingScoreElement::Destroy(kElement, score) == -31 &&
          score.words[kElement] == 0x845390u &&
          score.collection == kElement + 4u &&
          score.destroyed == kElement + 4u &&
          score.bytes[0x7000u] == 1u && score.freed == 0u &&
          score.words[kElement + 24u] == 0u &&
          score.words[kElement + 28u] == 0u &&
          score.words[kElement + 32u] == 0u &&
          score.order[1] == "cleanup" &&
          score.order.back() == "destroy",
          "score destructor decrements shared text and destroys container");
    score.order.clear();
    score.words[kElement + 24u] = 0x8001u;
    score.bytes[0x8000u] = 0xffu;
    (void)pal4::CombatEndingScoreElement::Destroy(kElement, score);
    Check(score.freed == 0x8000u, "saturated text header is freed");
    score.freed = 0;
    score.words[kElement + 24u] = 0u;
    (void)pal4::CombatEndingScoreElement::Destroy(kElement, score);
    Check(score.freed == 0u, "empty text is not freed");
    ConsoleRuntime console;
    Check(pal4::CombatConsoleInitThunk::Call(console) == 0x1234u &&
          console.calls == 1, "combat-console thunk forwards one call/result");
}

}  // namespace

int main() {
    try {
        BootAndFloat();
        ScoreElementAndThunk();
        std::cout << "recovered_combat_entries_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
