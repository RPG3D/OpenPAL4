#include "pal4/arena_skill_manager.h"

#include <bit>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Manager = pal4::ArenaSkillManager;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IArenaSkillRuntime {
    struct Invocation {
        pal4::ArenaActorCall operation;
        std::vector<std::uint32_t> arguments;
        bool operator==(const Invocation&) const = default;
    };
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Invocation> calls;
    struct SkillInvocation {
        pal4::ArenaSkillCall operation;
        std::vector<std::uint32_t> arguments;
        bool operator==(const SkillInvocation&) const = default;
    };
    std::vector<SkillInvocation> skill_calls;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> allocation_results;
    std::vector<std::uint32_t> frees;
    std::vector<std::string> sounds;
    std::string registered_name;
    std::vector<std::uint32_t> registration;
    std::uint32_t next_allocation = 0x12006000u;
    bool needs_content_update = true;
    bool player_exists = true;
    bool object_visible = true;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Call(
        pal4::ArenaActorCall operation,
        std::initializer_list<std::uint32_t> arguments) override {
        calls.push_back({operation, arguments});
        return 0u;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        allocations.push_back(size);
        if (!allocation_results.empty()) {
            const auto value = allocation_results.front();
            allocation_results.erase(allocation_results.begin());
            return value;
        }
        return next_allocation;
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        frees.push_back(pointer);
        return pointer + 1u;
    }
    std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t deleter) override {
        registered_name = name;
        registration = {id, factory, deleter};
        return 0x8500u;
    }
    std::uint32_t SkillCall(
        pal4::ArenaSkillCall operation,
        std::initializer_list<std::uint32_t> arguments) override {
        skill_calls.push_back({operation, arguments});
        if (operation == pal4::ArenaSkillCall::NeedsContentUpdate)
            return needs_content_update ? 1u : 0u;
        if (operation == pal4::ArenaSkillCall::GetGameObjectManager)
            return 0xa000u;
        if (operation == pal4::ArenaSkillCall::InterpolateAlpha)
            return std::bit_cast<std::uint32_t>(0.25f);
        if (operation == pal4::ArenaSkillCall::SetMaterialFlags)
            return 0xbeefu;
        if (operation == pal4::ArenaSkillCall::GetGameArenaManager)
            return 0xa100u;
        if (operation == pal4::ArenaSkillCall::GetPlayerManager)
            return 0xb000u;
        if (operation == pal4::ArenaSkillCall::GetPlayerByIndex)
            return player_exists ? 0x8000u : 0u;
        if (operation == pal4::ArenaSkillCall::CreateEnthrallEffect)
            return 0xc000u;
        if (operation == pal4::ArenaSkillCall::CreateSignpostEffect)
            return 0xc000u;
        if (operation == pal4::ArenaSkillCall::CreateSignpostResumeEffect)
            return 0xc000u;
        if (operation == pal4::ArenaSkillCall::IsObjectVisible)
            return object_visible ? 1u : 0u;
        if (operation == pal4::ArenaSkillCall::GetMinimapManager)
            return 0xa200u;
        if (operation == pal4::ArenaSkillCall::ConvertGridCoordinates)
            return 0xd000u;
        return 0u;
    }
    void PlayActionSound(std::string_view filename) override {
        sounds.emplace_back(filename);
    }
};

void RegistrationAndFactory() {
    Runtime runtime;
    std::uint8_t guard = 0x80u;
    std::uint32_t cached = 0u;
    Check(Manager::Register(guard, cached, runtime) == 0x8500u &&
              guard == 0x81u && cached == 0x8500u &&
              runtime.registered_name == "palGameArenaSkillMgr" &&
              runtime.registration ==
                  (std::vector<std::uint32_t>{1052u, 0x520f70u,
                                              0x520fd0u}),
          "singleton registration uses IDA name, ID and callbacks");
    runtime.registration.clear();
    Check(Manager::Register(guard, cached, runtime) == 0x8500u &&
              runtime.registration.empty(),
          "singleton guard reuses cached registration");

    const auto manager = Manager::Create(runtime);
    Check(manager == 0x12006000u &&
              runtime.allocations == (std::vector<std::uint32_t>{56u}) &&
              runtime.bytes[manager] == 0x12u &&
              runtime.bytes[manager + 16u] == 0x12u &&
              runtime.bytes[manager + 32u] == 0x12u &&
              runtime.bytes.size() == 3u,
          "factory allocates 56 bytes and writes only three flag bytes");
    for (const auto offset : {4u, 8u, 12u, 20u, 24u, 28u,
                              36u, 40u, 44u, 48u, 52u})
        Check(runtime.words[manager + offset] == 0u,
              "all array pointers and trailing words start at zero");
    runtime.next_allocation = 0u;
    Check(Manager::Create(runtime) == 0u,
          "failed singleton allocation returns null");
}

