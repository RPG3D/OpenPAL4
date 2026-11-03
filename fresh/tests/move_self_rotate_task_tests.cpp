#include "pal4/move_self_rotate_task.h"

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

struct Runtime : virtual pal4::IMoveSelfRotateRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> lengths;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> reallocations;
    std::vector<std::uint32_t> copies;
    std::vector<std::uint8_t> flags;
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
    std::uint32_t properties = 0u;
    pal4::MoveTaskVector position{};
    pal4::MoveTaskVector rotation_delta{};
    pal4::MoveTaskVector rotation{};
    pal4::MoveTaskVector forward{0.0f, 1.0f, 0.0f};
    std::uint8_t playing = 0u;
    unsigned transient_calls = 0u;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object || address == 0xd000u,
              "self rotate base construct address");
        calls.emplace_back("base construct");
    }
    void BaseDestruct(std::uint32_t address) override {
        Check(address == object || address == 0xd000u,
              "self rotate base destruct address");
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
        Check(address == object + 0xacu || address == 0xd0acu,
              "self rotate name node address");
        calls.emplace_back("name node");
        if (fail_name_node) throw std::runtime_error("name node failure");
    }
    void ReleaseText(std::uint32_t record) override {
        calls.push_back(record % 0x100u == 0x94u ?
                        "release effect" : "release sound");
    }
    bool AudioIsActive(std::uint32_t handle) override {
        Check((handle & 0xffu) == 0xa8u,
              "self rotate audio handle");
        calls.emplace_back("audio check");
        return audio_active;
    }
    void AudioPlaySound(std::uint32_t handle) override {
        Check((handle & 0xffu) == 0xa8u,
              "self rotate played handle");
        calls.emplace_back("audio play");
    }
    bool EffectNeedsUpdate(std::uint32_t handle) override {
        Check((handle & 0xffu) == 0xacu,
              "self rotate effect handle");
        calls.emplace_back("effect check");
        return effect_needs_update;
    }
    void EffectUpdate(std::uint32_t handle) override {
        Check((handle & 0xffu) == 0xacu,
              "self rotate effect update handle");
        calls.emplace_back("effect update");
    }
    void EffectReset(std::uint32_t handle) override {
        Check((handle & 0xffu) == 0xacu,
              "self rotate effect reset handle");
        calls.emplace_back("effect reset");
    }
    std::uint32_t BuildSoundPath(std::uint32_t record,
                                 std::string_view suffix) override {
        Check(record == object + 0x80u && suffix == ".wav",
              "self rotate sound path");
        calls.emplace_back("build path");
        return path;
    }
    void ReleaseSoundPath(std::uint32_t address) override {
        Check(address == path, "self rotate released path");
        calls.emplace_back("release path");
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty string");
        return 0xe000u;
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
              "self rotate temporary effect");
        calls.emplace_back("release temporary effect");
    }
    void SetEffectVisibility(std::uint32_t,
                             std::uint32_t) override {
        throw std::runtime_error("wrong effect method");
    }
    float InterpolateFloat(float start, float end, float fraction,
                           std::uint32_t mode) override {
        Check(mode == 0u, "self rotate interpolation mode");
        calls.emplace_back("interpolate");
        return start + (end - start) * fraction;
    }
    void SetOwnerPosition(std::uint32_t owner,
                          const pal4::MoveTaskVector& value,
                          std::uint32_t mode) override {
        Check(owner == 0x5000u && mode == 0u,
              "self rotate owner position");
        calls.emplace_back("position");
        position = value;
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        calls.emplace_back("length");
        return lengths.at(source);
    }
    [[noreturn]] void ThrowLength() override {
        calls.emplace_back("length error");
        throw std::length_error("self rotate name too long");
    }
    std::uint32_t SetErrorFlag(std::uint32_t record,
                               std::uint8_t flag) override {
        calls.emplace_back("reset text");
        flags.push_back(flag);
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
        return 0x76543210u;
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override {
        calls.emplace_back("reallocate text");
        reallocations.push_back(record);
        words[record + 4u] = (record & 0xffu) == 0x80u ?
            0xc100u : 0xc200u;
        words[record + 12u] = length;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(length == lengths.at(source),
              "self rotate copy length");
        calls.emplace_back("copy text");
        copies.push_back(destination);
    }
    std::uint8_t AudioIsPlaying(std::uint32_t handle) override {
        Check((handle & 0xffu) == 0xa8u,
              "self rotate playing handle");
        calls.emplace_back("is playing");
        return playing;
    }
    std::uint8_t AudioStopSound(std::uint32_t handle) override {
        Check((handle & 0xffu) == 0xa8u,
              "self rotate stop handle");
        calls.emplace_back("stop sound");
        return 0x72u;
    }
    std::uint8_t AudioPauseSound(std::uint32_t handle) override {
        Check((handle & 0xffu) == 0xa8u,
              "self rotate pause handle");
        calls.emplace_back("pause sound");
        return 0x73u;
    }
    pal4::MoveTaskVector FrameForwardVector(
        std::uint32_t owner) override {
        Check(owner == 0x5000u, "self rotate frame owner");
        calls.emplace_back("frame forward");
        return forward;
    }
    void SetEffectProperties(std::uint32_t handle,
                             std::uint32_t value) override {
        Check((handle & 0xffu) == 0xacu,
              "self rotate effect properties handle");
        calls.emplace_back("effect properties");
        properties = value;
    }
    pal4::MoveTaskVector UpdateFrameRotation(
        std::uint32_t owner,
        const pal4::MoveTaskVector& delta) override {
        Check(owner == 0x5000u, "self rotate frame owner");
        calls.emplace_back("frame rotation");
        rotation_delta = delta;
        return {delta.x + 1.0f, delta.y + 1.0f,
                delta.z + 1.0f};
    }
    void RotateOwner(std::uint32_t owner,
                     const pal4::MoveTaskVector& value,
                     std::uint32_t mode) override {
        Check(owner == 0x5000u && mode == 0u,
              "self rotate setter");
        calls.emplace_back("rotate owner");
        rotation = value;
    }
};

