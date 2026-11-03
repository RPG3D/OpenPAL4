#include "pal4/rotate_task.h"

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

struct Runtime : virtual pal4::IRotateTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t owner = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> string_lengths;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint8_t> reset_flags;
    std::vector<std::uint32_t> reallocated;
    std::vector<std::uint32_t> copied;
    bool fail_name_node = false;
    bool audio_active = false;
    bool effect_active = false;
    std::uint8_t playing = 0u;
    std::uint32_t path = 0xa000u;
    std::uint32_t music_name = 0u;
    std::uint32_t music_parameter = 0u;
    std::uint32_t effect_name = 0u;
    std::uint32_t effect_position = 0u;
    std::uint32_t effect_parameter = 0u;
    bool anchored = false;
    std::uint32_t visibility = 0u;
    pal4::MoveTaskVector rotation{};
    unsigned transient_count = 0u;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object || address == 0xe000u,
              "rotate base constructor address");
        calls.emplace_back("base construct");
    }
    void BaseDestruct(std::uint32_t) override {
        calls.emplace_back("base destruct");
    }
    std::uint8_t TransientInitialByte() override {
        ++transient_count;
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
        Check(address == object + 0x90u || address == 0xe090u,
              "rotate name node address");
        calls.emplace_back("name node");
        if (fail_name_node) throw std::runtime_error("name node failed");
    }
    void ReleaseText(std::uint32_t record) override {
        calls.emplace_back(record == object + 0x78u ||
                                   record == 0xe078u ?
                               "release effect" : "release sound");
    }
    bool AudioIsActive(std::uint32_t handle) override {
        Check(handle == object + 0x8cu, "rotate audio handle");
        calls.emplace_back("audio check");
        return audio_active;
    }
    void AudioPlaySound(std::uint32_t handle) override {
        Check(handle == object + 0x8cu, "rotate playback handle");
        calls.emplace_back("audio play");
    }
    bool EffectNeedsUpdate(std::uint32_t handle) override {
        Check(handle == object + 0x90u, "rotate effect handle");
        calls.emplace_back("effect check");
        return effect_active;
    }
    void EffectUpdate(std::uint32_t) override {
        calls.emplace_back("effect update");
    }
    void EffectReset(std::uint32_t) override {
        calls.emplace_back("effect reset");
    }
    std::uint32_t BuildSoundPath(std::uint32_t record,
                                 std::string_view suffix) override {
        Check(record == object + 0x64u && suffix == ".wav",
              "rotate sound path");
        calls.emplace_back("build path");
        return path;
    }
    void ReleaseSoundPath(std::uint32_t address) override {
        Check(address == path, "rotate release path");
        calls.emplace_back("release path");
    }
    std::uint32_t EmptyStringAddress() override { return 0xd000u; }
    std::uint32_t PlayMusic(std::uint32_t name,
                            std::uint32_t parameter) override {
        calls.emplace_back("play music");
        music_name = name;
        music_parameter = parameter;
        return 0xc000u;
    }
    pal4::MoveTaskEffectHandle CreateEffect(
        std::uint32_t name, std::uint32_t position,
        std::uint32_t parameter, bool anchor) override {
        calls.emplace_back("create effect");
        effect_name = name;
        effect_position = position;
        effect_parameter = parameter;
        anchored = anchor;
        return {0xf000u, 0xf001u};
    }
    void ReleaseTemporaryEffect(pal4::MoveTaskEffectHandle handle) override {
        Check(handle.first == 0xf000u && handle.second == 0xf001u,
              "rotate temporary effect");
        calls.emplace_back("release temporary effect");
    }
    void SetEffectVisibility(std::uint32_t handle,
                             std::uint32_t value) override {
        Check(handle == object + 0x90u, "rotate visibility handle");
        visibility = value;
        calls.emplace_back("visibility");
    }
    float InterpolateFloat(float start, float end,
                           float fraction, std::uint32_t mode) override {
        Check(start == 0.0f && mode == 0u,
              "rotate interpolation arguments");
        calls.emplace_back("interpolate");
        return start + (end - start) * fraction;
    }
    void SetOwnerPosition(std::uint32_t,
                          const pal4::MoveTaskVector&,
                          std::uint32_t) override {
        throw std::runtime_error("rotate unexpectedly moved owner");
    }
    void SetOwnerRotation(std::uint32_t address,
                          const pal4::MoveTaskVector& value,
                          std::uint32_t mode) override {
        Check(address == owner && mode == 0u, "rotate owner setter");
        rotation = value;
        calls.emplace_back("set rotation");
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        return string_lengths.at(source);
    }
    [[noreturn]] void ThrowLength() override {
        throw std::length_error("rotate name too long");
    }
    std::uint32_t SetErrorFlag(std::uint32_t record,
                               std::uint8_t flag) override {
        calls.emplace_back("reset text");
        reset_flags.push_back(flag);
        for (const auto offset : {4u, 8u, 12u})
            words[record + offset] = 0u;
        return 0u;
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override {
        calls.emplace_back("reallocate text");
        reallocated.push_back(record);
        words[record + 4u] = (record & 0xffu) == 0x64u ?
            0xc000u : 0xc100u;
        words[record + 12u] = length;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(length == string_lengths.at(source),
              "rotate copied length");
        copied.push_back(destination);
        calls.emplace_back("copy text");
    }
    std::uint8_t AudioIsPlaying(std::uint32_t handle) override {
        Check(handle == object + 0x8cu, "rotate playing handle");
        calls.emplace_back("is playing");
        return playing;
    }
    std::uint8_t AudioStopSound(std::uint32_t) override {
        calls.emplace_back("stop sound");
        return 0x71u;
    }
    std::uint8_t AudioPauseSound(std::uint32_t) override {
        calls.emplace_back("pause sound");
        return 0x73u;
    }
};