void CleanupAndDestroy() {
    Runtime runtime;
    constexpr std::uint32_t manager = 0x6000u;
    runtime.words[manager + 4u] = 0x7000u;
    runtime.words[manager + 20u] = 0x8000u;
    runtime.words[manager + 36u] = 0x9000u;
    Check(Manager::Cleanup(manager, runtime) == 0x7001u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x9000u, 0x8000u, 0x7000u}),
          "cleanup releases arrays in reverse field order");
    for (const auto offset : {4u, 8u, 12u, 20u, 24u, 28u,
                              36u, 40u, 44u})
        Check(runtime.words[manager + offset] == 0u,
              "cleanup clears each pointer triple");
    runtime.frees.clear();
    runtime.words[manager + 4u] = 0x7100u;
    runtime.words[manager + 20u] = 0x8100u;
    runtime.words[manager + 36u] = 0x9100u;
    Check(Manager::Destroy(manager, runtime) == manager + 1u &&
              runtime.frees == (std::vector<std::uint32_t>{
                  0x9100u, 0x8100u, 0x7100u, manager}),
          "deletion callback cleans arrays before freeing manager");
    runtime.frees.clear();
    Check(Manager::Destroy(0u, runtime) == 0u && runtime.frees.empty(),
          "null manager destruction is inert");
}

void UpdateObjectList() {
    Runtime runtime;
    constexpr std::uint32_t manager = 0x6000u;
    runtime.words[manager + 4u] = 0x5000u;
    runtime.words[manager + 8u] = 0x5008u;
    runtime.words[0x5000u] = 0x7000u;
    runtime.words[0x5004u] = 0x7100u;
    runtime.words[0x7000u + 32u] = 0x9000u;
    runtime.words[0x7100u + 32u] = 0u;
    Check(Manager::UpdateObjectList(manager, 7u, runtime) == 0x5008u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaActorCall::RenderClump, {0x9000u, 7u}},
                  {pal4::ArenaActorCall::ProcessAnimation,
                   {0x9000u + 368u, 7u}}}),
          "object list renders only non-null actor pointers at entry +32");
    runtime.calls.clear();
    runtime.words[manager + 8u] = 0x5000u;
    Check(Manager::UpdateObjectList(manager, 3u, runtime) == 0x5000u &&
              runtime.calls.empty(),
          "empty object list returns its end pointer");
}

void UpdateTimedObjects() {
    Runtime runtime;
    constexpr std::uint32_t manager = 0x6000u;
    constexpr std::uint32_t first = 0x7000u;
    constexpr std::uint32_t second = 0x7100u;
    runtime.words[manager + 4u] = 0x5000u;
    runtime.words[manager + 8u] = 0x5000u;
    const auto delta_bits = std::bit_cast<std::uint32_t>(1.0f);
    Check(Manager::UpdateTimedObjects(manager, 1.0f, runtime) == 0u &&
              runtime.skill_calls.empty(),
          "empty timed list still advances the fade queue");

    runtime.skill_calls.clear();
    runtime.words[manager + 8u] = 0x5008u;
    runtime.words[0x5000u] = first;
    runtime.words[0x5004u] = second;
    runtime.words[first] = 1u;
    runtime.words[first + 4u] = 1u;
    runtime.words[first + 20u] =
        std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[first + 24u] =
        std::bit_cast<std::uint32_t>(1.0f);
    runtime.words[second + 20u] =
        std::bit_cast<std::uint32_t>(-1.0f);
    Check(Manager::UpdateTimedObjects(manager, 1.0f, runtime) == 0u &&
              runtime.words[first + 24u] ==
                  std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words[first + 28u] == 0u &&
              runtime.words[0x5000u] == second &&
              runtime.words[manager + 8u] == 0x5004u &&
              runtime.frees == (std::vector<std::uint32_t>{first}) &&
              runtime.skill_calls ==
                  (std::vector<Runtime::SkillInvocation>{
                      {pal4::ArenaSkillCall::NeedsContentUpdate,
                       {first + 36u}},
                      {pal4::ArenaSkillCall::UpdateContent,
                       {first + 36u}},
                      {pal4::ArenaSkillCall::GetGameObjectManager, {}},
                      {pal4::ArenaSkillCall::ProcessMinimapOne,
                       {0xa000u, 0u}},
                      {pal4::ArenaSkillCall::ResetEffectHandle,
                       {first + 36u}}}),
          "expired non-actor record refreshes and releases before compacting");
    Check(runtime.words[second + 24u] == 0u,
          "negative-duration record does not advance its timer");

    runtime = Runtime{};
    runtime.words[manager + 4u] = 0x5000u;
    runtime.words[manager + 8u] = 0x5004u;
    runtime.words[manager + 36u] = 0x8000u;
    runtime.words[manager + 40u] = 0x8000u;
    runtime.words[manager + 44u] = 0x800cu;
    runtime.words[0x5000u] = first;
    runtime.words[first] = 1u;
    runtime.words[first + 4u] = 3u;
    runtime.words[first + 20u] =
        std::bit_cast<std::uint32_t>(8.0f);
    runtime.words[first + 24u] =
        std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[first + 32u] = 0x9000u;
    runtime.words[0x9000u + 20u] = 0xa500u;
    runtime.needs_content_update = false;
    Check(Manager::UpdateTimedObjects(manager, 1.0f, runtime) == 0x8000u &&
              runtime.words[first + 24u] ==
                  std::bit_cast<std::uint32_t>(3.0f) &&
              runtime.words[manager + 8u] == 0x5004u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaActorCall::UpdateAndRenderClump,
                   {delta_bits}},
                  {pal4::ArenaActorCall::UpdateInterpolator,
                   {delta_bits}}}) &&
              runtime.skill_calls.empty(),
          "active actor updates while its timer is below the duration");

    runtime.skill_calls.clear();
    runtime.calls.clear();
    runtime.words[first] = 0u;
    Check(Manager::UpdateTimedObjects(manager, 0.0f, runtime) == 0xbeefu &&
              runtime.words[first + 24u] ==
                  std::bit_cast<std::uint32_t>(8.0f) &&
              runtime.words[manager + 8u] == 0x5000u &&
              runtime.words[manager + 40u] == 0x800cu &&
              runtime.words[0x8000u] == 0x3f800000u &&
              runtime.words[0x8004u] == 0u &&
              runtime.words[0x8008u] == first &&
              runtime.frees.empty() &&
              runtime.skill_calls ==
                  (std::vector<Runtime::SkillInvocation>{
                      {pal4::ArenaSkillCall::NeedsContentUpdate,
                       {first + 36u}},
                      {pal4::ArenaSkillCall::GetGameObjectManager, {}},
                      {pal4::ArenaSkillCall::ProcessMinimapThree,
                       {0xa000u, 0u}},
                      {pal4::ArenaSkillCall::InterpolateAlpha,
                       {0x3f800000u, 0u, 0u, 0u}},
                      {pal4::ArenaSkillCall::SetMaterialFlags,
                       {0xa500u, 0x40404040u}}}),
          "forced expiry queues an actor for fading without freeing it");
}

