#include "pal4/move_task.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <deque>
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

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& name) {
    const auto found = std::find(calls.begin(), calls.end(), name);
    if (found == calls.end()) throw std::runtime_error("missing " + name);
    return static_cast<std::size_t>(found - calls.begin());
}

struct Runtime : virtual pal4::IMoveTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    bool fail_name_node = false;
    bool fail_play = false;
    bool audio_active = false;
    bool effect_needs_update = false;
    unsigned transient_calls = 0u;
    std::uint32_t built_path = 0xa000u;
    std::uint32_t played_path = 0u;
    std::uint32_t played_parameter = 0u;
    std::uint32_t effect_name = 0u;
    std::uint32_t effect_position = 0u;
    std::uint32_t effect_parameter = 0u;
    bool anchored = false;
    std::uint32_t visibility = 0u;
    std::uint32_t position_owner = 0u;
    pal4::MoveTaskVector position{};
    std::vector<float> fractions;
    std::map<std::uint32_t, std::uint32_t> string_lengths;
    std::vector<std::uint32_t> reallocated_records;
    std::vector<std::uint32_t> copied_destinations;
    std::vector<std::uint8_t> reset_flags;
    std::uint8_t is_playing = 0u;
    std::uint8_t stopped = 0x72u;
    std::uint8_t paused = 0x73u;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object || address == 0xd000u,
              "move base construct address");
        calls.emplace_back("base construct");
    }
    void BaseDestruct(std::uint32_t address) override {
        Check(address == object || address == 0xd000u,
              "move base destruct address");
        calls.emplace_back("base destruct");
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
        calls.emplace_back("byte " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word " + std::to_string(address));
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free " + std::to_string(address));
    }
    void ConstructNameNode(std::uint32_t address) override {
        Check(address == object + 0x8cu || address == 0xd08cu,
              "move name node address");
        calls.emplace_back("name node");
        if (fail_name_node) throw std::runtime_error("name node failed");
    }
    void ReleaseText(std::uint32_t record) override {
        calls.emplace_back("release " + std::to_string(record));
    }
    bool AudioIsActive(std::uint32_t handle) override {
        Check(handle == object + 0x88u, "move sound handle");
        calls.emplace_back("audio check");
        return audio_active;
    }
    void AudioPlaySound(std::uint32_t handle) override {
        Check(handle == object + 0x88u, "move played handle");
        calls.emplace_back("audio play");
    }
    bool EffectNeedsUpdate(std::uint32_t handle) override {
        Check(handle == object + 0x8cu, "move effect handle");
        calls.emplace_back("effect check");
        return effect_needs_update;
    }
    void EffectUpdate(std::uint32_t handle) override {
        Check(handle == object + 0x8cu, "move effect update handle");
        calls.emplace_back("effect update");
    }
    void EffectReset(std::uint32_t handle) override {
        Check(handle == object + 0x8cu, "move effect reset handle");
        calls.emplace_back("effect reset");
    }
    std::uint32_t BuildSoundPath(std::uint32_t record,
                                 std::string_view suffix) override {
        Check(record == object + 0x60u && suffix == ".wav",
              "move sound path parts");
        calls.emplace_back("build path");
        return built_path;
    }
    void ReleaseSoundPath(std::uint32_t path) override {
        Check(path == built_path, "move released sound path");
        calls.emplace_back("release path");
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty string");
        return 0xd000u;
    }
    std::uint32_t PlayMusic(std::uint32_t path,
                            std::uint32_t parameter) override {
        calls.emplace_back("play music");
        if (fail_play) throw std::runtime_error("audio failed");
        played_path = path;
        played_parameter = parameter;
        return 0xc000u;
    }
    pal4::MoveTaskEffectHandle CreateEffect(
        std::uint32_t name, std::uint32_t position_address,
        std::uint32_t parameter, bool anchor) override {
        calls.emplace_back("create effect");
        effect_name = name;
        effect_position = position_address;
        effect_parameter = parameter;
        anchored = anchor;
        return {0xf000u, 0xf001u};
    }
    void ReleaseTemporaryEffect(
        pal4::MoveTaskEffectHandle handle) override {
        Check(handle.first == 0xf000u && handle.second == 0xf001u,
              "move temporary effect");
        calls.emplace_back("release temporary effect");
    }
    void SetEffectVisibility(std::uint32_t handle,
                             std::uint32_t value) override {
        Check(handle == object + 0x8cu,
              "move effect visibility handle");
        calls.emplace_back("visibility");
        visibility = value;
    }
    float InterpolateFloat(float start, float end,
                           float fraction,
                           std::uint32_t mode) override {
        Check(mode == 0u, "move interpolation mode");
        calls.emplace_back("interpolate");
        fractions.push_back(fraction);
        return start + (end - start) * fraction;
    }
    void SetOwnerPosition(std::uint32_t owner,
                          const pal4::MoveTaskVector& value,
                          std::uint32_t mode) override {
        Check(mode == 0u, "move owner position mode");
        calls.emplace_back("set position");
        position_owner = owner;
        position = value;
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        calls.emplace_back("string length");
        return string_lengths.at(source);
    }
    [[noreturn]] void ThrowLength() override {
        calls.emplace_back("throw length");
        throw std::length_error("move name too long");
    }
    std::uint32_t SetErrorFlag(std::uint32_t record,
                               std::uint8_t flag) override {
        calls.emplace_back("reset text");
        reset_flags.push_back(flag);
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
        return 0x76543210u;
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override {
        calls.emplace_back("reallocate text");
        reallocated_records.push_back(record);
        words[record + 4u] = (record & 0xffu) == 0x60u ?
            0xc000u : 0xc100u;
        words[record + 12u] = length;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(length == string_lengths.at(source),
              "move copied text length");
        calls.emplace_back("copy text");
        copied_destinations.push_back(destination);
    }
    std::uint8_t AudioIsPlaying(std::uint32_t handle) override {
        Check(handle == object + 0x88u,
              "move sound playing handle");
        calls.emplace_back("is playing");
        return is_playing;
    }
    std::uint8_t AudioStopSound(std::uint32_t handle) override {
        Check(handle == object + 0x88u,
              "move stop sound handle");
        calls.emplace_back("stop sound");
        return stopped;
    }
    std::uint8_t AudioPauseSound(std::uint32_t handle) override {
        Check(handle == object + 0x88u,
              "move pause sound handle");
        calls.emplace_back("pause sound");
        return paused;
    }
};

struct SpawnRuntime final : Runtime, pal4::IMoveTaskSpawnRuntime {
    static constexpr std::uint32_t owner = 0x8000u;
    static constexpr std::uint32_t collection = owner + 0x2a0u;
    std::deque<std::uint32_t> allocations{0xd000u, 0xb000u};
    std::vector<std::uint32_t> allocation_sizes;
    std::uint32_t inserted = 0u;
    std::uint32_t registered = 0u;
    std::uint32_t added_helper = 0u;
    std::uint32_t processed = 0u;

    SpawnRuntime() {
        words[object + 0x98u] = 0x7000u;
        words[object + 0x94u] = owner;
        words[object + 0x30u] = 0u;
        words[object + 0x44u] = std::bit_cast<std::uint32_t>(10.0f);
        for (const auto offset : {0u, 4u, 8u})
            words[object + 0x48u + offset] =
                std::bit_cast<std::uint32_t>(1.0f + offset / 4u);
        words[object + 0x64u] = 0x9000u;
        words[object + 0x70u] = 7u;
        words[object + 0x78u] = 0x9100u;
        words[object + 0x84u] = 8u;
        string_lengths[0x9000u] = 3u;
        string_lengths[0x9100u] = 5u;
        string_lengths[0xd000u] = 0u;
        words[collection + 4u] = 0xa000u;
        words[collection + 8u] = 0xa004u;
        words[collection + 12u] = 0xa008u;
        words[0xa000u] = 0xaaaa0000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocation_sizes.push_back(bytes);
        calls.emplace_back("allocate");
        const auto address = allocations.front();
        allocations.pop_front();
        return address;
    }
    void AddReverseTask(std::uint32_t context,
                        std::uint32_t task,
                        std::uint32_t timing_address) override {
        Check(context == 0x7000u && task == 0xd000u &&
                  timing_address == object + 0x0cu,
              "move reverse task timing");
        calls.emplace_back("add reverse");
        registered = task;
    }
    void InsertOwnerTask(std::uint32_t address,
                         std::uint32_t task) override {
        Check(address == collection, "move owner collection");
        calls.emplace_back("insert reverse");
        inserted = task;
    }
    void NotifyPreviousTask(std::uint32_t previous) override {
        Check(previous == 0x6000u, "move previous task");
        calls.emplace_back("previous");
    }
    std::uint32_t ConstructHelperTask(
        std::uint32_t memory) override {
        Check(memory == 0xb000u, "move helper memory");
        calls.emplace_back("construct helper");
        return memory;
    }
    void AddHelperTask(std::uint32_t context,
                       std::uint32_t helper,
                       std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        Check(context == 0x7000u && helper == 0xb000u &&
                  first == 1u && second == 0u && third == 0u,
              "move helper timing");
        calls.emplace_back("add helper");
        added_helper = helper;
    }
    void SetHelperOwner(std::uint32_t helper,
                        std::uint32_t task_owner) override {
        Check((helper == 0xb000u || helper == 0u) &&
                  task_owner == owner, "move helper owner");
        calls.emplace_back("set helper owner");
    }
    void MarkProcessed(std::uint32_t context) override {
        Check(context == 0x7000u, "move processed context");
        calls.emplace_back("processed");
        processed = context;
    }
};

void SeedMovement(Runtime& runtime) {
    runtime.words[Runtime::object + 0x94u] = 0x5000u;
    runtime.words[Runtime::object + 0x40u] =
        std::bit_cast<std::uint32_t>(0.0f);
    runtime.words[Runtime::object + 0x44u] =
        std::bit_cast<std::uint32_t>(10.0f);
    runtime.bytes[Runtime::object + 0x3cu] = 0u;
    for (const auto offset : {0u, 4u, 8u}) {
        const auto index = offset / 4u;
        runtime.words[Runtime::object + 0x48u + offset] =
            std::bit_cast<std::uint32_t>(1.0f + index);
        runtime.words[Runtime::object + 0x54u + offset] =
            std::bit_cast<std::uint32_t>(11.0f + index);
        runtime.words[0x5000u + 0xacu + offset] =
            std::bit_cast<std::uint32_t>(1.0f + index);
    }
}

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::MoveTask::Construct(Runtime::object, runtime) ==
              Runtime::object &&
              runtime.transient_calls == 1u &&
              runtime.bytes.at(Runtime::object + 0x60u) == 0x5au &&
              runtime.bytes.at(Runtime::object + 0x74u) == 0x5au &&
              runtime.words.at(Runtime::object) == 0x848858u &&
              runtime.words.at(Runtime::object + 0x64u) == 0u &&
              runtime.words.at(Runtime::object + 0x68u) == 0u &&
              runtime.words.at(Runtime::object + 0x6cu) == 0u &&
              runtime.words.at(Runtime::object + 0x78u) == 0u &&
              runtime.words.at(Runtime::object + 0x7cu) == 0u &&
              runtime.words.at(Runtime::object + 0x80u) == 0u &&
              runtime.words.at(Runtime::object + 0x88u) == 0u &&
              runtime.words.at(Runtime::object + 0x40u) == 0u &&
              runtime.words.at(Runtime::object + 0x44u) == 0u &&
              runtime.words.at(Runtime::object + 0x94u) == 0u &&
              runtime.words.at(Runtime::object + 0x98u) == 0u &&
              IndexOf(runtime.calls, "base construct") <
                  IndexOf(runtime.calls, "name node") &&
              IndexOf(runtime.calls, "name node") <
                  IndexOf(runtime.calls, "word 4244"),
          "move constructor layout and ordering");
    Runtime failed;
    failed.fail_name_node = true;
    bool thrown = false;
    try {
        (void)pal4::MoveTask::Construct(Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls[failed.calls.size() - 3u] ==
                        "release 4212" &&
              failed.calls[failed.calls.size() - 2u] ==
                  "release 4192" &&
              failed.calls.back() == "base destruct",
          "move constructor exception cleanup");
}

