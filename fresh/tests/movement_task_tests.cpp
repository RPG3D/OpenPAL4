#include "pal4/movement_task.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <deque>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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

struct Runtime : virtual pal4::IMovementTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t owner = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::map<std::uint32_t, std::uint32_t> string_lengths;
    std::vector<std::uint8_t> reset_flags;
    std::vector<std::uint32_t> reallocated_records;
    std::vector<std::uint32_t> copied_destinations;
    bool fail_name_node = false;
    bool audio_active = false;
    bool effect_needs_update = false;
    std::uint32_t path = 0xa000u;
    std::uint32_t played_path = 0u;
    std::uint32_t played_parameter = 0u;
    std::uint32_t effect_name = 0u;
    std::uint32_t effect_position = 0u;
    std::uint32_t effect_parameter = 0u;
    bool anchored = false;
    std::uint32_t visibility = 0u;
    pal4::MoveTaskVector position{};
    pal4::MoveTaskVector rotation{};
    unsigned transient_calls = 0u;
    std::uint8_t is_playing = 0u;
    std::uint8_t stopped = 0x71u;
    std::uint8_t paused = 0x73u;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object || address == 0xe000u,
              "movement base construct");
        calls.emplace_back("base construct");
    }
    void BaseDestruct(std::uint32_t address) override {
        Check(address == object, "movement base destruct");
        calls.emplace_back("base destruct");
    }
    std::uint8_t TransientInitialByte() override {
        ++transient_calls;
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
        calls.emplace_back("write byte");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
    void ConstructNameNode(std::uint32_t address) override {
        Check(address == object + 0xb0u || address == 0xe0b0u,
              "movement name node address");
        calls.emplace_back("name node");
        if (fail_name_node) throw std::runtime_error("name node failed");
    }
    void ReleaseText(std::uint32_t record) override {
        calls.push_back(record == object + 0x98u ?
                        "release effect" : "release sound");
    }
    bool AudioIsActive(std::uint32_t handle) override {
        Check(handle == object + 0xacu,
              "movement audio handle");
        calls.emplace_back("audio check");
        return audio_active;
    }
    void AudioPlaySound(std::uint32_t handle) override {
        Check(handle == object + 0xacu,
              "movement playback handle");
        calls.emplace_back("audio play");
    }
    bool EffectNeedsUpdate(std::uint32_t handle) override {
        Check(handle == object + 0xb0u,
              "movement effect handle");
        calls.emplace_back("effect check");
        return effect_needs_update;
    }
    void EffectUpdate(std::uint32_t handle) override {
        Check(handle == object + 0xb0u,
              "movement effect update handle");
        calls.emplace_back("effect update");
    }
    void EffectReset(std::uint32_t handle) override {
        Check(handle == object + 0xb0u,
              "movement effect reset handle");
        calls.emplace_back("effect reset");
    }
    std::uint32_t BuildSoundPath(std::uint32_t record,
                                 std::string_view suffix) override {
        Check(record == object + 0x84u && suffix == ".wav",
              "movement sound path");
        calls.emplace_back("build path");
        return path;
    }
    void ReleaseSoundPath(std::uint32_t address) override {
        Check(address == path, "movement released path");
        calls.emplace_back("release path");
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty string");
        return 0xd000u;
    }
    std::uint32_t PlayMusic(std::uint32_t address,
                            std::uint32_t parameter) override {
        calls.emplace_back("play music");
        played_path = address;
        played_parameter = parameter;
        return 0xc000u;
    }
    pal4::MoveTaskEffectHandle CreateEffect(
        std::uint32_t name, std::uint32_t address,
        std::uint32_t parameter, bool anchor) override {
        calls.emplace_back("create effect");
        effect_name = name;
        effect_position = address;
        effect_parameter = parameter;
        anchored = anchor;
        return {0xf000u, 0xf001u};
    }
    void ReleaseTemporaryEffect(
        pal4::MoveTaskEffectHandle handle) override {
        Check(handle.first == 0xf000u && handle.second == 0xf001u,
              "movement temporary effect");
        calls.emplace_back("release temporary effect");
    }
    void SetEffectVisibility(std::uint32_t handle,
                             std::uint32_t value) override {
        Check(handle == object + 0xb0u,
              "movement effect visibility handle");
        calls.emplace_back("visibility");
        visibility = value;
    }
    float InterpolateFloat(float start, float end,
                           float fraction,
                           std::uint32_t mode) override {
        Check(mode == 0u, "movement interpolation mode");
        calls.emplace_back("interpolate");
        return start + (end - start) * fraction;
    }
    void SetOwnerPosition(std::uint32_t address,
                          const pal4::MoveTaskVector& value,
                          std::uint32_t mode) override {
        Check(address == owner && mode == 0u,
              "movement position setter");
        calls.emplace_back("set position");
        position = value;
    }
    void SetOwnerRotation(std::uint32_t address,
                          const pal4::MoveTaskVector& value,
                          std::uint32_t mode) override {
        Check(address == owner && mode == 0u,
              "movement rotation setter");
        calls.emplace_back("set rotation");
        rotation = value;
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        calls.emplace_back("string length");
        return string_lengths.at(source);
    }
    [[noreturn]] void ThrowLength() override {
        throw std::length_error("movement name too long");
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
        words[record + 4u] = (record & 0xffu) == 0x84u ?
            0xc000u : 0xc100u;
        words[record + 12u] = length;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(length == string_lengths.at(source),
              "movement copied text length");
        calls.emplace_back("copy text");
        copied_destinations.push_back(destination);
    }
    std::uint8_t AudioIsPlaying(std::uint32_t handle) override {
        Check(handle == object + 0xacu, "movement sound handle");
        calls.emplace_back("is playing");
        return is_playing;
    }
    std::uint8_t AudioStopSound(std::uint32_t handle) override {
        Check(handle == object + 0xacu, "movement stop handle");
        calls.emplace_back("stop sound");
        return stopped;
    }
    std::uint8_t AudioPauseSound(std::uint32_t handle) override {
        Check(handle == object + 0xacu, "movement pause handle");
        calls.emplace_back("pause sound");
        return paused;
    }
};