void UpdateFadeState() {
    Runtime runtime;
    constexpr std::uint32_t manager = 0x6000u;
    constexpr std::uint32_t fade = 0x5000u;
    constexpr std::uint32_t record = 0x7000u;
    constexpr std::uint32_t actor = 0x9000u;
    constexpr std::uint32_t clump = 0xa000u;
    runtime.words[manager + 36u] = fade;
    runtime.words[manager + 40u] = fade + 12u;
    runtime.words[fade] = std::bit_cast<std::uint32_t>(4.0f);
    runtime.words[fade + 4u] = std::bit_cast<std::uint32_t>(1.0f);
    runtime.words[fade + 8u] = record;
    runtime.words[record + 32u] = actor;
    runtime.words[actor + 20u] = clump;
    Check(Manager::UpdateFadeState(manager, 1.0f, runtime) == 0xbeefu &&
              runtime.words[fade + 4u] ==
                  std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words[manager + 40u] == fade + 12u &&
              runtime.skill_calls ==
                  (std::vector<Runtime::SkillInvocation>{
                      {pal4::ArenaSkillCall::InterpolateAlpha,
                       {0x3f800000u, 0u,
                        std::bit_cast<std::uint32_t>(0.5f), 0u}},
                      {pal4::ArenaSkillCall::SetMaterialFlags,
                       {clump, 0x40404040u}}}),
          "in-progress fade updates elapsed time and clump color");

    runtime.skill_calls.clear();
    runtime.calls.clear();
    Check(Manager::UpdateFadeState(manager, 3.0f, runtime) == fade + 12u &&
              runtime.words[fade + 4u] ==
                  std::bit_cast<std::uint32_t>(4.0f) &&
              runtime.words[manager + 40u] == fade &&
              runtime.frees == (std::vector<std::uint32_t>{record}) &&
              runtime.skill_calls ==
                  (std::vector<Runtime::SkillInvocation>{
                      {pal4::ArenaSkillCall::InterpolateAlpha,
                       {0x3f800000u, 0u,
                        std::bit_cast<std::uint32_t>(1.0f), 0u}},
                      {pal4::ArenaSkillCall::SetMaterialFlags,
                       {clump, 0x40404040u}},
                      {pal4::ArenaSkillCall::SetMaterialFlags,
                       {clump, 0x40404040u}},
                      {pal4::ArenaSkillCall::GetGameArenaManager, {}},
                      {pal4::ArenaSkillCall::RemoveClump,
                       {0xa100u, clump}},
                      {pal4::ArenaSkillCall::DestroyActor, {actor, 1u}},
                      {pal4::ArenaSkillCall::ResetEffectHandle,
                       {record + 36u}}}) &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::ArenaActorCall::CleanupObjectData, {actor + 368u}},
                  {pal4::ArenaActorCall::GetGameInstance, {}},
                  {pal4::ArenaActorCall::CleanupClump,
                   {actor, 0x1acu}}}),
          "finished fade removes clump, cleans actor and frees the record");
    runtime.skill_calls.clear();
    Check(Manager::ResetContent(record, runtime) == 0u &&
              runtime.skill_calls ==
                  (std::vector<Runtime::SkillInvocation>{
                      {pal4::ArenaSkillCall::ResetEffectHandle,
                       {record + 36u}}}),
          "effect reset forwards embedded handle at offset 36");
}