void CheckDestruct() {
    for (const auto effect_count : {std::uint8_t{0u},
                                    std::uint8_t{2u},
                                    std::uint8_t{0xffu}}) {
        for (const auto sound_count : {std::uint8_t{0u},
                                       std::uint8_t{2u},
                                       std::uint8_t{0xffu}}) {
            Runtime runtime;
            runtime.words[Runtime::object + 0x78u] = 0x3000u;
            runtime.words[Runtime::object + 0x64u] = 0x4000u;
            runtime.bytes[0x2fffu] = effect_count;
            runtime.bytes[0x3fffu] = sound_count;
            runtime.audio_active = true;
            runtime.effect_needs_update = true;
            pal4::MoveTask::Destruct(Runtime::object, runtime);
            Check(runtime.words.at(Runtime::object) == 0x8486fcu &&
                      runtime.words.at(Runtime::object + 0x78u) == 0u &&
                      runtime.words.at(Runtime::object + 0x7cu) == 0u &&
                      runtime.words.at(Runtime::object + 0x80u) == 0u &&
                      runtime.words.at(Runtime::object + 0x64u) == 0u &&
                      runtime.words.at(Runtime::object + 0x68u) == 0u &&
                      runtime.words.at(Runtime::object + 0x6cu) == 0u &&
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
                      IndexOf(runtime.calls, "audio play") <
                          IndexOf(runtime.calls, "effect update") &&
                      IndexOf(runtime.calls, "effect update") <
                          IndexOf(runtime.calls, "effect reset") &&
                      runtime.calls.back() == "base destruct",
                  "move destructor handles references and effects");
        }
    }
    Runtime empty;
    empty.words[Runtime::object + 0x78u] = 0u;
    empty.words[Runtime::object + 0x64u] = 0u;
    pal4::MoveTask::Destruct(Runtime::object, empty);
    Check(empty.freed.empty() && empty.calls.back() ==
              "base destruct",
          "move destructor accepts empty string records");
}

