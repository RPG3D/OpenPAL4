#include "pal4/actor_manager_player_status_texts.h"

#include <algorithm>
#include <array>
#include <cstdint>
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

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t base = pal4::ActorManagerPlayerStatusTexts::kRecords;

struct Runtime final : pal4::IActorManagerPlayerStatusTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> buffers;
    std::vector<std::string> trace;
    std::int32_t index = 0;
    std::uint32_t fail_record = 0u;
    std::uint32_t register_count = 0u;

    std::int32_t ReadSignedWord(std::uint32_t address) override {
        Check(address == actor + 1192u, "actor status offset");
        trace.emplace_back("index");
        return index;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.emplace_back("word:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        trace.emplace_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void SetExceptionMessage(std::uint32_t record,
                             std::string_view text) override {
        Check(record == base && text == "101", "first text assignment");
        trace.emplace_back("message:101");
        words[record + 4u] = 0x2000u;
        buffers[0x2000u] = text;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t value) override {
        Check(record >= base + 16u && record <= base + 48u &&
                  (record - base) % 16u == 0u && value == 0u,
              "later text flag");
        trace.emplace_back("flag:" + std::to_string(record));
        for (const auto offset : {4u, 8u, 12u})
            words[record + offset] = 0u;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(length == 3u && mode == 1u, "three-byte text allocation");
        trace.emplace_back("allocate:" + std::to_string(record));
        if (record == fail_record) return false;
        words[record + 4u] = 0x2000u + (record - base) * 16u;
        return true;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        trace.emplace_back("copy:" + std::string(text));
        buffers[destination] = text;
    }
    void RegisterExit(std::uint32_t callback) override {
        Check(callback == 0x538310u, "exact exit callback");
        ++register_count;
        trace.emplace_back("atexit");
    }
    void ReleaseText(std::uint32_t record) override {
        trace.emplace_back("release:" + std::to_string(record));
        words[record + 4u] = 0u;
    }
};

void InitializationAndLookup() {
    pal4::ActorManagerPlayerStatusTexts texts;
    Runtime runtime;
    const std::array<std::string_view, 4> expected = {
        "101", "103", "106", "105"};
    for (std::int32_t i = 0; i < 4; ++i) {
        runtime.index = i;
        const auto pointer = texts.Lookup(actor, runtime);
        Check(runtime.buffers.at(pointer) == expected[i],
              "status text index mapping");
        Check(runtime.words.at(base + static_cast<std::uint32_t>(i) * 16u + 4u)
                  == pointer,
              "status text record pointer");
    }
    Check(runtime.register_count == 1u, "one-time CRT registration");
    Check(runtime.words.at(base + 8u) == 0u &&
              runtime.words.at(base + 12u) == 0u &&
              runtime.words.at(base + 16u + 8u) == 3u &&
              runtime.bytes.at(base + 32u) == 0u,
          "record initialization and copied text lengths");
    Check(runtime.trace.front() == "byte:" + std::to_string(base) &&
              runtime.trace[4] == "message:101" &&
              runtime.trace[5] == "byte:" + std::to_string(base + 16u) &&
              runtime.trace[6] == "flag:" + std::to_string(base + 16u) &&
              runtime.trace[7] == "allocate:" + std::to_string(base + 16u) &&
              runtime.trace[9] == "copy:103",
          "first and second record initialization order");
    const auto exit = std::find(runtime.trace.begin(), runtime.trace.end(),
                                "atexit");
    const auto first_index = std::find(runtime.trace.begin(),
                                       runtime.trace.end(), "index");
    Check(exit != runtime.trace.end() && exit < first_index,
          "exit registration precedes first status lookup");
}

void BoundsAndFallback() {
    pal4::ActorManagerPlayerStatusTexts texts;
    Runtime runtime;
    runtime.index = 0;
    const auto first = texts.Lookup(actor, runtime);
    for (const auto index : {4, 5, 100}) {
        runtime.index = index;
        Check(texts.Lookup(actor, runtime) == first,
              "status index at least four selects first text");
    }
    runtime.index = -1;
    runtime.words[base - 16u + 4u] = 0x4111u;
    Check(texts.Lookup(actor, runtime) == 0x4111u,
          "negative index preserves 32-bit address arithmetic");
    runtime.words[base - 16u + 4u] = 0u;
    Check(texts.Lookup(actor, runtime) ==
              pal4::ActorManagerPlayerStatusTexts::kEmptyText,
          "null negative-index pointer uses shared empty text");
    Check(runtime.register_count == 1u, "bounds queries do not reinitialize");

    pal4::ActorManagerPlayerStatusTexts failed_texts;
    Runtime failed;
    failed.fail_record = base + 32u;
    failed.index = 2;
    Check(failed_texts.Lookup(actor, failed) ==
              pal4::ActorManagerPlayerStatusTexts::kEmptyText &&
              failed.words.at(base + 32u + 8u) == 0u &&
              failed.register_count == 1u,
          "failed allocation preserves empty record and registers cleanup");
    Check(failed.buffers.size() == 3u,
          "failed allocation skips copy and terminator");
}

void CleanupOrder() {
    pal4::ActorManagerPlayerStatusTexts texts;
    Runtime runtime;
    (void)texts.Lookup(actor, runtime);
    runtime.trace.clear();
    texts.Cleanup(runtime);
    Check(runtime.trace == std::vector<std::string>{
              "release:" + std::to_string(base + 48u),
              "release:" + std::to_string(base + 32u),
              "release:" + std::to_string(base + 16u),
              "release:" + std::to_string(base)},
          "vector destructor releases four records in reverse order");
    Check(texts.Lookup(actor, runtime) ==
              pal4::ActorManagerPlayerStatusTexts::kEmptyText &&
              runtime.register_count == 1u,
          "native guard remains set after exit cleanup");
}

}  // namespace

int main() {
    try {
        InitializationAndLookup();
        BoundsAndFallback();
        CleanupOrder();
        std::cout << "actor_manager_player_status_texts_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