void ClearActiveObjects() {
    Runtime runtime;
    constexpr std::uint32_t manager = 0x6000u;
    runtime.words[manager + 4u] = 0x5000u;
    runtime.words[manager + 8u] = 0x5004u;
    runtime.words[0x5000u] = 0x7000u;
    runtime.words[manager + 20u] = 0x5100u;
    runtime.words[manager + 24u] = 0x5104u;
    runtime.words[0x5100u] = 0x7100u;
    runtime.words[manager + 36u] = 0x5200u;
    runtime.words[manager + 40u] = 0x520cu;
    runtime.words[0x5208u] = 0x7200u;
    runtime.words[manager + 48u] = 0xaaaau;
    runtime.words[manager + 52u] = 0xbbbbu;
    runtime.words[0x7000u + 32u] = 0x9000u;
    runtime.words[0x7200u + 32u] = 0x9200u;
    for (const auto record : {0x7000u, 0x7100u, 0x7200u})
        runtime.words[record + 28u] = 0xffffu;
    Check(Manager::ClearActiveObjects(manager, runtime) == 0x520cu &&
              runtime.words[manager + 8u] == 0x5000u &&
              runtime.words[manager + 24u] == 0x5100u &&
              runtime.words[manager + 40u] == 0x5200u &&
              runtime.words[manager + 48u] == 0u &&
              runtime.words[manager + 52u] == 0u &&
              runtime.words[0x7000u + 28u] == 0u &&
              runtime.words[0x7100u + 28u] == 0u &&
              runtime.words[0x7200u + 28u] == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{
                  0x7000u, 0x7100u, 0x7200u}) &&
              runtime.skill_calls ==
                  (std::vector<Runtime::SkillInvocation>{
                      {pal4::ArenaSkillCall::DestroyActor, {0x9000u, 1u}},
                      {pal4::ArenaSkillCall::NeedsContentUpdate,
                       {0x7000u + 36u}},
                      {pal4::ArenaSkillCall::UpdateContent,
                       {0x7000u + 36u}},
                      {pal4::ArenaSkillCall::ResetEffectHandle,
                       {0x7000u + 36u}},
                      {pal4::ArenaSkillCall::NeedsContentUpdate,
                       {0x7100u + 36u}},
                      {pal4::ArenaSkillCall::UpdateContent,
                       {0x7100u + 36u}},
                      {pal4::ArenaSkillCall::ResetEffectHandle,
                       {0x7100u + 36u}},
                      {pal4::ArenaSkillCall::DestroyActor, {0x9200u, 1u}},
                      {pal4::ArenaSkillCall::NeedsContentUpdate,
                       {0x7200u + 36u}},
                      {pal4::ArenaSkillCall::UpdateContent,
                       {0x7200u + 36u}},
                      {pal4::ArenaSkillCall::ResetEffectHandle,
                       {0x7200u + 36u}}}),
          "clear releases all three lists in order and resets manager fields");
    Check(runtime.calls == (std::vector<Runtime::Invocation>{
              {pal4::ArenaActorCall::CleanupObjectData, {0x9000u + 368u}},
              {pal4::ArenaActorCall::GetGameInstance, {}},
              {pal4::ArenaActorCall::CleanupClump, {0x9000u, 0x1acu}},
              {pal4::ArenaActorCall::CleanupObjectData, {0x9200u + 368u}},
              {pal4::ArenaActorCall::GetGameInstance, {}},
              {pal4::ArenaActorCall::CleanupClump, {0x9200u, 0x1acu}}}),
          "clear composes actor cleanup only for non-null actors");

    runtime = Runtime{};
    runtime.words[manager + 4u] = 0x5000u;
    runtime.words[manager + 8u] = 0x5000u;
    runtime.words[manager + 20u] = 0x5100u;
    runtime.words[manager + 24u] = 0x5100u;
    runtime.words[manager + 36u] = 0x5200u;
    runtime.words[manager + 40u] = 0x5200u;
    Check(Manager::ClearActiveObjects(manager, runtime) == 0x5200u &&
              runtime.frees.empty() && runtime.calls.empty() &&
              runtime.skill_calls.empty(),
          "empty lists clear without touching resource entries");
}

void ForEachObject() {
    Runtime runtime;
    constexpr std::uint32_t manager = 0x6000u;
    runtime.words[manager + 4u] = 0x5000u;
    runtime.words[manager + 8u] = 0x5000u;
    std::vector<std::uint32_t> seen;
    const auto collect = [&seen](std::uint32_t value, std::uint32_t argument) {
        seen.push_back(value + argument);
        return 1u;
    };
    Check(Manager::ForEachObject(manager, collect, 4u, runtime) == 0x5000u &&
              seen.empty(),
          "empty first array returns its current end");
    runtime.words[manager + 8u] = 0x500cu;
    runtime.words[0x5000u] = 1u;
    runtime.words[0x5004u] = 2u;
    runtime.words[0x5008u] = 3u;
    Check(Manager::ForEachObject(manager, collect, 4u, runtime) == 0x500cu &&
              seen == (std::vector<std::uint32_t>{5u, 6u, 7u}),
          "visitor receives every pointer with its argument");
    seen.clear();
    const auto stop = [&seen](std::uint32_t value, std::uint32_t) {
        seen.push_back(value);
        return value == 2u ? 0u : 1u;
    };
    Check(Manager::ForEachObject(manager, stop, 0u, runtime) == 0u &&
              seen == (std::vector<std::uint32_t>{1u, 2u}),
          "zero callback result stops traversal immediately");
    seen.clear();
    const auto shorten = [&runtime, &seen, manager](
        std::uint32_t value, std::uint32_t) {
        seen.push_back(value);
        runtime.words[manager + 8u] = 0x5004u;
        return 1u;
    };
    Check(Manager::ForEachObject(manager, shorten, 0u, runtime) == 0x5004u &&
              seen == (std::vector<std::uint32_t>{1u}),
          "visitor rereads the end after callback mutation");
}

