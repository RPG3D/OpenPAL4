#include "pal4/even_rotate_task.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <deque>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::size_t IndexOf(const std::vector<std::string>& values,
                    const std::string& needle) {
    const auto found = std::find(values.begin(), values.end(), needle);
    if (found == values.end()) throw std::runtime_error("missing call: " + needle);
    return static_cast<std::size_t>(found - values.begin());
}

struct Runtime : virtual pal4::IEvenRotatePlaybackRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    bool throw_in_name_node = false;
    bool audio_active = false;
    bool effect_needs_update = false;
    unsigned transient_calls = 0u;
    std::uint32_t built_path = 0xa000u;
    std::uint32_t played_path = 0u;
    std::uint32_t played_parameter = 0u;
    std::uint32_t effect_name = 0u;
    std::uint32_t effect_parameter = 0u;
    bool effect_anchored = false;
    std::uint32_t visibility = 0u;
    pal4::EvenRotateVector rotation{};
    std::uint32_t rotation_owner = 0u;
    std::uint8_t is_playing = 0u;
    std::uint8_t stopped = 0x72u;
    std::uint8_t paused = 0x73u;
    std::map<std::uint32_t, std::uint32_t> string_lengths;
    std::vector<std::uint32_t> copied_destinations;
    std::vector<std::uint32_t> reset_records;
    std::vector<std::uint8_t> reset_flags;
    std::vector<std::uint32_t> reallocated_records;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object || address == 0xd000u,
              "evenRotate base construct address");
        calls.emplace_back("base construct");
    }
    std::uint32_t BaseDestruct(std::uint32_t address) override {
        Check(address == object || address == 0xd000u,
              "evenRotate base destruct address");
        calls.emplace_back("base destruct");
        return 0x2345u;
    }
    std::uint8_t TransientInitialByte() override {
        ++transient_calls;
        calls.emplace_back("transient");
        return 0x5au;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        calls.push_back("free " + std::to_string(address));
    }
    void ConstructNameNode(std::uint32_t address) override {
        Check(address == object + 0x6cu || address == 0xd06cu,
              "evenRotate name node address");
        calls.emplace_back("name node");
        if (throw_in_name_node) throw std::runtime_error("name node failed");
    }
    void ReleaseText(std::uint32_t record) override {
        calls.push_back("release " + std::to_string(record));
    }
    bool AudioIsActive(std::uint32_t handle) override {
        Check(handle == object + 0x68u, "evenRotate audio handle");
        calls.emplace_back("audio check");
        return audio_active;
    }
    void AudioPlaySound(std::uint32_t handle) override {
        Check(handle == object + 0x68u, "evenRotate audio playback handle");
        calls.emplace_back("audio play");
    }
    bool EffectNeedsUpdate(std::uint32_t handle) override {
        Check(handle == object + 0x6cu, "evenRotate effect handle");
        calls.emplace_back("effect check");
        return effect_needs_update;
    }
    void EffectUpdate(std::uint32_t handle) override {
        Check(handle == object + 0x6cu, "evenRotate effect update handle");
        calls.emplace_back("effect update");
    }
    void EffectReset(std::uint32_t handle) override {
        Check(handle == object + 0x6cu, "evenRotate effect reset handle");
        calls.emplace_back("effect reset");
    }
    std::uint32_t BuildSoundPath(std::uint32_t record,
                                  std::string_view suffix) override {
        Check(record == object + 0x40u && suffix == ".wav",
              "evenRotate sound path source");
        calls.emplace_back("build path");
        return built_path;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty string");
        return 0xd000u;
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio manager");
        return 0xb000u;
    }
    std::uint32_t PlayMusic(std::uint32_t manager,
                             std::uint32_t path,
                             std::uint32_t parameter) override {
        Check(manager == 0xb000u, "evenRotate audio manager");
        calls.emplace_back("play music");
        played_path = path;
        played_parameter = parameter;
        return 0xc000u;
    }
    void ReleaseSoundPath(std::uint32_t path) override {
        Check(path == built_path, "evenRotate released path");
        calls.emplace_back("release path");
    }
    std::uint32_t EffectManager() override {
        calls.emplace_back("effect manager");
        return 0xe000u;
    }
    pal4::EvenRotateEffectHandle CreateEffect(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t parameter,
        bool anchored) override {
        Check(manager == 0xe000u &&
                  position == words.at(object + 0x74u) + 0xacu,
              "evenRotate effect owner position");
        calls.emplace_back("create effect");
        effect_name = name;
        effect_parameter = parameter;
        effect_anchored = anchored;
        return {0xf000u, 0xf001u};
    }
    void ReleaseTemporaryEffect(
        pal4::EvenRotateEffectHandle handle) override {
        Check(handle.first == 0xf000u && handle.second == 0xf001u,
              "evenRotate temporary effect handle");
        calls.emplace_back("release temporary effect");
    }
    void SetEffectVisibility(std::uint32_t handle,
                              std::uint32_t value) override {
        Check(handle == object + 0x6cu,
              "evenRotate visibility handle");
        calls.emplace_back("visibility");
        visibility = value;
    }
    void RotateOwner(std::uint32_t owner,
                      const pal4::EvenRotateVector& delta,
                      std::uint32_t mode) override {
        Check(mode == 1u, "evenRotate rotation mode");
        calls.emplace_back("rotate");
        rotation_owner = owner;
        rotation = delta;
    }
    std::uint8_t AudioIsPlaying(std::uint32_t handle) override {
        Check(handle == object + 0x68u,
              "evenRotate audio playing handle");
        calls.emplace_back("is playing");
        return is_playing;
    }
    std::uint8_t AudioStopSound(std::uint32_t handle) override {
        Check(handle == object + 0x68u,
              "evenRotate audio stop handle");
        calls.emplace_back("stop sound");
        return stopped;
    }
    std::uint8_t AudioPauseSound(std::uint32_t handle) override {
        Check(handle == object + 0x68u,
              "evenRotate audio pause handle");
        calls.emplace_back("pause sound");
        return paused;
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        calls.push_back("length " + std::to_string(source));
        return string_lengths.at(source);
    }
    [[noreturn]] void ThrowLength() override {
        calls.emplace_back("throw length");
        throw std::length_error("evenRotate text too long");
    }
    std::uint32_t SetErrorFlag(std::uint32_t record,
                               std::uint8_t flag) override {
        calls.push_back("reset " + std::to_string(record) + " " +
                        std::to_string(flag));
        reset_records.push_back(record);
        reset_flags.push_back(flag);
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
        return 0x76543210u;
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override {
        Check(record == object + 0x40u || record == object + 0x54u,
              "evenRotate text record allocation");
        calls.push_back("reallocate " + std::to_string(record));
        reallocated_records.push_back(record);
        words[record + 4u] = record == object + 0x40u ? 0xa000u : 0xb000u;
        words[record + 12u] = length;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(length == string_lengths.at(source),
              "evenRotate copied text length");
        calls.push_back("copy " + std::to_string(source));
        copied_destinations.push_back(destination);
    }
};