struct SpawnRuntime final : Runtime, pal4::IRotateTaskSpawnRuntime {
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
        words[object + 0x9cu] = context;
        words[object + 0x98u] = owner;
        words[object + 0x30u] = 0u;
        words[object + 0x44u] = std::bit_cast<std::uint32_t>(10.0f);
        words[object + 0x68u] = 0x9000u;
        words[object + 0x74u] = 7u;
        words[object + 0x7cu] = 0x9100u;
        words[object + 0x88u] = 8u;
        for (const auto offset : {0u, 4u, 8u})
            words[object + 0x4cu + offset] =
                std::bit_cast<std::uint32_t>(1.0f + offset / 4u);
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
              "rotate reverse registration");
        calls.emplace_back("add reverse");
        registered = task;
    }
    void InsertOwnerTask(std::uint32_t address,
                         std::uint32_t task) override {
        Check(address == collection, "rotate owner collection");
        calls.emplace_back("insert reverse");
        inserted = task;
    }
    void NotifyPreviousTask(std::uint32_t previous) override {
        Check(previous == 0x6000u, "rotate previous task");
        calls.emplace_back("previous");
    }
    std::uint32_t ConstructHelperTask(std::uint32_t memory) override {
        Check(memory == helper, "rotate helper memory");
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
              "rotate helper timing");
        calls.emplace_back("add helper");
        added_helper = task;
    }
    void SetHelperOwner(std::uint32_t task,
                        std::uint32_t task_owner) override {
        Check((task == helper || task == 0u) && task_owner == owner,
              "rotate helper owner");
        calls.emplace_back("set helper owner");
    }
    void MarkProcessed(std::uint32_t task_context) override {
        Check(task_context == context, "rotate processed context");
        calls.emplace_back("processed");
        processed = task_context;
    }
};