bool HasSkillCall(const Runtime& runtime, pal4::ArenaSkillCall operation,
                  std::vector<std::uint32_t> arguments) {
    return std::find(runtime.skill_calls.begin(), runtime.skill_calls.end(),
                     Runtime::SkillInvocation{operation, arguments}) !=
           runtime.skill_calls.end();
}

void HandlePlayerAction() {
    constexpr std::uint32_t manager = 0x6000u;
    constexpr std::uint32_t record = 0x7000u;
    constexpr std::uint32_t player = 0x8000u;
    constexpr std::uint32_t actor = 0x9000u;
    constexpr std::uint32_t position[] = {0x3f800000u, 0x40000000u,
                                           0x40400000u};
    constexpr std::string_view sounds[] = {
        "WF032.wav", "WF033.wav", "WF034.wav", "WF035.wav"};
    for (auto action = 0u; action < 4u; ++action) {
        Runtime runtime;
        runtime.allocation_results = {record, actor};
        runtime.words[manager + 8u] = 0x5000u;
        runtime.words[manager + 48u] = 7u;
        runtime.words[player + 164u] = 0x1234u;
        runtime.words[actor + 20u] = 0xa500u;
        for (auto word = 0u; word < 3u; ++word)
            runtime.words[player + 172u + word * 4u] = position[word];
        runtime.words[0xc000u] = 0xaaaau;
        runtime.words[0xc004u] = 0xbbbbu;

        Check(Manager::HandlePlayerAction(manager, action, runtime) == 1u,
              "valid player action succeeds");
        Check(runtime.allocations ==
                  (action == 0u ? std::vector<std::uint32_t>{44u, 0x1ecu} :
                                  std::vector<std::uint32_t>{44u}) &&
                  runtime.words[record] == 1u &&
                  runtime.words[record + 4u] == action &&
                  runtime.words[record + 20u] ==
                      std::bit_cast<std::uint32_t>(10.0f) &&
                  runtime.words[record + 24u] == 0u &&
                  runtime.words[record + 28u] == 0u &&
                  runtime.sounds ==
                      (std::vector<std::string>{std::string(sounds[action])}),
              "action record fields, allocation and sound match IDA");
        for (auto word = 0u; word < 3u; ++word)
            Check(runtime.words[record + 8u + word * 4u] == position[word],
                  "record copies the player position");
        Check(HasSkillCall(runtime, pal4::ArenaSkillCall::GetPlayerByIndex,
                           {0xb000u, action}) &&
                  HasSkillCall(runtime,
                               pal4::ArenaSkillCall::InitializeRecordName,
                               {record + 36u}) &&
                  HasSkillCall(runtime,
                               pal4::ArenaSkillCall::InsertActiveRecord,
                               {manager, 0x5000u, 1u, record}),
              "all actions resolve player, initialize name and queue record");

        if (action == 0u) {
            Check(runtime.words[record + 32u] == actor &&
                      runtime.words[actor] ==
                          pal4::ArenaActorResource::kResourceVtable &&
                      runtime.words[manager + 48u] == 8u &&
                      HasSkillCall(runtime,
                                   pal4::ArenaSkillCall::LoadSnareResource,
                                   {actor, 7u}) &&
                      HasSkillCall(runtime,
                                   pal4::ArenaSkillCall::SetActorPosition,
                                   {actor, record + 8u, 0u}) &&
                      HasSkillCall(runtime,
                                   pal4::ArenaSkillCall::SetActorRotation,
                                   {actor, 0u, 0x1234u, 0u, 0u}) &&
                      HasSkillCall(runtime,
                                   pal4::ArenaSkillCall::SetAnimationFlag,
                                   {actor + 368u, 0u}) &&
                      HasSkillCall(runtime,
                                   pal4::ArenaSkillCall::AddClump,
                                   {0xa100u, 0xa500u}),
                  "snare action constructs and installs its arena actor");
        } else {
            Check(runtime.words[record + 32u] == 0u &&
                      runtime.words[manager + 48u] == 7u,
                  "non-snare actions have no actor or sequence increment");
        }
        if (action == 2u) {
            Check(HasSkillCall(runtime,
                               pal4::ArenaSkillCall::CreateEnthrallEffect,
                               {record + 8u, 0xffffffffu, 0u}) &&
                      runtime.words[record + 36u] == 0xaaaau &&
                      runtime.words[record + 40u] == 0xbbbbu &&
                      HasSkillCall(runtime,
                                   pal4::ArenaSkillCall::ResetEffectHandle,
                                   {0xc000u}),
                  "Enthrall effect handle transfers to the record");
        } else {
            Check(HasSkillCall(runtime,
                               pal4::ArenaSkillCall::ResetEffectHandle,
                               {record + 36u}),
                  "other actions reset the embedded effect handle");
        }
        if (action == 1u || action == 3u) {
            const auto op = action == 1u ?
                pal4::ArenaSkillCall::ProcessMinimapOne :
                pal4::ArenaSkillCall::ProcessMinimapThree;
            Check(HasSkillCall(runtime, op, {0xa000u, 1u}) &&
                      runtime.skill_calls[2].operation ==
                          pal4::ArenaSkillCall::GetGameObjectManager,
                  "minimap traversal precedes record construction");
        }
    }

    Runtime runtime;
    runtime.player_exists = false;
    Check(Manager::HandlePlayerAction(manager, 0u, runtime) == 0u &&
              runtime.allocations.empty() && runtime.sounds.empty(),
          "missing player leaves manager untouched");
    runtime = Runtime{};
    Check(Manager::HandlePlayerAction(manager, 4u, runtime) == 0u &&
              runtime.allocations.empty() && runtime.sounds.empty(),
          "unsupported action does not allocate or play audio");
    runtime = Runtime{};
    runtime.allocation_results = {0u};
    Check(Manager::HandlePlayerAction(manager, 1u, runtime) == 0u &&
              runtime.sounds.empty(),
          "allocation failure does not dispatch an incomplete action");
    runtime = Runtime{};
    runtime.allocation_results = {record, 0u};
    Check(Manager::HandlePlayerAction(manager, 0u, runtime) == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{record}) &&
              runtime.sounds.empty(),
          "failed snare actor allocation releases the pending record");
}