struct SpawnRuntime final : Runtime, pal4::IMovementTaskSpawnRuntime {
    static constexpr std::uint32_t child = 0xe000u;
    static constexpr std::uint32_t helper = 0xb000u;
    static constexpr std::uint32_t context = 0x7000u;
    static constexpr std::uint32_t collection = owner + 0x2a0u;
    std::deque<std::uint32_t> allocations{child, helper};
    std::vector<std::uint32_t> allocation_sizes;
    std::uint32_t registered = 0u;
    std::uint32_t inserted = 0u;
    std::uint32_t added_helper = 0u;
    std::uint32_t processed = 0u;

    SpawnRuntime() {
        words[object + 0xb8u] = context;
        words[object + 0x3cu] = owner;
        words[object + 0x30u] = 0u;
        words[object + 0x48u] = std::bit_cast<std::uint32_t>(10.0f);
        words[object + 0x88u] = 0x9000u;
        words[object + 0x94u] = 7u;
        words[object + 0x9cu] = 0x9100u;
        words[object + 0xa8u] = 8u;
        for (const auto offset : {0u, 4u, 8u}) {
            words[object + 0x54u + offset] =
                std::bit_cast<std::uint32_t>(1.0f + offset / 4u);
            words[object + 0x6cu + offset] =
                std::bit_cast<std::uint32_t>(4.0f + offset / 4u);
        }
        string_lengths[0x9000u] = 3u;
        string_lengths[0x9100u] = 5u;
        string_lengths[0xd000u] = 0u;
        words[collection + 4u] = 0xa000u;
        words[collection + 8u] = 0xa004u;
        words[collection + 12u] = 0xa008u;
        words[0xa000u] = 0xaaaa0000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        allocation_sizes.push_back(size);
        calls.emplace_back("allocate");
        const auto address = allocations.front();
        allocations.pop_front();
        return address;
    }
    void AddReverseTask(std::uint32_t task_context,
                        std::uint32_t task,
                        std::uint32_t timing) override {
        Check(task_context == context && task == child &&
                  timing == object + 0x0cu,
              "movement reverse registration");
        calls.emplace_back("add reverse");
        registered = task;
    }
    void InsertOwnerTask(std::uint32_t address,
                         std::uint32_t task) override {
        Check(address == collection, "movement owner collection");
        calls.emplace_back("insert reverse");
        inserted = task;
    }
    void NotifyPreviousTask(std::uint32_t previous) override {
        Check(previous == 0x6000u, "movement previous task");
        calls.emplace_back("previous");
    }
    std::uint32_t ConstructHelperTask(std::uint32_t memory) override {
        Check(memory == helper, "movement helper memory");
        calls.emplace_back("construct helper");
        return memory;
    }
    void AddHelperTask(std::uint32_t task_context,
                       std::uint32_t task,
                       std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        Check(task_context == context && task == helper &&
                  first == 1u && second == 0u && third == 0u,
              "movement helper timing");
        calls.emplace_back("add helper");
        added_helper = task;
    }
    void SetHelperOwner(std::uint32_t task,
                        std::uint32_t task_owner) override {
        Check((task == helper || task == 0u) && task_owner == owner,
              "movement helper owner");
        calls.emplace_back("set helper owner");
    }
    void MarkProcessed(std::uint32_t task_context) override {
        Check(task_context == context, "movement processed context");
        calls.emplace_back("processed");
        processed = task_context;
    }
};