void CheckEnter() {
    Runtime missing;
    missing.words[Runtime::object + 0x94u] = 0u;
    Check(pal4::MoveTask::Enter(Runtime::object, missing) ==
              0u && missing.calls.empty(),
          "move entry owner gate");
    Runtime empty;
    SeedMovement(empty);
    empty.words[Runtime::object + 0x68u] = 0u;
    empty.words[Runtime::object + 0x7cu] = 0u;
    Check(pal4::MoveTask::Enter(Runtime::object, empty) ==
              1u && empty.words.at(Runtime::object + 0x40u) ==
                        0u &&
              empty.words.at(Runtime::object + 0x48u) ==
                  empty.words.at(0x50acu) &&
              empty.words.at(Runtime::object + 0x4cu) ==
                  empty.words.at(0x50b0u) &&
              empty.words.at(Runtime::object + 0x50u) ==
                  empty.words.at(0x50b4u),
          "move entry snapshots owner position");
    Runtime active;
    SeedMovement(active);
    active.words[Runtime::object + 0x68u] = 3u;
    active.words[Runtime::object + 0x70u] =
        static_cast<std::uint32_t>(-2);
    active.words[Runtime::object + 0x7cu] = 4u;
    active.words[Runtime::object + 0x78u] = 0x9000u;
    active.words[Runtime::object + 0x84u] = 55u;
    active.words[0x5000u + 0x1a8u] = 1u;
    active.words[0x5000u + 0x14u] = 0x6000u;
    active.words[0x6004u] = 9u;
    active.audio_active = true;
    active.effect_needs_update = true;
    Check(pal4::MoveTask::Enter(Runtime::object, active) ==
              1u && active.played_path == 0xa000u &&
              active.played_parameter == 0u &&
              active.words.at(Runtime::object + 0x70u) == 0u &&
              active.words.at(Runtime::object + 0x88u) == 0xc000u &&
              active.effect_name == 0x9000u &&
              active.effect_position == 0x50acu &&
              active.effect_parameter == 55u && active.anchored &&
              active.words.at(Runtime::object + 0x8cu) == 0xf000u &&
              active.words.at(Runtime::object + 0x90u) == 0xf001u &&
              active.visibility == 9u &&
              IndexOf(active.calls, "audio play") <
                  IndexOf(active.calls, "play music") &&
              IndexOf(active.calls, "release path") <
                  IndexOf(active.calls, "effect check") &&
              IndexOf(active.calls, "create effect") <
                  IndexOf(active.calls, "visibility"),
          "move entry plays sound and creates effect in order");
    Runtime fallback;
    fallback.words = active.words;
    fallback.words[Runtime::object + 0x78u] = 0u;
    fallback.words[0x5000u + 0x1a8u] = 0u;
    fallback.built_path = 0u;
    Check(pal4::MoveTask::Enter(Runtime::object, fallback) ==
              1u && fallback.played_path == 0xd000u &&
              fallback.effect_name == 0xd000u &&
              !fallback.anchored,
          "move entry uses empty-name fallback");
    Runtime failed;
    failed.words = active.words;
    failed.fail_play = true;
    bool thrown = false;
    try {
        (void)pal4::MoveTask::Enter(Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls.back() == "release path",
          "move entry releases temporary path on audio failure");
}

void CheckUpdate() {
    Runtime missing;
    missing.words[Runtime::object + 0x94u] = 0u;
    missing.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::MoveTask::Update(Runtime::object, 1.0f,
                                  missing) == 0u &&
              missing.bytes.at(Runtime::object + 0x3cu) == 0u,
          "move update clears completion without owner");
    Runtime done;
    SeedMovement(done);
    done.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::MoveTask::Update(Runtime::object, 1.0f,
                                  done) == 0u &&
              done.calls.empty(),
          "move update skips completed task");
    Runtime moving;
    SeedMovement(moving);
    moving.audio_active = true;
    Check(pal4::MoveTask::Update(Runtime::object, 5.0f,
                                  moving) == 1u &&
              moving.bytes.at(Runtime::object + 0x3cu) == 0u &&
              std::bit_cast<float>(moving.words.at(
                  Runtime::object + 0x40u)) == 5.0f &&
              moving.fractions ==
                  (std::vector<float>{0.5f, 0.5f, 0.5f}) &&
              moving.position_owner == 0x5000u &&
              moving.position.x == 6.0f &&
              moving.position.y == 7.0f &&
              moving.position.z == 8.0f &&
              std::find(moving.calls.begin(), moving.calls.end(),
                        "audio play") == moving.calls.end(),
          "move update interpolates position before completion");
    moving.calls.clear();
    Check(pal4::MoveTask::Update(Runtime::object, 6.0f,
                                  moving) == 1u &&
              moving.bytes.at(Runtime::object + 0x3cu) == 1u &&
              std::bit_cast<float>(moving.words.at(
                  Runtime::object + 0x40u)) == 10.0f &&
              moving.position.x == 11.0f &&
              moving.position.y == 12.0f &&
              moving.position.z == 13.0f &&
              IndexOf(moving.calls, "audio play") <
                  IndexOf(moving.calls, "set position"),
          "move update clamps elapsed, stops audio and reaches target");
}