struct SpawnRuntime final : Runtime,
                            pal4::IMoveSelfRotateSpawnRuntime {
    static constexpr std::uint32_t owner = 0x8000u;
    static constexpr std::uint32_t collection = owner + 0x2a0u;
    std::deque<std::uint32_t> allocations{0xd000u, 0xb000u};
    std::vector<std::uint32_t> allocation_sizes;
    std::uint32_t registered = 0u;
    std::uint32_t inserted = 0u;
    std::uint32_t added_helper = 0u;
    std::uint32_t processed = 0u;

    SpawnRuntime() {
        words[object + 0xb8u] = 0x7000u;
        words[object + 0xb4u] = owner;
        words[object + 0x30u] = 0u;
        words[object + 0x44u] =
            std::bit_cast<std::uint32_t>(10.0f);
        for (const auto offset : {0u, 4u, 8u}) {
            words[object + 0x48u + offset] =
                std::bit_cast<std::uint32_t>(1.0f + offset / 4u);
            words[object + 0x60u + offset] =
                std::bit_cast<std::uint32_t>(2.0f + offset / 4u);
        }
        words[object + 0x6cu] = 1u;
        words[object + 0x70u] =
            std::bit_cast<std::uint32_t>(5.0f);
        words[object + 0x84u] = 0x9000u;
        words[object + 0x90u] = 7u;
        words[object + 0x98u] = 0x9100u;
        words[object + 0xa4u] = 8u;
        lengths[0x9000u] = 3u;
        lengths[0x9100u] = 5u;
        lengths[0xe000u] = 0u;
        words[collection + 4u] = 0xa000u;
        words[collection + 8u] = 0xa004u;
        words[collection + 12u] = 0xa008u;
        words[0xa000u] = 0xaaaa0000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocation_sizes.push_back(bytes);
        const auto result = allocations.front();
        allocations.pop_front();
        return result;
    }
    void AddReverseTask(std::uint32_t context,
                        std::uint32_t task,
                        std::uint32_t timing) override {
        Check(context == 0x7000u && task == 0xd000u &&
                  timing == object + 0x0cu,
              "self rotate reverse task timing");
        calls.emplace_back("add reverse");
        registered = task;
    }
    void InsertOwnerTask(std::uint32_t address,
                         std::uint32_t task) override {
        Check(address == collection,
              "self rotate owner collection");
        calls.emplace_back("insert reverse");
        inserted = task;
    }
    void NotifyPreviousTask(std::uint32_t previous) override {
        Check(previous == 0x6000u,
              "self rotate previous task");
        calls.emplace_back("previous");
    }
    std::uint32_t ConstructHelperTask(
        std::uint32_t memory) override {
        Check(memory == 0xb000u,
              "self rotate helper memory");
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
              "self rotate helper timing");
        calls.emplace_back("add helper");
        added_helper = helper;
    }
    void SetHelperOwner(std::uint32_t helper,
                        std::uint32_t task_owner) override {
        Check((helper == 0xb000u || helper == 0u) &&
                  task_owner == owner,
              "self rotate helper owner");
        calls.emplace_back("set helper owner");
    }
    void MarkProcessed(std::uint32_t context) override {
        Check(context == 0x7000u,
              "self rotate processed context");
        calls.emplace_back("processed");
        processed = context;
    }
};