void Seed(Runtime& runtime, float start_y = 10.0f,
          float target_y = 20.0f) {
    runtime.words[Runtime::object + 0x3cu] = Runtime::owner;
    runtime.words[Runtime::object + 0x44u] = 0u;
    runtime.words[Runtime::object + 0x48u] =
        std::bit_cast<std::uint32_t>(10.0f);
    runtime.bytes[Runtime::object + 0x40u] = 0u;
    runtime.words[Runtime::object + 0x8cu] = 0u;
    runtime.words[Runtime::object + 0xa0u] = 0u;
    for (const auto offset : {0u, 4u, 8u}) {
        const auto index = offset / 4u;
        runtime.words[Runtime::owner + 0xacu + offset] =
            std::bit_cast<std::uint32_t>(1.0f + index);
        runtime.words[Runtime::owner + 0xa0u + offset] =
            std::bit_cast<std::uint32_t>(index == 1u ?
                                             start_y : 0.0f);
        runtime.words[Runtime::object + 0x60u + offset] =
            std::bit_cast<std::uint32_t>(11.0f + index);
    }
    runtime.words[Runtime::object + 0x7cu] =
        std::bit_cast<std::uint32_t>(target_y);
}

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::MovementTask::Construct(Runtime::object, runtime) ==
              Runtime::object && runtime.transient_calls == 1u &&
              runtime.bytes.at(Runtime::object + 0x84u) == 0x5au &&
              runtime.bytes.at(Runtime::object + 0x98u) == 0x5au &&
              runtime.words.at(Runtime::object) == 0x8488a8u &&
              runtime.words.at(Runtime::object + 0x88u) == 0u &&
              runtime.words.at(Runtime::object + 0x9cu) == 0u &&
              runtime.words.at(Runtime::object + 0xacu) == 0u &&
              runtime.words.at(Runtime::object + 0x3cu) == 0u &&
              runtime.words.at(Runtime::object + 0xb8u) == 0u &&
              IndexOf(runtime.calls, "base construct") <
                  IndexOf(runtime.calls, "name node"),
          "movement constructor layout and order");
    Runtime failed;
    failed.fail_name_node = true;
    bool thrown = false;
    try {
        (void)pal4::MovementTask::Construct(Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls[failed.calls.size() - 3u] ==
                        "release effect" &&
              failed.calls[failed.calls.size() - 2u] ==
                  "release sound" &&
              failed.calls.back() == "base destruct",
          "movement constructor exception cleanup");
    for (const auto count : {std::uint8_t{0u},
                             std::uint8_t{2u},
                             std::uint8_t{0xffu}}) {
        Runtime destructing;
        destructing.words[Runtime::object + 0x9cu] = 0x3000u;
        destructing.words[Runtime::object + 0x88u] = 0x4000u;
        destructing.bytes[0x2fffu] = count;
        destructing.bytes[0x3fffu] = count;
        destructing.audio_active = true;
        destructing.effect_needs_update = true;
        pal4::MovementTask::Destruct(Runtime::object, destructing);
        Check(destructing.words.at(Runtime::object) == 0x8486fcu &&
                  destructing.words.at(Runtime::object + 0x9cu) == 0u &&
                  destructing.words.at(Runtime::object + 0xa0u) == 0u &&
                  destructing.words.at(Runtime::object + 0x88u) == 0u &&
                  destructing.words.at(Runtime::object + 0x8cu) == 0u &&
                  (count == 2u ?
                       destructing.bytes.at(0x2fffu) == 1u &&
                           destructing.bytes.at(0x3fffu) == 1u :
                       destructing.freed.size() == 2u) &&
                  IndexOf(destructing.calls, "audio play") <
                      IndexOf(destructing.calls, "effect update") &&
                  IndexOf(destructing.calls, "effect update") <
                      IndexOf(destructing.calls, "effect reset") &&
                  destructing.calls.back() == "base destruct",
              "movement destructor reference and effect lifecycle");
    }
}