struct SpawnRuntime final : Runtime, pal4::IEvenRotateSpawnRuntime {
    static constexpr std::uint32_t owner = 0x8000u;
    static constexpr std::uint32_t collection = owner + 0x2a0u;
    std::deque<std::uint32_t> allocations{0xd000u, 0xb000u};
    std::vector<std::uint32_t> allocation_sizes;
    std::uint32_t reverse_task = 0xd000u;
    std::uint32_t inserted = 0u;
    std::uint32_t added_helper = 0u;
    std::uint32_t initialized_sound = 0u;
    std::uint32_t initialized_effect = 0u;
    std::uint32_t initialized_speed = 0u;

    SpawnRuntime() {
        words[object + 0x78u] = 0x7000u;
        words[object + 0x74u] = owner;
        words[object + 0x30u] = 0u;
        words[object + 0x3cu] = 0x41a00000u;
        words[object + 0x44u] = 0x9000u;
        words[object + 0x50u] = 7u;
        words[object + 0x58u] = 0x9100u;
        words[object + 0x64u] = 8u;
        words[collection + 4u] = 0xa000u;
        words[collection + 8u] = 0xa004u;
        words[collection + 12u] = 0xa008u;
        words[0xa000u] = 0xaaaa0000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocation_sizes.push_back(bytes);
        calls.emplace_back("allocate");
        const auto result = allocations.front();
        allocations.pop_front();
        return result;
    }
    void AddReverseTask(std::uint32_t context,
                        std::uint32_t task,
                        std::uint32_t timing_address) override {
        Check(context == 0x7000u && task == reverse_task &&
                  timing_address == object + 0x30u,
              "evenRotate reverse timing");
        calls.emplace_back("add reverse");
    }
    void InitializeReverseTask(
        std::uint32_t task, std::uint32_t task_owner,
        std::uint32_t speed_bits, std::uint32_t sound_name,
        std::uint32_t sound_time, std::uint32_t effect_name,
        std::uint32_t effect_time, std::uint32_t context) override {
        Check(task == reverse_task && task_owner == owner &&
                  sound_time == 7u && effect_time == 8u &&
                  context == 0u,
              "evenRotate reverse initialization fields");
        calls.emplace_back("init reverse");
        initialized_speed = speed_bits;
        initialized_sound = sound_name;
        initialized_effect = effect_name;
    }
    void InsertOwnerTask(std::uint32_t address,
                         std::uint32_t task) override {
        Check(address == collection, "evenRotate owner collection");
        calls.emplace_back("insert reverse");
        inserted = task;
    }
    void NotifyPreviousTask(std::uint32_t task) override {
        Check(task == 0xc000u, "evenRotate previous task");
        calls.emplace_back("previous");
    }
    std::uint32_t ConstructHelperTask(std::uint32_t address) override {
        Check(address == 0xb000u, "evenRotate helper allocation");
        calls.emplace_back("construct helper");
        return address;
    }
    void AddHelperTask(std::uint32_t context,
                       std::uint32_t helper,
                       std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        Check(context == 0x7000u && first == 1u &&
                  second == 0u && third == 0u,
              "evenRotate helper timing");
        calls.emplace_back("add helper");
        added_helper = helper;
    }
    void SetHelperOwner(std::uint32_t helper,
                        std::uint32_t task_owner) override {
        Check((helper == 0xb000u || helper == 0u) &&
                  task_owner == owner,
              "evenRotate helper owner");
        calls.emplace_back("set helper owner");
    }
    void MarkProcessed(std::uint32_t context) override {
        Check(context == 0x7000u, "evenRotate processed context");
        calls.emplace_back("processed");
    }
};

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::EvenRotateTask::Construct(Runtime::object, runtime) ==
              Runtime::object &&
              runtime.calls.front() == "base construct" &&
              runtime.transient_calls == 1u &&
              runtime.bytes.at(Runtime::object + 0x40u) == 0x5au &&
              runtime.bytes.at(Runtime::object + 0x54u) == 0x5au &&
              runtime.words.at(Runtime::object) == 0x8487e8u &&
              runtime.words.at(Runtime::object + 0x3cu) == 0x41a00000u &&
              runtime.words.at(Runtime::object + 0x44u) == 0u &&
              runtime.words.at(Runtime::object + 0x48u) == 0u &&
              runtime.words.at(Runtime::object + 0x4cu) == 0u &&
              runtime.words.at(Runtime::object + 0x58u) == 0u &&
              runtime.words.at(Runtime::object + 0x5cu) == 0u &&
              runtime.words.at(Runtime::object + 0x60u) == 0u &&
              runtime.words.at(Runtime::object + 0x68u) == 0u &&
              runtime.words.at(Runtime::object + 0x74u) == 0u &&
              runtime.words.at(Runtime::object + 0x78u) == 0u &&
              IndexOf(runtime.calls, "word 4096") <
                  IndexOf(runtime.calls, "name node") &&
              IndexOf(runtime.calls, "name node") <
                  IndexOf(runtime.calls, "word 4212"),
          "evenRotate constructor initializes layout and name node in order");

    Runtime failed;
    failed.throw_in_name_node = true;
    bool thrown = false;
    try {
        (void)pal4::EvenRotateTask::Construct(Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls.size() >= 4u &&
              failed.calls[failed.calls.size() - 3u] == "release 4180" &&
              failed.calls[failed.calls.size() - 2u] == "release 4160" &&
              failed.calls.back() == "base destruct",
          "evenRotate constructor unwinds text and base after name node failure");
}