void ShowSignpost() {
    constexpr std::uint32_t manager = 0x6000u;
    constexpr std::uint32_t record = 0x7000u;
    constexpr std::uint32_t actor = 0x9000u;
    Runtime runtime;
    runtime.allocation_results = {record, actor, 0x5000u};
    runtime.words[0x8000u + 164u] = 0x2244u;
    runtime.words[0x8000u + 172u] = 11u;
    runtime.words[0x8000u + 176u] = 22u;
    runtime.words[0x8000u + 180u] = 33u;
    runtime.words[0xc000u] = 0xaau;
    runtime.words[0xc004u] = 0xbbu;
    runtime.words[actor + 20u] = 0xa500u;
    Check(Manager::ShowSignpost(manager, 6u, runtime) == 1u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{44u, 0x1ecu, 4u}) &&
              runtime.words[record] == 1u &&
              runtime.words[record + 4u] == 4u &&
              runtime.words[record + 8u] == 11u &&
              runtime.words[record + 12u] == 22u &&
              runtime.words[record + 16u] == 33u &&
              runtime.words[record + 20u] ==
                  std::bit_cast<std::uint32_t>(-1.0f) &&
              runtime.words[record + 24u] == 0u &&
              runtime.words[record + 28u] == 0u &&
              runtime.words[record + 32u] == actor &&
              runtime.words[record + 36u] == 0xaau &&
              runtime.words[record + 40u] == 0xbbu &&
              runtime.words[manager + 20u] == 0x5000u &&
              runtime.words[manager + 24u] == 0x5004u &&
              runtime.words[manager + 28u] == 0x5004u &&
              runtime.words[manager + 52u] == 1u &&
              runtime.words[0x5000u] == record,
          "new signpost initializes record, actor, effect and secondary array");
    Check(HasSkillCall(runtime,
                       pal4::ArenaSkillCall::CreateSignpostEffect,
                       {record + 8u, 0xffffffffu, 0u}) &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::LoadSignpostResource,
                           {actor, 1u}) &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::SetActorPosition,
                           {actor, record + 8u, 0u}) &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::SetActorRotation,
                           {actor, 0u, 0x2244u, 0u, 0u}) &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::AddClump,
                           {0xa100u, 0xa500u}),
          "signpost effect and M001 actor use confirmed runtime inputs");

    runtime = Runtime{};
    runtime.allocation_results = {record, actor};
    runtime.words[manager + 20u] = 0x5000u;
    runtime.words[manager + 24u] = 0x5004u;
    runtime.words[manager + 28u] = 0x5008u;
    runtime.words[manager + 52u] = 9u;
    runtime.words[0x5000u] = 0x7100u;
    Check(Manager::ShowSignpost(manager, 2u, runtime) == 1u &&
              runtime.words[0x5000u] == 0x7100u &&
              runtime.words[0x5004u] == record &&
              runtime.words[manager + 24u] == 0x5008u &&
              runtime.words[manager + 52u] == 10u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{44u, 0x1ecu}) &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::LoadSignpostResource,
                           {actor, 10u}),
          "available secondary capacity appends without reallocation");

    runtime = Runtime{};
    runtime.allocation_results = {record, actor};
    runtime.words[manager + 20u] = 0x5000u;
    runtime.words[manager + 24u] = 0x5008u;
    runtime.words[manager + 28u] = 0x5008u;
    runtime.words[manager + 36u] = 0x5200u;
    runtime.words[manager + 40u] = 0x5200u;
    runtime.words[manager + 44u] = 0x520cu;
    runtime.words[manager + 52u] = 10u;
    runtime.words[0x5000u] = 0x7100u;
    runtime.words[0x5004u] = 0x7200u;
    runtime.words[0x7100u + 28u] = 1u;
    runtime.words[0x7100u + 32u] = 0x9100u;
    Check(Manager::ShowSignpost(manager, 3u, runtime) == 1u &&
              runtime.words[0x7100u + 28u] == 0u &&
              runtime.words[0x5000u] == 0x7200u &&
              runtime.words[0x5004u] == record &&
              runtime.words[manager + 24u] == 0x5008u &&
              runtime.words[manager + 52u] == 10u &&
              runtime.words[manager + 40u] == 0x520cu &&
              runtime.words[0x5200u] == 0x3f800000u &&
              runtime.words[0x5204u] == 0u &&
              runtime.words[0x5208u] == 0x7100u &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::LoadSignpostResource,
                           {actor, 10u}),
          "full signpost list retires oldest to fade before appending");
    Check(runtime.frees.empty() &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::NeedsContentUpdate,
                           {0x7100u + 36u}) &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::UpdateContent,
                           {0x7100u + 36u}),
          "retired signpost refreshes content and stays owned by fade queue");

    runtime = Runtime{};
    runtime.allocation_results = {record, actor, 0x5100u};
    runtime.words[manager + 20u] = 0x5000u;
    runtime.words[manager + 24u] = 0x5008u;
    runtime.words[manager + 28u] = 0x5008u;
    runtime.words[manager + 52u] = 2u;
    runtime.words[0x5000u] = 0x7100u;
    runtime.words[0x5004u] = 0x7200u;
    Check(Manager::ShowSignpost(manager, 1u, runtime) == 1u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{44u, 0x1ecu, 16u}) &&
              runtime.words[0x5100u] == 0x7100u &&
              runtime.words[0x5104u] == 0x7200u &&
              runtime.words[0x5108u] == record &&
              runtime.words[manager + 20u] == 0x5100u &&
              runtime.words[manager + 24u] == 0x510cu &&
              runtime.words[manager + 28u] == 0x5110u &&
              runtime.words[manager + 52u] == 3u &&
              runtime.frees == (std::vector<std::uint32_t>{0x5000u}),
          "secondary array grows geometrically and preserves existing records");

    runtime = Runtime{};
    runtime.allocation_results = {record, 0u};
    Check(Manager::ShowSignpost(manager, 1u, runtime) == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{record}) &&
              runtime.words[manager + 52u] == 0u,
          "failed signpost actor allocation releases its record");

    runtime = Runtime{};
    runtime.allocation_results = {record, actor, 0u};
    runtime.words[actor + 20u] = 0xa500u;
    Check(Manager::ShowSignpost(manager, 1u, runtime) == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{record}) &&
              runtime.words[manager + 24u] == 0u &&
              HasSkillCall(runtime, pal4::ArenaSkillCall::RemoveClump,
                           {0xa100u, 0xa500u}) &&
              HasSkillCall(runtime, pal4::ArenaSkillCall::DestroyActor,
                           {actor, 1u}),
          "failed array growth removes the unqueued actor and record");
}