void CheckEnterAngles() {
    Runtime missing;
    missing.words[Runtime::object + 0x3cu] = 0u;
    Check(pal4::MovementTask::Enter(Runtime::object, missing) ==
              0u && missing.calls.empty(),
          "movement entry owner gate");
    for (const auto sample : {
             std::pair{10.0f, 20.0f},
             std::pair{10.0f, 350.0f},
             std::pair{350.0f, 10.0f},
             std::pair{180.0f, 0.0f}}) {
        Runtime runtime;
        Seed(runtime, sample.first, sample.second);
        Check(pal4::MovementTask::Enter(Runtime::object,
                                         runtime) == 1u &&
                  runtime.words.at(Runtime::object + 0x44u) == 0u &&
                  runtime.words.at(Runtime::object + 0x54u) ==
                      runtime.words.at(Runtime::owner + 0xacu) &&
                  runtime.words.at(Runtime::object + 0x70u) ==
                      runtime.words.at(Runtime::owner + 0xa4u) &&
                  std::bit_cast<float>(runtime.words.at(
                      Runtime::object + 0x4cu)) ==
                      (sample.first == 10.0f &&
                               sample.second == 350.0f ? -20.0f :
                       sample.first == 350.0f ? 20.0f :
                       sample.first == 180.0f ? -180.0f : 10.0f) &&
                  std::bit_cast<float>(runtime.words.at(
                      Runtime::object + 0x50u)) ==
                      std::bit_cast<float>(runtime.words.at(
                          Runtime::object + 0x4cu)) / 10.0f,
              "movement entry snapshots transforms and shortest yaw");
    }
}