void CheckDestruct() {
    for (const auto effect_count : {std::uint8_t{0u},
                                    std::uint8_t{2u},
                                    std::uint8_t{0xffu}}) {
        for (const auto sound_count : {std::uint8_t{0u},
                                       std::uint8_t{2u},
                                       std::uint8_t{0xffu}}) {
            Runtime runtime;
            runtime.words[Runtime::object + 0x58u] = 0x3000u;
            runtime.words[Runtime::object + 0x44u] = 0x4000u;
            runtime.bytes[0x2fffu] = effect_count;
            runtime.bytes[0x3fffu] = sound_count;
            runtime.audio_active = true;
            runtime.effect_needs_update = true;
            Check(pal4::EvenRotateTask::Destruct(
                      Runtime::object, runtime) == 0x2345u &&
                      runtime.words.at(Runtime::object) == 0x8486fcu &&
                      runtime.words.at(Runtime::object + 0x3cu) ==
                          0x41a00000u &&
                      runtime.words.at(Runtime::object + 0x74u) == 0u &&
                      runtime.words.at(Runtime::object + 0x78u) == 0u &&
                      runtime.words.at(Runtime::object + 0x58u) == 0u &&
                      runtime.words.at(Runtime::object + 0x5cu) == 0u &&
                      runtime.words.at(Runtime::object + 0x60u) == 0u &&
                      runtime.words.at(Runtime::object + 0x44u) == 0u &&
                      runtime.words.at(Runtime::object + 0x48u) == 0u &&
                      runtime.words.at(Runtime::object + 0x4cu) == 0u &&
                      (effect_count == 2u ?
                           runtime.bytes.at(0x2fffu) == 1u :
                           std::find(runtime.freed.begin(),
                                     runtime.freed.end(), 0x2fffu) !=
                               runtime.freed.end()) &&
                      (sound_count == 2u ?
                           runtime.bytes.at(0x3fffu) == 1u :
                           std::find(runtime.freed.begin(),
                                     runtime.freed.end(), 0x3fffu) !=
                               runtime.freed.end()) &&
                      IndexOf(runtime.calls, "audio check") <
                          IndexOf(runtime.calls, "audio play") &&
                      IndexOf(runtime.calls, "audio play") <
                          IndexOf(runtime.calls, "effect check") &&
                      IndexOf(runtime.calls, "effect check") <
                          IndexOf(runtime.calls, "effect update") &&
                      IndexOf(runtime.calls, "effect update") <
                          IndexOf(runtime.calls, "effect reset") &&
                      runtime.calls.back() == "base destruct",
                  "evenRotate destructor releases handles and text in order");
        }
    }

    Runtime empty;
    empty.words[Runtime::object + 0x58u] = 0u;
    empty.words[Runtime::object + 0x44u] = 0u;
    Check(pal4::EvenRotateTask::Destruct(Runtime::object, empty) ==
              0x2345u && empty.freed.empty() &&
              std::find(empty.calls.begin(), empty.calls.end(),
                        "audio play") == empty.calls.end() &&
              std::find(empty.calls.begin(), empty.calls.end(),
                        "effect update") == empty.calls.end() &&
              IndexOf(empty.calls, "effect reset") <
                  IndexOf(empty.calls, "word 4184"),
          "evenRotate destructor handles empty text and inactive handles");
}