void Seed(Runtime& runtime, float start_y = 10.0f,
          float target_y = 20.0f) {
    runtime.words[Runtime::object + 0x98u] = Runtime::owner;
    runtime.words[Runtime::object + 0x40u] = 0u;
    runtime.words[Runtime::object + 0x44u] =
        std::bit_cast<std::uint32_t>(10.0f);
    runtime.bytes[Runtime::object + 0x3cu] = 0u;
    runtime.words[Runtime::object + 0x6cu] = 0u;
    runtime.words[Runtime::object + 0x80u] = 0u;
    for (const auto offset : {0u, 4u, 8u})
        runtime.words[Runtime::owner + 0xa0u + offset] =
            std::bit_cast<std::uint32_t>(offset == 4u ?
                start_y : 1.0f + offset / 4u);
    runtime.words[Runtime::object + 0x5cu] =
        std::bit_cast<std::uint32_t>(target_y);
}

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::RotateTask::Construct(Runtime::object, runtime) ==
              Runtime::object && runtime.transient_count == 1u &&
              runtime.words.at(Runtime::object) == 0x848918u &&
              runtime.bytes.at(Runtime::object + 0x64u) == 0x5au &&
              runtime.bytes.at(Runtime::object + 0x78u) == 0x5au &&
              runtime.words.at(Runtime::object + 0x68u) == 0u &&
              runtime.words.at(Runtime::object + 0x7cu) == 0u &&
              runtime.words.at(Runtime::object + 0x98u) == 0u &&
              runtime.words.at(Runtime::object + 0x9cu) == 0u &&
              IndexOf(runtime.calls, "base construct") <
                  IndexOf(runtime.calls, "name node"),
          "rotate constructor layout and order");
    Runtime failed;
    failed.fail_name_node = true;
    bool thrown = false;
    try {
        (void)pal4::RotateTask::Construct(Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls[failed.calls.size() - 3u] ==
                        "release effect" &&
              failed.calls[failed.calls.size() - 2u] ==
                  "release sound" &&
              failed.calls.back() == "base destruct",
          "rotate constructor exceptional cleanup");
    for (const auto count : {std::uint8_t{0u},
                             std::uint8_t{2u},
                             std::uint8_t{0xffu}}) {
        Runtime destructing;
        destructing.words[Runtime::object + 0x7cu] = 0x3000u;
        destructing.words[Runtime::object + 0x68u] = 0x4000u;
        destructing.bytes[0x2fffu] = count;
        destructing.bytes[0x3fffu] = count;
        destructing.audio_active = true;
        destructing.effect_active = true;
        pal4::RotateTask::Destruct(Runtime::object, destructing);
        Check(destructing.words.at(Runtime::object) == 0x8486fcu &&
                  destructing.words.at(Runtime::object + 0x7cu) == 0u &&
                  destructing.words.at(Runtime::object + 0x68u) == 0u &&
                  (count == 2u ?
                       destructing.bytes.at(0x2fffu) == 1u &&
                           destructing.bytes.at(0x3fffu) == 1u :
                       destructing.freed.size() == 2u) &&
                  IndexOf(destructing.calls, "audio play") <
                      IndexOf(destructing.calls, "effect update") &&
                  destructing.calls.back() == "base destruct",
              "rotate destructor reference and resource handling");
    }
}