void CheckEnterResourcesAndUpdate() {
    Runtime active;
    Seed(active);
    active.words[Runtime::object + 0x8cu] = 3u;
    active.words[Runtime::object + 0x94u] =
        static_cast<std::uint32_t>(-3);
    active.words[Runtime::object + 0xa0u] = 4u;
    active.words[Runtime::object + 0x9cu] = 0x9000u;
    active.words[Runtime::object + 0xa8u] = 55u;
    active.words[Runtime::owner + 0x1a8u] = 1u;
    active.words[Runtime::owner + 0x14u] = 0x6000u;
    active.words[0x6004u] = 9u;
    active.audio_active = true;
    active.effect_needs_update = true;
    Check(pal4::MovementTask::Enter(Runtime::object, active) ==
              1u && active.played_path == 0xa000u &&
              active.played_parameter == 0u &&
              active.words.at(Runtime::object + 0x94u) == 0u &&
              active.words.at(Runtime::object + 0xacu) == 0xc000u &&
              active.effect_name == 0x9000u &&
              active.effect_position == Runtime::owner + 0xacu &&
              active.effect_parameter == 55u && active.anchored &&
              active.visibility == 9u &&
              IndexOf(active.calls, "release path") <
                  IndexOf(active.calls, "create effect"),
          "movement entry sound and effect resources");
    Runtime fallback;
    fallback.words = active.words;
    fallback.words[Runtime::object + 0x9cu] = 0u;
    fallback.words[Runtime::owner + 0x1a8u] = 0u;
    fallback.path = 0u;
    Check(pal4::MovementTask::Enter(Runtime::object, fallback) ==
              1u && fallback.played_path == 0xd000u &&
              fallback.effect_name == 0xd000u &&
              !fallback.anchored,
          "movement entry uses empty resource fallback");

    Runtime no_owner;
    no_owner.words[Runtime::object + 0x3cu] = 0u;
    no_owner.bytes[Runtime::object + 0x40u] = 1u;
    Check(pal4::MovementTask::Update(Runtime::object, 1.0f,
                                      no_owner) == 0u &&
              no_owner.bytes.at(Runtime::object + 0x40u) == 0u,
          "movement update owner gate clears completion");
    Runtime moving;
    Seed(moving);
    (void)pal4::MovementTask::Enter(Runtime::object, moving);
    moving.calls.clear();
    moving.audio_active = true;
    Check(pal4::MovementTask::Update(Runtime::object, 5.0f,
                                      moving) == 1u &&
              moving.position.x == 6.0f &&
              moving.position.y == 7.0f &&
              moving.position.z == 8.0f &&
              moving.rotation.x == 0.0f &&
              moving.rotation.y == 15.0f &&
              moving.rotation.z == 0.0f &&
              moving.bytes.at(Runtime::object + 0x40u) == 0u &&
              IndexOf(moving.calls, "set position") <
                  IndexOf(moving.calls, "set rotation"),
          "movement update interpolates position and yaw");
    moving.calls.clear();
    Check(pal4::MovementTask::Update(Runtime::object, 6.0f,
                                      moving) == 1u &&
              moving.position.x == 11.0f &&
              moving.rotation.y == 20.0f &&
              moving.bytes.at(Runtime::object + 0x40u) == 1u &&
              std::bit_cast<float>(moving.words.at(
                  Runtime::object + 0x44u)) == 10.0f &&
              IndexOf(moving.calls, "audio play") <
                  IndexOf(moving.calls, "set position"),
          "movement update clamps endpoint and services sound");
    moving.calls.clear();
    Check(pal4::MovementTask::Update(Runtime::object, 1.0f,
                                      moving) == 0u &&
              moving.calls.empty(),
          "movement update skips completed task");
}

void CheckCapturedM14Yaw() {
    Runtime runtime;
    // Pair246 trans01: original task words at the first held M14/3 sample.
    Seed(runtime, std::bit_cast<float>(0x43b27fcau),
         std::bit_cast<float>(0x4283f649u));
    runtime.words[Runtime::object + 0x48u] = 0x40200000u; // 2.5 seconds
    Check(pal4::MovementTask::Enter(Runtime::object, runtime) == 1u &&
              runtime.words.at(Runtime::object + 0x4cu) == 0x4289f721u &&
              runtime.words.at(Runtime::object + 0x50u) == 0x41dcbe9bu,
          "captured M14 movement yaw delta/speed retain original operation order");
}