void PlayerDataLifecycle() {
    constexpr std::uint32_t manager = 0x6000u;
    Runtime runtime;
    runtime.words[manager + 4u] = 0x5000u;
    runtime.words[manager + 8u] = 0x5004u;
    runtime.words[0x5000u] = 0x7000u;
    runtime.words[0x7000u + 32u] = 0x9000u;
    runtime.words[0x7000u + 28u] = 1u;
    runtime.words[manager + 20u] = 0x5100u;
    runtime.words[manager + 24u] = 0x5108u;
    runtime.words[0x5100u] = 0x7100u;
    runtime.words[0x5104u] = 0u;
    runtime.words[0x7100u + 32u] = 0x9100u;
    runtime.words[0x7100u + 28u] = 1u;
    runtime.words[0x9100u + 20u] = 0xa500u;
    runtime.words[manager + 36u] = 0x5200u;
    runtime.words[manager + 40u] = 0x520cu;
    runtime.words[0x5208u] = 0x7200u;
    runtime.words[0x7200u + 32u] = 0x9200u;
    runtime.words[0x7200u + 28u] = 1u;
    Check(Manager::CleanupPlayerData(manager, runtime) == 0x520cu &&
              runtime.words[manager + 8u] == 0x5000u &&
              runtime.words[manager + 24u] == 0x5108u &&
              runtime.words[manager + 40u] == 0x5200u &&
              runtime.words[0x7000u + 28u] == 0u &&
              runtime.words[0x7100u + 28u] == 1u &&
              runtime.words[0x7200u + 28u] == 0u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x7000u, 0x7200u}) &&
              HasSkillCall(runtime, pal4::ArenaSkillCall::RemoveClump,
                           {0xa100u, 0xa500u}),
          "scene cleanup releases first and fade lists but retains signposts");
    Check(!HasSkillCall(runtime, pal4::ArenaSkillCall::DestroyActor,
                        {0x9100u, 1u}) &&
              HasSkillCall(runtime, pal4::ArenaSkillCall::UpdateContent,
                           {0x7100u + 36u}),
          "retained signpost only detaches clump and refreshes content");

    runtime = Runtime{};
    runtime.words[manager + 20u] = 0x5100u;
    runtime.words[manager + 24u] = 0x510cu;
    runtime.words[0x5100u] = 0u;
    runtime.words[0x5104u] = 0x7100u;
    runtime.words[0x5108u] = 0x7200u;
    runtime.words[0x7100u + 32u] = 0u;
    runtime.words[0x7200u + 32u] = 0x9200u;
    runtime.words[0x9200u + 20u] = 0xa500u;
    runtime.words[0xc000u] = 0xaaaau;
    runtime.words[0xc004u] = 0xbbbbu;
    Check(Manager::UpdatePlayerData(manager, runtime) == manager &&
              HasSkillCall(runtime, pal4::ArenaSkillCall::AddClump,
                           {0xa100u, 0xa500u}) &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::CreateSignpostResumeEffect,
                           {0x7200u + 8u, 0xffffffffu, 0u}) &&
              runtime.words[0x7200u + 36u] == 0xaaaau &&
              runtime.words[0x7200u + 40u] == 0xbbbbu &&
              HasSkillCall(runtime, pal4::ArenaSkillCall::ResetEffectHandle,
                           {0xc000u}),
          "resume reattaches only live actors and recreates their effects");
    runtime = Runtime{};
    runtime.words[manager + 20u] = 0x5100u;
    runtime.words[manager + 24u] = 0x5104u;
    runtime.words[0x5100u] = 0x7200u;
    runtime.words[0x7200u + 32u] = 0x9200u;
    runtime.needs_content_update = false;
    Check(Manager::UpdatePlayerData(manager, runtime) == manager &&
              HasSkillCall(runtime, pal4::ArenaSkillCall::AddClump,
                           {0xa100u, 0u}) &&
              !HasSkillCall(runtime,
                            pal4::ArenaSkillCall::CreateSignpostResumeEffect,
                            {0x7200u + 8u, 0xffffffffu, 0u}),
          "existing effect is preserved when content needs no update");
    runtime.words[manager + 24u] = 0x5100u;
    runtime.skill_calls.clear();
    Check(Manager::UpdatePlayerData(manager, runtime) == 0x5100u &&
              runtime.skill_calls.empty(),
          "empty signpost list returns its end without native calls");
}