void CheckEnterAndUpdate() {
    Runtime missing;
    missing.words[Runtime::object + 0x98u] = 0u;
    Check(pal4::RotateTask::Enter(Runtime::object, missing) ==
              0u && missing.calls.empty(), "rotate entry owner gate");
    for (const auto sample : {
             std::pair{10.0f, 20.0f},
             std::pair{10.0f, 350.0f},
             std::pair{350.0f, 10.0f},
             std::pair{0.0f, 180.0f},
             std::pair{10.0f, 190.0f},
             std::pair{180.0f, 0.0f}}) {
        Runtime angle;
        Seed(angle, sample.first, sample.second);
        Check(pal4::RotateTask::Enter(Runtime::object, angle) == 1u &&
                  angle.words.at(Runtime::object + 0x4cu) ==
                      angle.words.at(Runtime::owner + 0xa0u) &&
                  std::bit_cast<float>(angle.words.at(
                      Runtime::object + 0x48u)) ==
                      (sample.first == 10.0f &&
                               sample.second == 350.0f ? -20.0f :
                       sample.first == 350.0f ? 20.0f :
                       sample.first == 180.0f ? -180.0f :
                       sample.second == 180.0f ||
                               sample.second == 190.0f ? 180.0f : 10.0f),
              "rotate entry captures shortest yaw");
    }
    Runtime active;
    Seed(active);
    active.words[Runtime::object + 0x6cu] = 3u;
    active.words[Runtime::object + 0x74u] =
        static_cast<std::uint32_t>(-3);
    active.words[Runtime::object + 0x80u] = 4u;
    active.words[Runtime::object + 0x7cu] = 0x9000u;
    active.words[Runtime::object + 0x88u] = 55u;
    active.words[Runtime::owner + 0x1a8u] = 1u;
    active.words[Runtime::owner + 0x14u] = 0x6000u;
    active.words[0x6004u] = 9u;
    active.audio_active = true;
    active.effect_active = true;
    Check(pal4::RotateTask::Enter(Runtime::object, active) ==
              1u && active.music_name == 0xa000u &&
              active.music_parameter == 0u &&
              active.words.at(Runtime::object + 0x74u) == 0u &&
              active.effect_name == 0x9000u &&
              active.effect_position == Runtime::owner + 0xacu &&
              active.effect_parameter == 55u && active.anchored &&
              active.visibility == 9u &&
              IndexOf(active.calls, "release path") <
                  IndexOf(active.calls, "create effect"),
          "rotate entry sound and effect resources");
    Runtime fallback;
    fallback.words = active.words;
    fallback.words[Runtime::object + 0x7cu] = 0u;
    fallback.words[Runtime::owner + 0x1a8u] = 0u;
    fallback.path = 0u;
    Check(pal4::RotateTask::Enter(Runtime::object, fallback) ==
              1u && fallback.music_name == 0xd000u &&
              fallback.effect_name == 0xd000u &&
              !fallback.anchored,
          "rotate entry empty sound and effect fallbacks");
    Runtime no_owner;
    no_owner.words[Runtime::object + 0x98u] = 0u;
    no_owner.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::RotateTask::Update(Runtime::object, 1.0f,
                                    no_owner) == 0u &&
              no_owner.bytes.at(Runtime::object + 0x3cu) == 0u,
          "rotate update owner gate");
    Runtime moving;
    Seed(moving);
    (void)pal4::RotateTask::Enter(Runtime::object, moving);
    Check(pal4::RotateTask::Update(Runtime::object, 5.0f,
                                    moving) == 1u &&
              moving.rotation.x == 1.0f &&
              moving.rotation.y == 15.0f &&
              moving.rotation.z == 3.0f &&
              moving.bytes.at(Runtime::object + 0x3cu) == 0u,
          "rotate update interpolates yaw");
    moving.audio_active = true;
    moving.calls.clear();
    Check(pal4::RotateTask::Update(Runtime::object, 6.0f,
                                    moving) == 1u &&
              moving.rotation.y == 20.0f &&
              moving.bytes.at(Runtime::object + 0x3cu) == 1u &&
              std::bit_cast<float>(moving.words.at(
                  Runtime::object + 0x40u)) == 10.0f &&
              IndexOf(moving.calls, "audio play") <
                  IndexOf(moving.calls, "set rotation"),
          "rotate update clamps and completes");
    moving.calls.clear();
    Check(pal4::RotateTask::Update(Runtime::object, 1.0f,
                                    moving) == 0u &&
              moving.calls.empty(), "rotate completed update gate");
}