void CheckInitializeAndHooks() {
    Runtime runtime;
    runtime.words[Runtime::object + 0x88u] = 0u;
    runtime.words[Runtime::object + 0x90u] = 0u;
    runtime.words[Runtime::object + 0x9cu] = 0u;
    runtime.words[Runtime::object + 0xa4u] = 0u;
    runtime.string_lengths[0x9000u] = 3u;
    runtime.string_lengths[0x9100u] = 5u;
    const pal4::MoveTaskVector target{11.0f, 12.0f, 13.0f};
    const pal4::MoveTaskVector rotation{21.0f, 22.0f, 23.0f};
    Check(pal4::MovementTask::Initialize(
              Runtime::object, Runtime::owner,
              std::bit_cast<std::uint32_t>(10.0f), target,
              rotation, 0x9000u, 7u, 0x9100u, 8u,
              0x7000u, runtime) == 0x7000u &&
              runtime.words.at(Runtime::object + 0x3cu) ==
                  Runtime::owner &&
              runtime.bytes.at(Runtime::object + 0x40u) == 0u &&
              runtime.words.at(Runtime::object + 0x60u) ==
                  std::bit_cast<std::uint32_t>(11.0f) &&
              runtime.words.at(Runtime::object + 0x7cu) ==
                  std::bit_cast<std::uint32_t>(22.0f) &&
              runtime.words.at(Runtime::object + 0x94u) == 7u &&
              runtime.words.at(Runtime::object + 0xa8u) == 8u &&
              runtime.words.at(Runtime::object + 0xb8u) == 0x7000u &&
              runtime.reallocated_records ==
                  (std::vector<std::uint32_t>{Runtime::object + 0x84u,
                                              Runtime::object + 0x98u}) &&
              runtime.copied_destinations ==
                  (std::vector<std::uint32_t>{0xc000u, 0xc100u}),
          "movement initializer fields and text allocation");
    runtime.bytes[Runtime::object + 0x40u] = 0x5au;
    Check(pal4::MovementTask::IsComplete(Runtime::object, runtime) ==
              0x5au, "movement completion byte");
    runtime.calls.clear();
    runtime.is_playing = 1u;
    Check(pal4::MovementTask::StopSoundIfPlaying(
              Runtime::object, runtime) == 0x71u &&
              runtime.calls ==
                  (std::vector<std::string>{"is playing", "stop sound"}),
          "movement stop playing sound");
    runtime.calls.clear();
    runtime.is_playing = 0u;
    Check(pal4::MovementTask::StopSoundIfPlaying(
              Runtime::object, runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"is playing"}),
          "movement stop skips inactive sound");
    runtime.calls.clear();
    Check(pal4::MovementTask::PauseSoundIfNotPlaying(
              Runtime::object, runtime) == 0x73u &&
              runtime.calls ==
                  (std::vector<std::string>{"is playing", "pause sound"}),
          "movement pause inactive sound");
    runtime.calls.clear();
    runtime.is_playing = 1u;
    Check(pal4::MovementTask::PauseSoundIfNotPlaying(
              Runtime::object, runtime) == 1u &&
              runtime.calls ==
                  (std::vector<std::string>{"is playing"}),
          "movement pause skips playing sound");

    Runtime shared;
    shared.words[Runtime::object + 0x88u] = 0x3000u;
    shared.words[Runtime::object + 0x9cu] = 0x4000u;
    shared.bytes[0x2fffu] = 2u;
    shared.bytes[0x3fffu] = 2u;
    shared.string_lengths[0xd000u] = 0u;
    Check(pal4::MovementTask::Initialize(
              Runtime::object, Runtime::owner, 0u, target, rotation,
              0xd000u, 7u, 0xd000u, 8u, 0x7000u, shared) == 8u &&
              shared.bytes.at(0x2fffu) == 1u &&
              shared.bytes.at(0x3fffu) == 1u &&
              shared.reset_flags ==
                  (std::vector<std::uint8_t>{0u, 0u}),
          "movement shared empty names and raw return");
    Runtime ordinary_empty;
    ordinary_empty.words[Runtime::object + 0x88u] = 0u;
    ordinary_empty.words[Runtime::object + 0x9cu] = 0u;
    ordinary_empty.string_lengths[0xd000u] = 0u;
    Check(pal4::MovementTask::Initialize(
              Runtime::object, Runtime::owner, 0u, target, rotation,
              0xd000u, 7u, 0xd000u, 8u, 0x7000u,
              ordinary_empty) == 0x7000u &&
              ordinary_empty.reset_flags ==
                  (std::vector<std::uint8_t>{1u, 1u}),
          "movement ordinary empty names");
    Runtime reused;
    reused.words[Runtime::object + 0x88u] = 0x3000u;
    reused.words[Runtime::object + 0x90u] = 10u;
    reused.words[Runtime::object + 0x9cu] = 0x4000u;
    reused.words[Runtime::object + 0xa4u] = 10u;
    reused.bytes[0x2fffu] = 0u;
    reused.bytes[0x3fffu] = 0xffu;
    reused.string_lengths[0x9000u] = 3u;
    reused.string_lengths[0x9100u] = 5u;
    (void)pal4::MovementTask::Initialize(
        Runtime::object, Runtime::owner, 0u, target, rotation,
        0x9000u, 7u, 0x9100u, 8u, 0x7000u, reused);
    Check(reused.reallocated_records.empty() &&
              reused.copied_destinations ==
                  (std::vector<std::uint32_t>{0x3000u, 0x4000u}),
          "movement reuses exclusive name buffers");
    Runtime too_long;
    too_long.string_lengths[0x9000u] = 0xfffffffeu;
    bool threw = false;
    try {
        (void)pal4::MovementTask::Initialize(
            Runtime::object, Runtime::owner, 0u, target, rotation,
            0x9000u, 7u, 0x9100u, 8u, 0x7000u, too_long);
    } catch (const std::length_error&) {
        threw = true;
    }
    Check(threw, "movement oversized name rejected");
}