void MinimapCallbacks() {
    constexpr std::uint32_t object = 0x8000u;
    Runtime runtime;
    runtime.words[object + 4u] = 0x9000u;
    runtime.words[object + 432u] = 4u;
    runtime.words[object + 388u] = 1u;
    runtime.words[0xd000u] = std::bit_cast<std::uint32_t>(4.9f);
    runtime.words[0xd004u] = std::bit_cast<std::uint32_t>(-2.9f);
    Check(Manager::UpdateTreasureMinimap(object, true, runtime) == object &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::ConvertGridCoordinates,
                           {0xa200u, object + 172u}) &&
              HasSkillCall(runtime, pal4::ArenaSkillCall::AddTreasureIcon,
                           {0xa200u, 0x9000u, 3u, 4u,
                            static_cast<std::uint32_t>(-2)}) &&
              !HasSkillCall(runtime,
                            pal4::ArenaSkillCall::RemoveMinimapIcon,
                            {0xa200u, 0x9000u, 3u}),
          "visible treasure adds category-three icon at truncated grid point");
    runtime.skill_calls.clear();
    Check(Manager::UpdateTreasureMinimap(object, false, runtime) == object &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::RemoveMinimapIcon,
                           {0xa200u, 0x9000u, 3u}) &&
              !HasSkillCall(runtime,
                            pal4::ArenaSkillCall::ConvertGridCoordinates,
                            {0xa200u, object + 172u}),
          "disabled treasure removes icon without coordinate conversion");
    runtime.skill_calls.clear();
    runtime.words[object + 388u] = 0u;
    Check(Manager::UpdateTreasureMinimap(object, true, runtime) == object &&
              runtime.skill_calls.empty(),
          "treasure without content is ignored before visibility lookup");

    runtime = Runtime{};
    runtime.words[object + 4u] = 0x9000u;
    runtime.words[object + 432u] = 1u;
    runtime.words[object + 388u] = 1u;
    runtime.words[0xd000u] = std::bit_cast<std::uint32_t>(-7.5f);
    runtime.words[0xd004u] = std::bit_cast<std::uint32_t>(3.5f);
    Check(Manager::UpdateOreMinimap(object, true, runtime) == object &&
              HasSkillCall(runtime, pal4::ArenaSkillCall::AddOreIcon,
                           {0xa200u, 0x9000u, 2u,
                            static_cast<std::uint32_t>(-7), 3u}),
          "visible ore adds category-two icon at truncated grid point");
    runtime.skill_calls.clear();
    runtime.words[object + 388u] = 0u;
    Check(Manager::UpdateOreMinimap(object, true, runtime) == object &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::IsObjectVisible, {object}) &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::RemoveMinimapIcon,
                           {0xa200u, 0x9000u, 2u}),
          "ore without content removes its old icon after visibility check");
    runtime.skill_calls.clear();
    runtime.words[object + 440u] = 1u;
    Check(Manager::UpdateOreMinimap(object, true, runtime) == object &&
              runtime.skill_calls.empty(),
          "inactive map object is ignored");
    runtime.words[object + 440u] = 0u;
    runtime.object_visible = false;
    Check(Manager::UpdateOreMinimap(object, true, runtime) == object &&
              HasSkillCall(runtime,
                           pal4::ArenaSkillCall::RemoveMinimapIcon,
                           {0xa200u, 0x9000u, 2u}),
          "hidden ore removes icon");
}

}  // namespace

int main() {
    try {
        RegistrationAndFactory();
        CleanupAndDestroy();
        UpdateObjectList();
        UpdateTimedObjects();
        UpdateFadeState();
        ClearActiveObjects();
        ForEachObject();
        HandlePlayerAction();
        ShowSignpost();
        PlayerDataLifecycle();
        MinimapCallbacks();
        pal4::ArenaSkillRuntimeStub stub;
        Check(Manager::Create(stub) == 0u,
              "native skill boundary stays linkable");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