void CheckPlayback() {
    Runtime missing_owner;
    missing_owner.words[Runtime::object + 0x74u] = 0u;
    Check(pal4::EvenRotateTask::Enter(Runtime::object,
                                       missing_owner) == 0u &&
              missing_owner.calls.empty() &&
              pal4::EvenRotateTask::Update(Runtime::object, 0.25f,
                                            missing_owner) == 0u,
          "evenRotate playback requires owner");

    Runtime empty;
    empty.words[Runtime::object + 0x74u] = 0x5000u;
    empty.words[Runtime::object + 0x48u] = 0u;
    empty.words[Runtime::object + 0x5cu] = 0u;
    Check(pal4::EvenRotateTask::Enter(Runtime::object, empty) == 1u &&
              empty.calls.empty(),
          "evenRotate entry skips absent sound and effect");

    Runtime sound;
    sound.words[Runtime::object + 0x74u] = 0x5000u;
    sound.words[Runtime::object + 0x48u] = 3u;
    sound.words[Runtime::object + 0x50u] =
        static_cast<std::uint32_t>(-4);
    sound.words[Runtime::object + 0x5cu] = 0u;
    sound.audio_active = true;
    Check(pal4::EvenRotateTask::Enter(Runtime::object, sound) == 1u &&
              sound.played_path == 0xa000u &&
              sound.played_parameter == 0u &&
              sound.words.at(Runtime::object + 0x50u) == 0u &&
              sound.words.at(Runtime::object + 0x68u) == 0xc000u &&
              IndexOf(sound.calls, "build path") <
                  IndexOf(sound.calls, "audio check") &&
              IndexOf(sound.calls, "audio check") <
                  IndexOf(sound.calls, "audio play") &&
              IndexOf(sound.calls, "audio play") <
                  IndexOf(sound.calls, "play music") &&
              IndexOf(sound.calls, "play music") <
                  IndexOf(sound.calls, "release path"),
          "evenRotate sound branch clamps parameter and replaces handle");

    Runtime fallback_sound;
    fallback_sound.words[Runtime::object + 0x74u] = 0x5000u;
    fallback_sound.words[Runtime::object + 0x48u] = 1u;
    fallback_sound.words[Runtime::object + 0x50u] = 7u;
    fallback_sound.words[Runtime::object + 0x5cu] = 0u;
    fallback_sound.built_path = 0u;
    Check(pal4::EvenRotateTask::Enter(Runtime::object,
                                       fallback_sound) == 1u &&
              fallback_sound.played_path == 0xd000u &&
              fallback_sound.played_parameter == 7u &&
              std::find(fallback_sound.calls.begin(),
                        fallback_sound.calls.end(), "audio play") ==
                  fallback_sound.calls.end(),
          "evenRotate sound uses empty fallback and leaves positive parameter");

    Runtime effect;
    effect.words[Runtime::object + 0x74u] = 0x5000u;
    effect.words[Runtime::object + 0x48u] = 0u;
    effect.words[Runtime::object + 0x5cu] = 4u;
    effect.words[Runtime::object + 0x58u] = 0x9000u;
    effect.words[Runtime::object + 0x64u] = 55u;
    effect.words[0x5000u + 0x1a8u] = 1u;
    effect.words[0x5000u + 0x14u] = 0x6000u;
    effect.words[0x6004u] = 9u;
    effect.effect_needs_update = true;
    Check(pal4::EvenRotateTask::Enter(Runtime::object, effect) == 1u &&
              effect.effect_name == 0x9000u &&
              effect.effect_parameter == 55u && effect.effect_anchored &&
              effect.words.at(Runtime::object + 0x6cu) == 0xf000u &&
              effect.words.at(Runtime::object + 0x70u) == 0xf001u &&
              effect.visibility == 9u &&
              IndexOf(effect.calls, "effect update") <
                  IndexOf(effect.calls, "create effect") &&
              IndexOf(effect.calls, "create effect") <
                  IndexOf(effect.calls, "release temporary effect") &&
              IndexOf(effect.calls, "release temporary effect") <
                  IndexOf(effect.calls, "visibility"),
          "evenRotate effect branch updates, creates and shows effect");

    Runtime fallback_effect;
    fallback_effect.words = effect.words;
    fallback_effect.words[Runtime::object + 0x58u] = 0u;
    fallback_effect.words[0x5000u + 0x1a8u] = 0u;
    Check(pal4::EvenRotateTask::Enter(Runtime::object,
                                       fallback_effect) == 1u &&
              fallback_effect.effect_name == 0xd000u &&
              !fallback_effect.effect_anchored &&
              std::find(fallback_effect.calls.begin(),
                        fallback_effect.calls.end(), "effect update") ==
                  fallback_effect.calls.end(),
          "evenRotate effect uses empty fallback and unanchored flag");

    Runtime update;
    update.words[Runtime::object + 0x74u] = 0x5000u;
    update.words[Runtime::object + 0x3cu] = 0x41a00000u;
    Check(pal4::EvenRotateTask::Update(Runtime::object, 0.25f,
                                        update) == 1u &&
              update.rotation_owner == 0x5000u &&
              update.rotation.x == 0.0f &&
              update.rotation.y == 5.0f &&
              update.rotation.z == 0.0f,
          "evenRotate update applies positive Y delta");
    update.words[Runtime::object + 0x3cu] = std::bit_cast<std::uint32_t>(-8.0f);
    Check(pal4::EvenRotateTask::Update(Runtime::object, 0.5f,
                                        update) == 1u &&
              update.rotation.y == -4.0f,
          "evenRotate update preserves negative speed");

    Runtime sound_hooks;
    Check(pal4::EvenRotateTask::StopSoundIfPlaying(
              Runtime::object, sound_hooks) == 0u &&
              sound_hooks.calls ==
                  (std::vector<std::string>{"is playing"}),
          "evenRotate stop hook skips inactive sound");
    sound_hooks.calls.clear();
    sound_hooks.is_playing = 1u;
    Check(pal4::EvenRotateTask::StopSoundIfPlaying(
              Runtime::object, sound_hooks) == 0x72u &&
              sound_hooks.calls ==
                  (std::vector<std::string>{"is playing", "stop sound"}),
          "evenRotate stop hook forwards active sound result");
    sound_hooks.calls.clear();
    Check(pal4::EvenRotateTask::PauseSoundIfNotPlaying(
              Runtime::object, sound_hooks) == 1u &&
              sound_hooks.calls ==
                  (std::vector<std::string>{"is playing"}),
          "evenRotate pause hook keeps playing result");
    sound_hooks.calls.clear();
    sound_hooks.is_playing = 0u;
    Check(pal4::EvenRotateTask::PauseSoundIfNotPlaying(
              Runtime::object, sound_hooks) == 0x73u &&
              sound_hooks.calls ==
                  (std::vector<std::string>{"is playing", "pause sound"}),
          "evenRotate pause hook forwards inactive sound result");
}