void CheckFactoryAndFollowup() {
    Runtime descriptor;
    Check(pal4::MovementTaskFactory::ConstructDescriptor(
              0x2000u, descriptor) == 0x2000u &&
              descriptor.words.at(0x2000u) == 0x8488ccu,
          "movement factory descriptor");
    SpawnRuntime factory;
    Check(pal4::MovementTaskFactory::Create(factory) ==
              SpawnRuntime::child &&
              factory.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xbcu}) &&
              factory.words.at(SpawnRuntime::child) == 0x8488a8u,
          "movement factory allocates and constructs");
    SpawnRuntime null_factory;
    null_factory.allocations = {0u};
    Check(pal4::MovementTaskFactory::Create(null_factory) == 0u &&
              null_factory.calls ==
                  (std::vector<std::string>{"allocate"}),
          "movement factory null allocation");
    SpawnRuntime failed_factory;
    failed_factory.fail_name_node = true;
    bool threw = false;
    try {
        (void)pal4::MovementTaskFactory::Create(failed_factory);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failed_factory.freed ==
                        (std::vector<std::uint32_t>{SpawnRuntime::child}) &&
              failed_factory.calls.back() == "free",
          "movement factory frees failed construction");

    SpawnRuntime no_context;
    no_context.words[Runtime::object + 0xb8u] = 0u;
    Check(pal4::MovementTask::SpawnFollowup(
              Runtime::object, no_context) == 0u &&
              no_context.calls.empty(), "movement context gate");
    SpawnRuntime no_owner;
    no_owner.words[Runtime::object + 0x3cu] = 0u;
    Check(pal4::MovementTask::SpawnFollowup(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(), "movement owner gate");
    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0x6000u;
    Check(pal4::MovementTask::SpawnFollowup(
              Runtime::object, previous) == 1u &&
              previous.registered == SpawnRuntime::child &&
              previous.inserted == SpawnRuntime::child &&
              previous.words.at(SpawnRuntime::child + 0x3cu) ==
                  Runtime::owner &&
              previous.words.at(SpawnRuntime::child + 0x60u) ==
                  previous.words.at(Runtime::object + 0x54u) &&
              previous.words.at(SpawnRuntime::child + 0x7cu) ==
                  previous.words.at(Runtime::object + 0x70u) &&
              previous.words.at(SpawnRuntime::child + 0x94u) == 7u &&
              previous.words.at(SpawnRuntime::child + 0xa8u) == 8u &&
              previous.words.at(SpawnRuntime::child + 0xb8u) == 0u &&
              IndexOf(previous.calls, "add reverse") <
                  IndexOf(previous.calls, "copy text") &&
              IndexOf(previous.calls, "insert reverse") <
                  IndexOf(previous.calls, "previous"),
          "movement reverse child before previous callback");
    SpawnRuntime followup;
    Check(pal4::MovementTask::SpawnFollowup(
              Runtime::object, followup) == 1u &&
              followup.added_helper == SpawnRuntime::helper &&
              followup.words.at(0xa004u) == SpawnRuntime::helper &&
              followup.words.at(SpawnRuntime::collection + 8u) ==
                  0xa008u &&
              followup.processed == SpawnRuntime::context &&
              followup.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xbcu, 0x40u}),
          "movement follow-up appends helper");
    SpawnRuntime growth;
    growth.words[SpawnRuntime::collection + 12u] = 0xa004u;
    growth.allocations.push_back(0xf000u);
    Check(pal4::MovementTask::SpawnFollowup(
              Runtime::object, growth) == 1u &&
              growth.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xbcu, 0x40u, 8u}) &&
              growth.words.at(0xf000u) == 0xaaaa0000u &&
              growth.words.at(0xf004u) == SpawnRuntime::helper &&
              growth.words.at(SpawnRuntime::collection + 8u) ==
                  0xf008u,
          "movement follow-up grows helper collection");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckEnterAngles();
        CheckCapturedM14Yaw();
        CheckEnterResourcesAndUpdate();
        CheckInitializeAndHooks();
        CheckFactoryAndFollowup();
        std::cout << "movement_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