void SeedMotion(Runtime& runtime) {
    runtime.words[Runtime::object + 0xb4u] = 0x5000u;
    runtime.words[Runtime::object + 0x40u] = 0u;
    runtime.words[Runtime::object + 0x44u] =
        std::bit_cast<std::uint32_t>(10.0f);
    runtime.bytes[Runtime::object + 0x3cu] = 0u;
    runtime.words[Runtime::object + 0x6cu] = 0u;
    for (const auto offset : {0u, 4u, 8u}) {
        runtime.words[Runtime::object + 0x48u + offset] =
            std::bit_cast<std::uint32_t>(1.0f + offset / 4u);
        runtime.words[Runtime::object + 0x54u + offset] =
            std::bit_cast<std::uint32_t>(11.0f + offset / 4u);
        runtime.words[Runtime::object + 0x60u + offset] = 0u;
        runtime.words[Runtime::object + 0x74u + offset] =
            std::bit_cast<std::uint32_t>(offset == 4u ?
                                             1.0f : 0.0f);
        runtime.words[0x5000u + 0xacu + offset] =
            std::bit_cast<std::uint32_t>(1.0f + offset / 4u);
    }
}

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::MoveSelfRotateTask::Construct(
              Runtime::object, runtime) == Runtime::object &&
              runtime.transient_calls == 1u &&
              runtime.bytes.at(Runtime::object + 0x80u) == 0x5au &&
              runtime.bytes.at(Runtime::object + 0x94u) == 0x5au &&
              runtime.words.at(Runtime::object) == 0x848880u &&
              runtime.words.at(Runtime::object + 0x84u) == 0u &&
              runtime.words.at(Runtime::object + 0x98u) == 0u &&
              runtime.words.at(Runtime::object + 0xa8u) == 0u &&
              runtime.words.at(Runtime::object + 0xb4u) == 0u &&
              runtime.words.at(Runtime::object + 0xb8u) == 0u &&
              IndexOf(runtime.calls, "base construct") <
                  IndexOf(runtime.calls, "name node"),
          "self rotate constructor initializes text and layout");
    Runtime failed;
    failed.fail_name_node = true;
    bool thrown = false;
    try {
        (void)pal4::MoveSelfRotateTask::Construct(
            Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls[failed.calls.size() - 3u] ==
                        "release effect" &&
              failed.calls[failed.calls.size() - 2u] ==
                  "release sound" &&
              failed.calls.back() == "base destruct",
          "self rotate constructor exception cleanup");
    for (const auto count : {std::uint8_t{0u},
                             std::uint8_t{2u},
                             std::uint8_t{0xffu}}) {
        Runtime destructing;
        destructing.words[Runtime::object + 0x98u] = 0x3000u;
        destructing.words[Runtime::object + 0x84u] = 0x4000u;
        destructing.bytes[0x2fffu] = count;
        destructing.bytes[0x3fffu] = count;
        destructing.audio_active = true;
        destructing.effect_needs_update = true;
        pal4::MoveSelfRotateTask::Destruct(Runtime::object,
                                           destructing);
        Check(destructing.words.at(Runtime::object) == 0x8486fcu &&
                  destructing.words.at(Runtime::object + 0x98u) == 0u &&
                  destructing.words.at(Runtime::object + 0x9cu) == 0u &&
                  destructing.words.at(Runtime::object + 0x84u) == 0u &&
                  destructing.words.at(Runtime::object + 0x88u) == 0u &&
                  (count == 2u ?
                       destructing.bytes.at(0x2fffu) == 1u &&
                           destructing.bytes.at(0x3fffu) == 1u :
                       destructing.freed.size() == 2u) &&
                  IndexOf(destructing.calls, "audio play") <
                      IndexOf(destructing.calls, "effect update") &&
                  IndexOf(destructing.calls, "effect update") <
                      IndexOf(destructing.calls, "effect reset") &&
                  destructing.calls.back() == "base destruct",
              "self rotate destructor releases handles and strings");
    }
}