void CheckInitializeAndHooks() {
    Runtime runtime;
    runtime.words[Runtime::object + 0x68u] = 0u;
    runtime.words[Runtime::object + 0x70u] = 0u;
    runtime.words[Runtime::object + 0x7cu] = 0u;
    runtime.words[Runtime::object + 0x84u] = 0u;
    runtime.string_lengths[0x9000u] = 3u;
    runtime.string_lengths[0x9100u] = 5u;
    const pal4::MoveTaskVector target{11.0f, 12.0f, 13.0f};
    Check(pal4::RotateTask::Initialize(
              Runtime::object, Runtime::owner,
              std::bit_cast<std::uint32_t>(10.0f), target,
              0x9000u, 7u, 0x9100u, 8u, 0x7000u,
              runtime) == 0x7000u &&
              runtime.words.at(Runtime::object + 0x98u) ==
                  Runtime::owner &&
              runtime.words.at(Runtime::object + 0x58u) ==
                  std::bit_cast<std::uint32_t>(11.0f) &&
              runtime.words.at(Runtime::object + 0x5cu) ==
                  std::bit_cast<std::uint32_t>(12.0f) &&
              runtime.words.at(Runtime::object + 0x74u) == 7u &&
              runtime.words.at(Runtime::object + 0x88u) == 8u &&
              runtime.words.at(Runtime::object + 0x9cu) == 0x7000u &&
              runtime.reallocated ==
                  (std::vector<std::uint32_t>{Runtime::object + 0x64u,
                                              Runtime::object + 0x78u}) &&
              runtime.copied ==
                  (std::vector<std::uint32_t>{0xc000u, 0xc100u}),
          "rotate initializer fields and text allocation");
    runtime.calls.clear();
    runtime.playing = 1u;
    Check(pal4::RotateTask::StopSoundIfPlaying(
              Runtime::object, runtime) == 0x71u &&
              runtime.calls ==
                  (std::vector<std::string>{"is playing", "stop sound"}),
          "rotate stop playing sound");
    runtime.calls.clear();
    runtime.playing = 0u;
    Check(pal4::RotateTask::StopSoundIfPlaying(
              Runtime::object, runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"is playing"}),
          "rotate skip stop inactive sound");
    runtime.calls.clear();
    Check(pal4::RotateTask::PauseSoundIfNotPlaying(
              Runtime::object, runtime) == 0x73u &&
              runtime.calls ==
                  (std::vector<std::string>{"is playing", "pause sound"}),
          "rotate pause inactive sound");
    runtime.calls.clear();
    runtime.playing = 1u;
    Check(pal4::RotateTask::PauseSoundIfNotPlaying(
              Runtime::object, runtime) == 1u &&
              runtime.calls ==
                  (std::vector<std::string>{"is playing"}),
          "rotate skip pause active sound");
    Runtime shared;
    shared.words[Runtime::object + 0x68u] = 0x3000u;
    shared.words[Runtime::object + 0x7cu] = 0x4000u;
    shared.bytes[0x2fffu] = 2u;
    shared.bytes[0x3fffu] = 2u;
    shared.string_lengths[0xd000u] = 0u;
    Check(pal4::RotateTask::Initialize(
              Runtime::object, Runtime::owner, 0u, target,
              0xd000u, 7u, 0xd000u, 8u, 0x7000u,
              shared) == 8u &&
              shared.bytes.at(0x2fffu) == 1u &&
              shared.bytes.at(0x3fffu) == 1u &&
              shared.reset_flags ==
                  (std::vector<std::uint8_t>{0u, 0u}),
          "rotate shared empty text and raw return");
    Runtime empty;
    empty.words[Runtime::object + 0x68u] = 0u;
    empty.words[Runtime::object + 0x7cu] = 0u;
    empty.string_lengths[0xd000u] = 0u;
    Check(pal4::RotateTask::Initialize(
              Runtime::object, Runtime::owner, 0u, target,
              0xd000u, 7u, 0xd000u, 8u, 0x7000u,
              empty) == 0x7000u &&
              empty.reset_flags ==
                  (std::vector<std::uint8_t>{1u, 1u}),
          "rotate ordinary empty text");
    Runtime reused;
    reused.words[Runtime::object + 0x68u] = 0x3000u;
    reused.words[Runtime::object + 0x70u] = 10u;
    reused.words[Runtime::object + 0x7cu] = 0x4000u;
    reused.words[Runtime::object + 0x84u] = 10u;
    reused.bytes[0x2fffu] = 0u;
    reused.bytes[0x3fffu] = 0xffu;
    reused.string_lengths[0x9000u] = 3u;
    reused.string_lengths[0x9100u] = 5u;
    (void)pal4::RotateTask::Initialize(
        Runtime::object, Runtime::owner, 0u, target,
        0x9000u, 7u, 0x9100u, 8u, 0x7000u, reused);
    Check(reused.reallocated.empty() &&
              reused.copied ==
                  (std::vector<std::uint32_t>{0x3000u, 0x4000u}),
          "rotate reuses exclusive buffers");
    Runtime too_long;
    too_long.string_lengths[0x9000u] = 0xfffffffeu;
    bool thrown = false;
    try {
        (void)pal4::RotateTask::Initialize(
            Runtime::object, Runtime::owner, 0u, target,
            0x9000u, 7u, 0x9100u, 8u, 0x7000u, too_long);
    } catch (const std::length_error&) {
        thrown = true;
    }
    Check(thrown, "rotate oversized name rejected");
}