void CheckInitialize() {
    Runtime empty;
    for (const auto offset : {0u, 4u, 8u})
        empty.words[0x6000u + offset] =
            std::bit_cast<std::uint32_t>(2.0f + offset / 4u);
    empty.words[Runtime::object + 0x64u] = 0u;
    empty.words[Runtime::object + 0x78u] = 0u;
    empty.string_lengths[0x9000u] = 0u;
    empty.string_lengths[0x9100u] = 0u;
    Check(pal4::MoveTask::Initialize(
              Runtime::object, 0x5000u,
              std::bit_cast<std::uint32_t>(10.0f), 0x6000u,
              0x9000u, 7u, 0x9100u, 8u, 0x7000u,
              empty) == 0x7000u &&
              empty.words.at(Runtime::object + 0x94u) == 0x5000u &&
              empty.words.at(Runtime::object + 0x44u) ==
                  std::bit_cast<std::uint32_t>(10.0f) &&
              empty.words.at(Runtime::object + 0x54u) ==
                  empty.words.at(0x6000u) &&
              empty.words.at(Runtime::object + 0x58u) ==
                  empty.words.at(0x6004u) &&
              empty.words.at(Runtime::object + 0x5cu) ==
                  empty.words.at(0x6008u) &&
              empty.bytes.at(Runtime::object + 0x3cu) == 0u &&
              empty.words.at(Runtime::object + 0x40u) == 0u &&
              empty.words.at(Runtime::object + 0x70u) == 7u &&
              empty.words.at(Runtime::object + 0x84u) == 8u &&
              empty.words.at(Runtime::object + 0x98u) == 0x7000u &&
              empty.reset_flags ==
                  (std::vector<std::uint8_t>{1u, 1u}),
          "move initializer writes vector, state and empty strings");

    Runtime shared;
    shared.words = empty.words;
    shared.words[Runtime::object + 0x64u] = 0x3000u;
    shared.words[Runtime::object + 0x78u] = 0x4000u;
    shared.bytes[0x2fffu] = 2u;
    shared.bytes[0x3fffu] = 2u;
    shared.string_lengths = empty.string_lengths;
    Check(pal4::MoveTask::Initialize(
              Runtime::object, 0x5000u, 0x41200000u,
              0x6000u, 0x9000u, 7u, 0x9100u, 8u,
              0x7000u, shared) == 8u &&
              shared.bytes.at(0x2fffu) == 1u &&
              shared.bytes.at(0x3fffu) == 1u &&
              shared.reset_flags ==
                  (std::vector<std::uint8_t>{0u, 0u}),
          "move initializer preserves raw EAX for shared empty effect");

    Runtime reused;
    reused.words = empty.words;
    reused.words[Runtime::object + 0x64u] = 0x3000u;
    reused.words[Runtime::object + 0x6cu] = 10u;
    reused.words[Runtime::object + 0x78u] = 0x4000u;
    reused.words[Runtime::object + 0x80u] = 10u;
    reused.bytes[0x2fffu] = 0u;
    reused.bytes[0x3fffu] = 0xffu;
    reused.string_lengths[0x9000u] = 3u;
    reused.string_lengths[0x9100u] = 5u;
    Check(pal4::MoveTask::Initialize(
              Runtime::object, 0x5000u, 0x41200000u,
              0x6000u, 0x9000u, 7u, 0x9100u, 8u,
              0x7000u, reused) == 0x7000u &&
              reused.reallocated_records.empty() &&
              reused.copied_destinations ==
                  (std::vector<std::uint32_t>{0x3000u, 0x4000u}) &&
              reused.words.at(Runtime::object + 0x68u) == 3u &&
              reused.words.at(Runtime::object + 0x7cu) == 5u &&
              reused.bytes.at(0x3003u) == 0u &&
              reused.bytes.at(0x4005u) == 0u,
          "move initializer reuses exclusive text buffers");

    Runtime reallocated;
    reallocated.words = reused.words;
    reallocated.words[Runtime::object + 0x64u] = 0x3000u;
    reallocated.words[Runtime::object + 0x78u] = 0x4000u;
    reallocated.bytes[0x2fffu] = 2u;
    reallocated.bytes[0x3fffu] = 2u;
    reallocated.string_lengths = reused.string_lengths;
    Check(pal4::MoveTask::Initialize(
              Runtime::object, 0x5000u, 0x41200000u,
              0x6000u, 0x9000u, 7u, 0x9100u, 8u,
              0x7000u, reallocated) == 0x7000u &&
              reallocated.reallocated_records ==
                  (std::vector<std::uint32_t>{0x1060u, 0x1074u}) &&
              reallocated.copied_destinations ==
                  (std::vector<std::uint32_t>{0xc000u, 0xc100u}),
          "move initializer reallocates shared nonempty text");

    Runtime too_long;
    for (const auto offset : {0u, 4u, 8u})
        too_long.words[0x6000u + offset] =
            empty.words.at(0x6000u + offset);
    too_long.string_lengths[0x9000u] = 0xfffffffeu;
    bool thrown = false;
    try {
        (void)pal4::MoveTask::Initialize(
            Runtime::object, 0x5000u, 0x41200000u,
            0x6000u, 0x9000u, 7u, 0x9100u, 8u,
            0x7000u, too_long);
    } catch (const std::length_error&) {
        thrown = true;
    }
    Check(thrown && too_long.calls.back() == "throw length" &&
              too_long.words.find(Runtime::object + 0x70u) ==
                  too_long.words.end(),
          "move initializer rejects oversized sound before time write");
}