void CheckEnterAndUpdate() {
    Runtime no_owner;
    no_owner.words[Runtime::object + 0xb4u] = 0u;
    Check(pal4::MoveSelfRotateTask::Enter(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(),
          "self rotate entry owner gate");
    Runtime entered;
    SeedMotion(entered);
    entered.words[Runtime::object + 0x6cu] = 1u;
    entered.words[Runtime::object + 0x88u] = 3u;
    entered.words[Runtime::object + 0x90u] =
        static_cast<std::uint32_t>(-4);
    entered.words[Runtime::object + 0x9cu] = 5u;
    entered.words[Runtime::object + 0x98u] = 0x9000u;
    entered.words[Runtime::object + 0xa4u] = 6u;
    entered.words[0x5000u + 0x1a8u] = 1u;
    entered.words[0x5000u + 0x14u] = 0x6000u;
    entered.words[0x6004u] = 9u;
    entered.audio_active = true;
    entered.effect_needs_update = true;
    Check(pal4::MoveSelfRotateTask::Enter(
              Runtime::object, entered) == 1u &&
              entered.words.at(Runtime::object + 0x48u) ==
                  entered.words.at(0x50acu) &&
              entered.words.at(Runtime::object + 0x78u) ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              entered.played_path == 0xa000u &&
              entered.played_parameter == 0u &&
              entered.words.at(Runtime::object + 0x90u) == 0u &&
              entered.effect_name == 0x9000u &&
              entered.effect_position == 0x84b1dcu &&
              entered.effect_parameter == 6u &&
              entered.anchored && entered.properties == 9u &&
              IndexOf(entered.calls, "frame forward") <
                  IndexOf(entered.calls, "build path") &&
              IndexOf(entered.calls, "release path") <
                  IndexOf(entered.calls, "create effect"),
          "self rotate entry captures frame and starts resources");
    Runtime fallback;
    fallback.words = entered.words;
    fallback.words[Runtime::object + 0x98u] = 0u;
    fallback.words[0x5000u + 0x1a8u] = 0u;
    fallback.path = 0u;
    Check(pal4::MoveSelfRotateTask::Enter(
              Runtime::object, fallback) == 1u &&
              fallback.played_path == 0xe000u &&
              fallback.effect_name == 0xe000u &&
              !fallback.anchored,
          "self rotate entry empty resource names");

    Runtime missing;
    missing.words[Runtime::object + 0xb4u] = 0u;
    missing.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::MoveSelfRotateTask::Update(
              Runtime::object, 1.0f, missing) == 0u &&
              missing.bytes.at(Runtime::object + 0x3cu) == 0u,
          "self rotate update owner gate clears completion");
    Runtime normal;
    SeedMotion(normal);
    Check(pal4::MoveSelfRotateTask::Update(
              Runtime::object, 5.0f, normal) == 1u &&
              normal.position.x == 6.0f &&
              normal.position.y == 7.0f &&
              normal.position.z == 8.0f &&
              normal.bytes.at(Runtime::object + 0x3cu) == 0u &&
              std::find(normal.calls.begin(), normal.calls.end(),
                        "frame rotation") == normal.calls.end(),
          "self rotate update normal target mode with zero speed");
    Runtime direct;
    SeedMotion(direct);
    direct.words[Runtime::object + 0x6cu] = 1u;
    direct.words[Runtime::object + 0x70u] =
        std::bit_cast<std::uint32_t>(20.0f);
    direct.words[Runtime::object + 0x64u] =
        std::bit_cast<std::uint32_t>(2.0f);
    direct.audio_active = true;
    direct.effect_needs_update = true;
    Check(pal4::MoveSelfRotateTask::Update(
              Runtime::object, 5.0f, direct) == 1u &&
              direct.position.x == 1.0f &&
              direct.position.y == 12.0f &&
              direct.position.z == 3.0f &&
              direct.rotation_delta.x == 0.0f &&
              direct.rotation_delta.y == 10.0f &&
              direct.rotation_delta.z == 0.0f &&
              direct.rotation.y == 11.0f &&
              IndexOf(direct.calls, "position") <
                  IndexOf(direct.calls, "frame rotation") &&
              direct.bytes.at(Runtime::object + 0x3cu) == 0u,
          "self rotate direct distance and per-frame rotation");
    direct.calls.clear();
    Check(pal4::MoveSelfRotateTask::Update(
              Runtime::object, 6.0f, direct) == 1u &&
              direct.position.y == 22.0f &&
              direct.bytes.at(Runtime::object + 0x3cu) == 1u &&
              IndexOf(direct.calls, "audio play") <
                  IndexOf(direct.calls, "effect update") &&
              IndexOf(direct.calls, "effect update") <
                  IndexOf(direct.calls, "position"),
          "self rotate completion updates sound and effect first");
    direct.calls.clear();
    Check(pal4::MoveSelfRotateTask::Update(
              Runtime::object, 1.0f, direct) == 0u &&
              direct.calls.empty(),
          "self rotate completed task stays idle");
}

