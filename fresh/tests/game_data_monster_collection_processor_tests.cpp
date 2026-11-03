#include "pal4/game_data_monster_collection_processor.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMonsterCollectionRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> music{0xeeee};
    std::uint32_t database_reads = 0;
    bool fade_sound = true;
    std::uint32_t sound_dispatch_count = 0;

    std::uint32_t GetDatabase() override {
        ++database_reads;
        calls.emplace_back("database");
        return 0x9000;
    }
    bool StopSound(std::uint32_t database, std::uint32_t key) override {
        Check(database == 0x9000, "sound check receives database");
        calls.push_back("stop sound " + std::to_string(key));
        return key == 1 || key == 3;
    }
    std::uint32_t PlaySound(std::uint32_t database,
                            std::uint32_t key) override {
        Check(database == 0x9000, "sound lookup receives database");
        calls.push_back("play sound " + std::to_string(key));
        return key + 0x100;
    }
    std::uint32_t ResolveSound(std::uint32_t token) override {
        calls.emplace_back("resolve sound");
        return token == 0x101 ? 0x1000 : 0;
    }
    bool StopMusic(std::uint32_t database, std::uint32_t key) override {
        Check(database == 0x9000, "music check receives database");
        calls.push_back("stop music " + std::to_string(key));
        return key == 2 || key == 5;
    }
    std::uint32_t PlayMusic(std::uint32_t database,
                            std::uint32_t key) override {
        Check(database == 0x9000, "music lookup receives database");
        calls.push_back("play music " + std::to_string(key));
        return key + 0x200;
    }
    std::uint32_t ResolveMusic(std::uint32_t token) override {
        calls.emplace_back("resolve music");
        return token == 0x202 ? 0x2000 : token == 0x205 ? 0x5000 : 0;
    }
    bool FadeIn(std::uint32_t record) override {
        calls.push_back("fade " + std::to_string(record));
        return record != 0x1000 || fade_sound;
    }
    std::uint32_t CollectionEnd(std::uint32_t collection) override {
        Check(collection == 0xa000, "end lookup receives collection");
        calls.emplace_back("collection end");
        return collection + static_cast<std::uint32_t>(music.size() * 4);
    }
    std::uint8_t DispatchSound(std::uint32_t collection,
                               std::uint32_t collection_end,
                               std::uint32_t value) override {
        Check(collection == 0xa000 &&
                  collection_end == collection + music.size() * 4 &&
                  value == 0xaaaa,
              "sound dispatch receives current collection end and record value");
        ++sound_dispatch_count;
        calls.emplace_back("dispatch sound");
        return 0x41;
    }
    std::uint32_t ReadRecordValue(std::uint32_t record) override {
        calls.emplace_back("read record");
        return record == 0x1000 ? 0xaaaa
               : record == 0x2000 ? 0xbbbb
                                  : 0xcccc;
    }
    std::uint32_t InsertMusicAtFront(std::uint32_t collection,
                                     std::uint32_t value) override {
        Check(collection == 0xa000, "music insertion receives collection");
        calls.emplace_back("insert music");
        music.insert(music.begin(), value);
        return value == 0xbbbb ? 0x42 : 0x3456;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected collection operation missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckAllBranches() {
    Runtime runtime;
    constexpr std::array<std::uint32_t, 5> keys{1, 2, 3, 4, 5};
    Check(pal4::GameDataMonsterCollectionProcessor::Process(
              keys, 0xa000, runtime) == 0x56,
          "last music insertion determines returned low byte");
    Check(runtime.database_reads == 12 &&
              runtime.music == std::vector<std::uint32_t>{
                                   0xcccc, 0xbbbb, 0xeeee},
          "five keys cause expected GameDB rereads and front insertions");
    Check(At(runtime, "stop sound 1") < At(runtime, "play sound 1") &&
              At(runtime, "play sound 1") < At(runtime, "dispatch sound") &&
              At(runtime, "stop sound 2") < At(runtime, "stop music 2") &&
              At(runtime, "stop music 2") < At(runtime, "insert music") &&
              At(runtime, "stop sound 5") < At(runtime, "play music 5"),
          "sound priority and music fallback follow IDA order");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "stop music 1") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "stop music 3") == runtime.calls.end(),
          "matched sound keys never enter music branch");
}

void CheckFadeFailure() {
    Runtime runtime;
    runtime.fade_sound = false;
    constexpr std::array<std::uint32_t, 5> keys{4, 4, 4, 4, 1};
    Check(pal4::GameDataMonsterCollectionProcessor::Process(
              keys, 0xa000, runtime) == 0 &&
              runtime.music == std::vector<std::uint32_t>{0xeeee} &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "dispatch sound") == runtime.calls.end(),
          "fade failure suppresses sound dispatch and returns false");
}

void CheckSoundAfterMusicInsertion() {
    Runtime runtime;
    constexpr std::array<std::uint32_t, 5> keys{2, 1, 4, 4, 4};
    (void)pal4::GameDataMonsterCollectionProcessor::Process(
        keys, 0xa000, runtime);
    Check(runtime.sound_dispatch_count == 1 && runtime.music.size() == 2 &&
              At(runtime, "insert music") < At(runtime, "collection end") &&
              At(runtime, "collection end") < At(runtime, "dispatch sound"),
          "sound dispatch rereads collection end after music insertion");
}

}  // namespace

int main() {
    try {
        CheckAllBranches();
        CheckFadeFailure();
        CheckSoundAfterMusicInsertion();
        std::cout << "game_data_monster_collection_processor_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