void CheckHooksAndFollowup() {
    Runtime hooks;
    Check(pal4::MoveTask::StopSoundIfPlaying(
              Runtime::object, hooks) == 0u &&
              hooks.calls ==
                  (std::vector<std::string>{"is playing"}),
          "move stop hook skips inactive sound");
    hooks.calls.clear();
    hooks.is_playing = 1u;
    Check(pal4::MoveTask::StopSoundIfPlaying(
              Runtime::object, hooks) == 0x72u &&
              hooks.calls ==
                  (std::vector<std::string>{"is playing", "stop sound"}),
          "move stop hook forwards active result");
    hooks.calls.clear();
    Check(pal4::MoveTask::PauseSoundIfNotPlaying(
              Runtime::object, hooks) == 1u &&
              hooks.calls ==
                  (std::vector<std::string>{"is playing"}),
          "move pause hook keeps playing result");
    hooks.calls.clear();
    hooks.is_playing = 0u;
    Check(pal4::MoveTask::PauseSoundIfNotPlaying(
              Runtime::object, hooks) == 0x73u &&
              hooks.calls ==
                  (std::vector<std::string>{"is playing", "pause sound"}),
          "move pause hook forwards inactive result");

    Runtime descriptor;
    Check(pal4::MoveTaskFactory::ConstructDescriptor(
              0x2000u, descriptor) == 0x2000u &&
              descriptor.words.at(0x2000u) == 0x84887cu,
          "move factory descriptor");
    SpawnRuntime factory;
    Check(pal4::MoveTaskFactory::Create(factory) == 0xd000u &&
              factory.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x9cu}) &&
              factory.words.at(0xd000u) == 0x848858u &&
              factory.calls.front() == "allocate",
          "move factory allocation and constructor");
    SpawnRuntime null_factory;
    null_factory.allocations = {0u};
    Check(pal4::MoveTaskFactory::Create(null_factory) == 0u &&
              null_factory.calls ==
                  (std::vector<std::string>{"allocate"}),
          "move factory null allocation");
    SpawnRuntime failed_factory;
    failed_factory.fail_name_node = true;
    bool thrown = false;
    try {
        (void)pal4::MoveTaskFactory::Create(failed_factory);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed_factory.freed ==
                        (std::vector<std::uint32_t>{0xd000u}) &&
              failed_factory.calls.back() == "free 53248" &&
              IndexOf(failed_factory.calls, "base destruct") <
                  IndexOf(failed_factory.calls, "free 53248"),
          "move factory exception cleanup");

    SpawnRuntime no_context;
    no_context.words[Runtime::object + 0x98u] = 0u;
    Check(pal4::MoveTask::SpawnReverseAndFollowup(
              Runtime::object, no_context) == 0u &&
              no_context.calls.empty(),
          "move follow-up context gate");
    SpawnRuntime no_owner;
    no_owner.words[Runtime::object + 0x94u] = 0u;
    Check(pal4::MoveTask::SpawnReverseAndFollowup(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(),
          "move follow-up owner gate");
    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0x6000u;
    Check(pal4::MoveTask::SpawnReverseAndFollowup(
              Runtime::object, previous) == 1u &&
              previous.registered == 0xd000u &&
              previous.inserted == 0xd000u &&
              previous.words.at(0xd094u) == SpawnRuntime::owner &&
              previous.words.at(0xd054u) ==
                  previous.words.at(Runtime::object + 0x48u) &&
              previous.words.at(0xd098u) == 0u &&
              previous.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x9cu}) &&
              IndexOf(previous.calls, "add reverse") <
                  IndexOf(previous.calls, "copy text") &&
              IndexOf(previous.calls, "insert reverse") <
                  IndexOf(previous.calls, "previous"),
          "move reverse child before previous callback");
    SpawnRuntime helper;
    Check(pal4::MoveTask::SpawnReverseAndFollowup(
              Runtime::object, helper) == 1u &&
              helper.added_helper == 0xb000u &&
              helper.words.at(0xa004u) == 0xb000u &&
              helper.words.at(SpawnRuntime::collection + 8u) ==
                  0xa008u &&
              helper.processed == 0x7000u &&
              helper.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x9cu, 0x40u}),
          "move follow-up appends helper");
    SpawnRuntime growth;
    growth.words[SpawnRuntime::collection + 12u] = 0xa004u;
    growth.allocations.push_back(0xe000u);
    Check(pal4::MoveTask::SpawnReverseAndFollowup(
              Runtime::object, growth) == 1u &&
              growth.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x9cu, 0x40u, 8u}) &&
              growth.words.at(0xe000u) == 0xaaaa0000u &&
              growth.words.at(0xe004u) == 0xb000u &&
              growth.words.at(SpawnRuntime::collection + 8u) ==
                  0xe008u,
          "move follow-up grows helper collection");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckDestruct();
        CheckEnter();
        CheckUpdate();
        CheckInitialize();
        CheckHooksAndFollowup();
        std::cout << "move_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