void CheckInitializeAndSpawn() {
    Runtime init;
    init.words[Runtime::object + 0x84u] = 0u;
    init.words[Runtime::object + 0x98u] = 0u;
    init.lengths[0x9000u] = 0u;
    init.lengths[0x9100u] = 0u;
    const pal4::MoveTaskVector target{1.0f, 2.0f, 3.0f};
    const pal4::MoveTaskVector speed{4.0f, 5.0f, 6.0f};
    Check(pal4::MoveSelfRotateTask::Initialize(
              Runtime::object, 0x5000u,
              std::bit_cast<std::uint32_t>(10.0f),
              target, speed, 1u,
              std::bit_cast<std::uint32_t>(20.0f),
              0x9000u, 7u, 0x9100u, 8u, 0x7000u,
              init) == 0x7000u &&
              init.words.at(Runtime::object + 0xb4u) == 0x5000u &&
              init.words.at(Runtime::object + 0x54u) ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              init.words.at(Runtime::object + 0x64u) ==
                  std::bit_cast<std::uint32_t>(5.0f) &&
              init.words.at(Runtime::object + 0x6cu) == 1u &&
              init.words.at(Runtime::object + 0x70u) ==
                  std::bit_cast<std::uint32_t>(20.0f) &&
              init.words.at(Runtime::object + 0x90u) == 7u &&
              init.words.at(Runtime::object + 0xa4u) == 8u &&
              init.words.at(Runtime::object + 0xb8u) == 0x7000u &&
              init.flags ==
                  (std::vector<std::uint8_t>{1u, 1u}),
          "self rotate initializer fields and empty strings");
    Runtime shared;
    shared.words[Runtime::object + 0x84u] = 0x3000u;
    shared.words[Runtime::object + 0x98u] = 0x4000u;
    shared.bytes[0x2fffu] = 2u;
    shared.bytes[0x3fffu] = 2u;
    shared.lengths = init.lengths;
    Check(pal4::MoveSelfRotateTask::Initialize(
              Runtime::object, 0x5000u, 0x41200000u,
              target, speed, 0u, 0u, 0x9000u, 7u,
              0x9100u, 8u, 0x7000u, shared) == 8u &&
              shared.flags ==
                  (std::vector<std::uint8_t>{0u, 0u}) &&
              shared.bytes.at(0x2fffu) == 1u &&
              shared.bytes.at(0x3fffu) == 1u,
          "self rotate initializer shared-empty raw return");
    Runtime reused;
    reused.words[Runtime::object + 0x84u] = 0x3000u;
    reused.words[Runtime::object + 0x8cu] = 10u;
    reused.words[Runtime::object + 0x98u] = 0x4000u;
    reused.words[Runtime::object + 0xa0u] = 10u;
    reused.bytes[0x2fffu] = 0u;
    reused.bytes[0x3fffu] = 0xffu;
    reused.lengths[0x9000u] = 3u;
    reused.lengths[0x9100u] = 5u;
    Check(pal4::MoveSelfRotateTask::Initialize(
              Runtime::object, 0x5000u, 0x41200000u,
              target, speed, 0u, 0u, 0x9000u, 7u,
              0x9100u, 8u, 0x7000u, reused) == 0x7000u &&
              reused.copies ==
                  (std::vector<std::uint32_t>{0x3000u, 0x4000u}) &&
              reused.reallocations.empty(),
          "self rotate initializer reuses exclusive buffers");
    Runtime oversized;
    oversized.words[Runtime::object + 0x84u] = 0u;
    oversized.lengths[0x9000u] = 0xfffffffeu;
    bool length_threw = false;
    try {
        (void)pal4::MoveSelfRotateTask::Initialize(
            Runtime::object, 0x5000u, 0x41200000u,
            target, speed, 0u, 0u, 0x9000u, 7u,
            0x9100u, 8u, 0x7000u, oversized);
    } catch (const std::length_error&) {
        length_threw = true;
    }
    Check(length_threw && oversized.calls.back() ==
                              "length error",
          "self rotate initializer rejects oversized text");

    Runtime hooks;
    Check(pal4::MoveSelfRotateTask::StopSoundIfPlaying(
              Runtime::object, hooks) == 0u &&
              hooks.calls.size() == 1u,
          "self rotate stop hook inactive");
    hooks.playing = 1u;
    Check(pal4::MoveSelfRotateTask::StopSoundIfPlaying(
              Runtime::object, hooks) == 0x72u &&
              pal4::MoveSelfRotateTask::PauseSoundIfNotPlaying(
                  Runtime::object, hooks) == 1u,
          "self rotate sound hooks active");
    hooks.playing = 0u;
    Check(pal4::MoveSelfRotateTask::PauseSoundIfNotPlaying(
              Runtime::object, hooks) == 0x73u,
          "self rotate pause hook inactive");

    Runtime descriptor;
    Check(pal4::MoveSelfRotateTaskFactory::ConstructDescriptor(
              0x2000u, descriptor) == 0x2000u &&
              descriptor.words.at(0x2000u) == 0x8488a4u,
          "self rotate factory descriptor");
    SpawnRuntime factory;
    Check(pal4::MoveSelfRotateTaskFactory::Create(factory) ==
              0xd000u &&
              factory.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xbcu}) &&
              factory.words.at(0xd000u) == 0x848880u,
          "self rotate factory allocates and constructs");
    SpawnRuntime null_factory;
    null_factory.allocations = {0u};
    Check(pal4::MoveSelfRotateTaskFactory::Create(
              null_factory) == 0u &&
              null_factory.calls ==
                  (std::vector<std::string>{"allocate"}),
          "self rotate factory null allocation");
    SpawnRuntime failed;
    failed.fail_name_node = true;
    bool factory_threw = false;
    try {
        (void)pal4::MoveSelfRotateTaskFactory::Create(failed);
    } catch (const std::runtime_error&) {
        factory_threw = true;
    }
    Check(factory_threw && failed.freed ==
                               (std::vector<std::uint32_t>{0xd000u}) &&
              failed.calls.back() == "free",
          "self rotate factory exception cleanup");
    SpawnRuntime no_context;
    no_context.words[Runtime::object + 0xb8u] = 0u;
    Check(pal4::MoveSelfRotateTask::SpawnReverseAndFollowup(
              Runtime::object, no_context) == 0u &&
              no_context.calls.empty(),
          "self rotate reverse context gate");
    SpawnRuntime no_owner;
    no_owner.words[Runtime::object + 0xb4u] = 0u;
    Check(pal4::MoveSelfRotateTask::SpawnReverseAndFollowup(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(),
          "self rotate reverse owner gate");
    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0x6000u;
    Check(pal4::MoveSelfRotateTask::SpawnReverseAndFollowup(
              Runtime::object, previous) == 1u &&
              previous.registered == 0xd000u &&
              previous.inserted == 0xd000u &&
              previous.words.at(0xd0b4u) == SpawnRuntime::owner &&
              previous.words.at(0xd060u) ==
                  std::bit_cast<std::uint32_t>(-2.0f) &&
              previous.words.at(0xd064u) ==
                  std::bit_cast<std::uint32_t>(-3.0f) &&
              previous.words.at(0xd068u) ==
                  std::bit_cast<std::uint32_t>(-4.0f) &&
              previous.words.at(0xd054u) ==
                  previous.words.at(Runtime::object + 0x48u) &&
              previous.words.at(0xd0b8u) == 0u &&
              previous.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xbcu}) &&
              IndexOf(previous.calls, "add reverse") <
                  IndexOf(previous.calls, "copy text") &&
              IndexOf(previous.calls, "insert reverse") <
                  IndexOf(previous.calls, "previous"),
          "self rotate reverse negates speed before previous callback");
    SpawnRuntime helper;
    Check(pal4::MoveSelfRotateTask::SpawnReverseAndFollowup(
              Runtime::object, helper) == 1u &&
              helper.added_helper == 0xb000u &&
              helper.words.at(0xa004u) == 0xb000u &&
              helper.processed == 0x7000u &&
              helper.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xbcu, 0x40u}),
          "self rotate reverse creates helper");
    SpawnRuntime growth;
    growth.words[SpawnRuntime::collection + 12u] = 0xa004u;
    growth.allocations.push_back(0xe800u);
    Check(pal4::MoveSelfRotateTask::SpawnReverseAndFollowup(
              Runtime::object, growth) == 1u &&
              growth.allocation_sizes ==
                  (std::vector<std::uint32_t>{0xbcu, 0x40u, 8u}) &&
              growth.words.at(0xe800u) == 0xaaaa0000u &&
              growth.words.at(0xe804u) == 0xb000u,
          "self rotate helper collection growth");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckEnterAndUpdate();
        CheckInitializeAndSpawn();
        std::cout << "move_self_rotate_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