void CheckFollowup() {
    Runtime descriptor;
    Check(pal4::EvenRotateTaskFactory::ConstructDescriptor(
              0x2000u, descriptor) == 0x2000u &&
              descriptor.words.at(0x2000u) == 0x84880cu,
          "evenRotate factory descriptor");
    SpawnRuntime factory;
    Check(pal4::EvenRotateTaskFactory::Create(factory) ==
              0xd000u &&
              factory.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x7cu}) &&
              factory.words.at(0xd000u) == 0x8487e8u &&
              factory.calls.front() == "allocate" &&
              IndexOf(factory.calls, "allocate") <
                  IndexOf(factory.calls, "base construct"),
          "evenRotate factory allocates and constructs task");
    SpawnRuntime no_memory;
    no_memory.allocations = {0u};
    Check(pal4::EvenRotateTaskFactory::Create(no_memory) == 0u &&
              no_memory.calls ==
                  (std::vector<std::string>{"allocate"}),
          "evenRotate factory returns null allocation");
    SpawnRuntime failed_factory;
    failed_factory.throw_in_name_node = true;
    bool factory_threw = false;
    try {
        (void)pal4::EvenRotateTaskFactory::Create(failed_factory);
    } catch (const std::runtime_error&) {
        factory_threw = true;
    }
    Check(factory_threw &&
              failed_factory.freed ==
                  (std::vector<std::uint32_t>{0xd000u}) &&
              failed_factory.calls.back() == "free 53248" &&
              IndexOf(failed_factory.calls, "base destruct") <
                  IndexOf(failed_factory.calls, "free 53248"),
          "evenRotate factory frees task after constructor failure");
    SpawnRuntime no_context;
    no_context.words[Runtime::object + 0x78u] = 0u;
    Check(pal4::EvenRotateTask::SpawnReverseAndFollowup(
              Runtime::object, no_context) == 0u &&
              no_context.calls.empty(),
          "evenRotate follow-up requires context");
    SpawnRuntime no_owner;
    no_owner.words[Runtime::object + 0x74u] = 0u;
    Check(pal4::EvenRotateTask::SpawnReverseAndFollowup(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(),
          "evenRotate follow-up requires owner");
    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0xc000u;
    Check(pal4::EvenRotateTask::SpawnReverseAndFollowup(
              Runtime::object, previous) == 1u &&
              previous.inserted == 0xd000u &&
              previous.initialized_speed ==
                  std::bit_cast<std::uint32_t>(-20.0f) &&
              previous.initialized_sound == 0x9000u &&
              previous.initialized_effect == 0x9100u &&
              IndexOf(previous.calls, "add reverse") <
                  IndexOf(previous.calls, "init reverse") &&
              IndexOf(previous.calls, "insert reverse") <
                  IndexOf(previous.calls, "previous") &&
              previous.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x7cu}),
          "evenRotate reverse task precedes previous callback");
    SpawnRuntime fallback_names;
    fallback_names.words[Runtime::object + 0x44u] = 0u;
    fallback_names.words[Runtime::object + 0x58u] = 0u;
    Check(pal4::EvenRotateTask::SpawnReverseAndFollowup(
              Runtime::object, fallback_names) == 1u &&
              fallback_names.initialized_sound == 0xd000u &&
              fallback_names.initialized_effect == 0xd000u &&
              fallback_names.added_helper == 0xb000u &&
              fallback_names.words.at(0xa004u) == 0xb000u &&
              fallback_names.words.at(SpawnRuntime::collection + 8u) ==
                  0xa008u &&
              fallback_names.calls.back() == "processed",
          "evenRotate follow-up uses empty names and appends helper");
    SpawnRuntime null_reverse;
    null_reverse.reverse_task = 0u;
    null_reverse.allocations = {0u, 0xb000u};
    Check(pal4::EvenRotateTask::SpawnReverseAndFollowup(
              Runtime::object, null_reverse) == 1u &&
              null_reverse.inserted == 0u &&
              std::find(null_reverse.calls.begin(),
                        null_reverse.calls.end(), "add reverse") ==
                  null_reverse.calls.end(),
          "evenRotate follow-up skips null reverse registration");
    SpawnRuntime growth;
    growth.words[SpawnRuntime::collection + 8u] = 0xa008u;
    growth.words[SpawnRuntime::collection + 12u] = 0xa008u;
    growth.words[0xa004u] = 0xbbbb0000u;
    growth.allocations.push_back(0xe000u);
    Check(pal4::EvenRotateTask::SpawnReverseAndFollowup(
              Runtime::object, growth) == 1u &&
              growth.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x7cu, 0x40u, 16u}) &&
              growth.words.at(0xe008u) == 0xb000u &&
              growth.words.at(SpawnRuntime::collection + 8u) == 0xe00cu &&
              growth.freed ==
                  (std::vector<std::uint32_t>{0xa000u}),
          "evenRotate follow-up grows helper collection");
}