void CheckFactoryAndFollowup() {
    Runtime descriptor;
    Check(pal4::RotateTaskFactory::ConstructDescriptor(
              0x2000u, descriptor) == 0x2000u &&
              descriptor.words.at(0x2000u) == 0x84893cu,
          "rotate factory descriptor");
    SpawnRuntime factory;
    Check(pal4::RotateTaskFactory::Create(factory) ==
              SpawnRuntime::child &&
              factory.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xa0u}) &&
              factory.words.at(SpawnRuntime::child) == 0x848918u,
          "rotate factory creates task");
    SpawnRuntime null_factory;
    null_factory.allocations = {0u};
    Check(pal4::RotateTaskFactory::Create(null_factory) == 0u &&
              null_factory.calls ==
                  (std::vector<std::string>{"allocate"}),
          "rotate factory null allocation");
    SpawnRuntime failed_factory;
    failed_factory.fail_name_node = true;
    bool thrown = false;
    try {
        (void)pal4::RotateTaskFactory::Create(failed_factory);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed_factory.freed ==
                        (std::vector<std::uint32_t>{SpawnRuntime::child}) &&
              failed_factory.calls.back() == "free",
          "rotate factory constructor failure cleanup");
    SpawnRuntime no_context;
    no_context.words[Runtime::object + 0x9cu] = 0u;
    Check(pal4::RotateTask::SpawnReverseAndFollowup(
              Runtime::object, no_context) == 0u &&
              no_context.calls.empty(), "rotate context gate");
    SpawnRuntime no_owner;
    no_owner.words[Runtime::object + 0x98u] = 0u;
    Check(pal4::RotateTask::SpawnReverseAndFollowup(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(), "rotate owner gate");
    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0x6000u;
    Check(pal4::RotateTask::SpawnReverseAndFollowup(
              Runtime::object, previous) == 1u &&
              previous.registered == SpawnRuntime::child &&
              previous.inserted == SpawnRuntime::child &&
              previous.words.at(SpawnRuntime::child + 0x58u) ==
                  previous.words.at(Runtime::object + 0x4cu) &&
              previous.words.at(SpawnRuntime::child + 0x5cu) ==
                  previous.words.at(Runtime::object + 0x50u) &&
              previous.words.at(SpawnRuntime::child + 0x9cu) == 0u &&
              IndexOf(previous.calls, "add reverse") <
                  IndexOf(previous.calls, "copy text") &&
              IndexOf(previous.calls, "insert reverse") <
                  IndexOf(previous.calls, "previous"),
          "rotate reverse child before previous task");
    SpawnRuntime followup;
    Check(pal4::RotateTask::SpawnReverseAndFollowup(
              Runtime::object, followup) == 1u &&
              followup.added_helper == SpawnRuntime::helper &&
              followup.words.at(0xa004u) == SpawnRuntime::helper &&
              followup.processed == SpawnRuntime::context &&
              followup.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xa0u, 0x40u}),
          "rotate appends helper");
    SpawnRuntime growth;
    growth.words[SpawnRuntime::collection + 12u] = 0xa004u;
    growth.allocations.push_back(0xf000u);
    Check(pal4::RotateTask::SpawnReverseAndFollowup(
              Runtime::object, growth) == 1u &&
              growth.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xa0u, 0x40u, 8u}) &&
              growth.words.at(0xf000u) == 0xaaaa0000u &&
              growth.words.at(0xf004u) == SpawnRuntime::helper,
          "rotate grows helper collection");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckEnterAndUpdate();
        CheckInitializeAndHooks();
        CheckFactoryAndFollowup();
        std::cout << "rotate_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