void CheckInitialize() {
    Runtime empty;
    empty.string_lengths[0x9000u] = 0u;
    empty.string_lengths[0x9100u] = 0u;
    empty.words[Runtime::object + 0x44u] = 0u;
    empty.words[Runtime::object + 0x58u] = 0u;
    Check(pal4::EvenRotateTask::Initialize(
              Runtime::object, 0x5000u, 0x41a00000u,
              0x9000u, 7u, 0x9100u, 8u, 0x7000u,
              empty) == 0x76543210u &&
              empty.reset_records ==
                  (std::vector<std::uint32_t>{0x1040u, 0x1054u}) &&
              empty.reset_flags ==
                  (std::vector<std::uint8_t>{1u, 1u}) &&
              empty.words.at(Runtime::object + 0x74u) == 0x5000u &&
              empty.words.at(Runtime::object + 0x3cu) == 0x41a00000u &&
              empty.words.at(Runtime::object + 0x50u) == 7u &&
              empty.words.at(Runtime::object + 0x64u) == 8u &&
              empty.words.at(Runtime::object + 0x78u) == 0x7000u,
          "evenRotate init clears empty strings and forwards reset result");

    Runtime shared_empty;
    shared_empty.string_lengths[0x9000u] = 0u;
    shared_empty.string_lengths[0x9100u] = 0u;
    shared_empty.words[Runtime::object + 0x44u] = 0x3000u;
    shared_empty.words[Runtime::object + 0x58u] = 0x4000u;
    shared_empty.bytes[0x2fffu] = 2u;
    shared_empty.bytes[0x3fffu] = 3u;
    Check(pal4::EvenRotateTask::Initialize(
              Runtime::object, 0x5000u, 0x41a00000u,
              0x9000u, 7u, 0x9100u, 8u, 0x7000u,
              shared_empty) == 0x7000u &&
              shared_empty.bytes.at(0x2fffu) == 1u &&
              shared_empty.bytes.at(0x3fffu) == 2u &&
              shared_empty.reset_flags ==
                  (std::vector<std::uint8_t>{0u, 0u}) &&
              shared_empty.copied_destinations.empty(),
          "evenRotate init decrements shared empty strings");

    Runtime reused;
    reused.string_lengths[0x9000u] = 3u;
    reused.string_lengths[0x9100u] = 5u;
    reused.words[Runtime::object + 0x44u] = 0x3000u;
    reused.words[Runtime::object + 0x4cu] = 10u;
    reused.words[Runtime::object + 0x58u] = 0x4000u;
    reused.words[Runtime::object + 0x60u] = 10u;
    reused.bytes[0x2fffu] = 0u;
    reused.bytes[0x3fffu] = 0xffu;
    Check(pal4::EvenRotateTask::Initialize(
              Runtime::object, 0x5000u, 0x41a00000u,
              0x9000u, 7u, 0x9100u, 8u, 0x7000u,
              reused) == Runtime::object &&
              reused.reallocated_records.empty() &&
              reused.copied_destinations ==
                  (std::vector<std::uint32_t>{0x3000u, 0x4000u}) &&
              reused.words.at(Runtime::object + 0x48u) == 3u &&
              reused.words.at(Runtime::object + 0x5cu) == 5u &&
              reused.bytes.at(0x3003u) == 0u &&
              reused.bytes.at(0x4005u) == 0u &&
              IndexOf(reused.calls, "copy 36864") <
                  IndexOf(reused.calls, "length 37120"),
          "evenRotate init reuses both text buffers in order");

    Runtime shared;
    shared.string_lengths[0x9000u] = 3u;
    shared.string_lengths[0x9100u] = 5u;
    shared.words[Runtime::object + 0x44u] = 0x3000u;
    shared.words[Runtime::object + 0x58u] = 0x4000u;
    shared.bytes[0x2fffu] = 2u;
    shared.bytes[0x3fffu] = 2u;
    Check(pal4::EvenRotateTask::Initialize(
              Runtime::object, 0x5000u, 0x41a00000u,
              0x9000u, 7u, 0x9100u, 8u, 0x7000u,
              shared) == Runtime::object &&
              shared.reallocated_records ==
                  (std::vector<std::uint32_t>{0x1040u, 0x1054u}) &&
              shared.reset_records.empty() &&
              shared.copied_destinations ==
                  (std::vector<std::uint32_t>{0xa000u, 0xb000u}),
          "evenRotate init reallocates shared nonempty text");

    for (const auto capacity : {2u, 32u}) {
        Runtime growth;
        growth.string_lengths[0x9000u] = 3u;
        growth.string_lengths[0x9100u] = 5u;
        growth.words[Runtime::object + 0x44u] = 0x3000u;
        growth.words[Runtime::object + 0x4cu] = capacity;
        growth.words[Runtime::object + 0x58u] = 0x4000u;
        growth.words[Runtime::object + 0x60u] = capacity;
        growth.bytes[0x2fffu] = 0u;
        growth.bytes[0x3fffu] = 0u;
        Check(pal4::EvenRotateTask::Initialize(
                  Runtime::object, 0x5000u, 0x41a00000u,
                  0x9000u, 7u, 0x9100u, 8u, 0x7000u,
                  growth) == Runtime::object &&
                  growth.reset_flags ==
                      (std::vector<std::uint8_t>{1u, 1u}) &&
                  growth.reallocated_records.size() == 2u,
              "evenRotate init grows insufficient or large capacity");
    }

    Runtime too_long;
    too_long.string_lengths[0x9000u] = 0xfffffffeu;
    bool thrown = false;
    try {
        (void)pal4::EvenRotateTask::Initialize(
            Runtime::object, 0x5000u, 0x41a00000u,
            0x9000u, 7u, 0x9100u, 8u, 0x7000u, too_long);
    } catch (const std::length_error&) {
        thrown = true;
    }
    Check(thrown && too_long.calls.back() == "throw length" &&
              too_long.words.at(Runtime::object + 0x74u) == 0x5000u &&
              too_long.words.at(Runtime::object + 0x3cu) == 0x41a00000u,
          "evenRotate init rejects oversized sound before text writes");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckDestruct();
        CheckPlayback();
        CheckFollowup();
        CheckInitialize();
        std::cout << "even_rotate_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
