#include "pal4/actor_manager_player_collection.h"

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

constexpr std::uint32_t kCollection = 0x1000u;

struct Runtime final : pal4::IActorManagerPlayerCollectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::uint32_t free_result = 0x55u;
    bool mutate_on_second_free = false;
    std::uint32_t allocated = 0x5000u;
    std::uint32_t constructed = 0x5000u;
    std::string name = "Xuan Xiao";
    std::map<std::uint32_t, std::uint32_t> players;
    bool append_on_copy = false;
    std::uint32_t iterator = 0x6000u;
    std::map<std::uint32_t, std::uint32_t> actor_values;
    bool append_on_child_update = false;
    bool append_on_animation_update = false;
    bool append_on_slot388 = false;
    bool change_second_begin_on_slot388 = false;
    std::uint32_t dialog_value = 0u;
    std::uint32_t dialog_calls = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        trace.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
        if (mutate_on_second_free && address == 0x3300u)
            words[kCollection + 4u] = 0x4400u;
        return free_result;
    }
    void ReportNullPlayer(std::string_view context,
                          std::string_view message) override {
        trace.emplace_back("null:" + std::string(context) + ":" +
                           std::string(message));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        trace.emplace_back("allocate:" + std::to_string(bytes));
        return bytes == 12u ? iterator : allocated;
    }
    std::uint32_t ConstructActor(std::uint32_t storage,
                                 std::uint32_t player) override {
        trace.emplace_back("construct:" + std::to_string(storage) +
                           ":" + std::to_string(player));
        return constructed;
    }
    std::string_view ActorBaseName(std::uint32_t actor) override {
        trace.emplace_back("base-name:" + std::to_string(actor));
        return name;
    }
    void AssignActorName(std::uint32_t actor,
                         std::string_view value) override {
        trace.emplace_back("name:" + std::to_string(actor) +
                           ":" + std::string(value));
    }
    void InsertActor(std::uint32_t collection, std::uint32_t position,
                     std::uint32_t actor) override {
        trace.emplace_back("insert:" + std::to_string(collection) +
                           ":" + std::to_string(position) +
                           ":" + std::to_string(actor));
    }
    std::uint32_t PlayerActionState(std::uint32_t player) override {
        trace.emplace_back("action:" + std::to_string(player));
        return 9u;
    }
    void SetActorState(std::uint32_t actor,
                       std::uint32_t state) override {
        trace.emplace_back("state:" + std::to_string(actor) +
                           ":" + std::to_string(state));
    }
    void DispatchNewActor(std::string_view event, std::uint32_t actor,
                          std::uint32_t mode) override {
        trace.emplace_back("event:" + std::string(event) +
                           ":" + std::to_string(actor) +
                           ":" + std::to_string(mode));
    }
    std::uint32_t PlayerManager() override {
        trace.emplace_back("player-manager");
        return 0x7000u;
    }
    std::uint32_t FindPlayer(std::uint32_t manager,
                             std::uint32_t index) override {
        trace.emplace_back("find:" + std::to_string(manager) +
                           ":" + std::to_string(index));
        return players[index];
    }
    void CopyActorToPlayer(std::uint32_t actor,
                           std::uint32_t player) override {
        trace.emplace_back("copy:" + std::to_string(actor) +
                           ":" + std::to_string(player));
        if (append_on_copy) {
            append_on_copy = false;
            words[kCollection + 8u] += 4u;
        }
    }
    std::uint32_t DialogData(std::uint32_t parameter) override {
        trace.emplace_back("dialog:" + std::to_string(parameter));
        ++dialog_calls;
        return dialog_value;
    }
    std::uint32_t ActorValue20(std::uint32_t actor) override {
        trace.emplace_back("value20:" + std::to_string(actor));
        return actor_values[actor];
    }
    void CallChildSlot56(std::uint32_t actor,
                         std::uint32_t argument) override {
        trace.emplace_back("child56:" + std::to_string(actor) +
                           ":" + std::to_string(argument));
        if (append_on_child_update) {
            append_on_child_update = false;
            words[kCollection + 24u] += 4u;
        }
    }
    void UpdateAnimationList(std::uint32_t list,
                             std::uint32_t time_bits) override {
        trace.emplace_back("animation:" + std::to_string(list) +
                           ":" + std::to_string(time_bits));
        if (append_on_animation_update) {
            append_on_animation_update = false;
            words[kCollection + 8u] += 4u;
        }
    }
    void ActorSlot388(std::uint32_t actor) override {
        trace.emplace_back("slot388:" + std::to_string(actor));
        if (append_on_slot388) {
            append_on_slot388 = false;
            words[kCollection + 8u] += 4u;
        }
        if (change_second_begin_on_slot388) {
            change_second_begin_on_slot388 = false;
            words[kCollection + 20u] = 0x3300u;
        }
    }
    void LogRelease(std::uint32_t actor,
                    std::uint32_t name) override {
        trace.emplace_back("release:" + std::to_string(actor) +
                           ":" + std::to_string(name));
    }
    void ActorSlot248(std::uint32_t actor,
                      std::uint32_t flags) override {
        trace.emplace_back("delete:" + std::to_string(actor) +
                           ":" + std::to_string(flags));
    }
    std::uint32_t EventRouter() override {
        trace.emplace_back("router");
        return 0x9000u;
    }
    void RegisterEvent(std::uint32_t router, std::uint32_t mode,
                       std::string_view event, std::uint32_t owner,
                       std::uint32_t callback) override {
        trace.emplace_back("register:" + std::to_string(router) +
                           ":" + std::to_string(mode) + ":" +
                           std::string(event) + ":" +
                           std::to_string(owner) + ":" +
                           std::to_string(callback));
    }
};

}  // namespace

int main() {
    try {
        Runtime constructed;
        constructed.bytes[kCollection] = 0xaau;
        constructed.bytes[kCollection + 16u] = 0xbbu;
        Check(pal4::ActorManagerPlayerCollection::Construct(
                  kCollection, constructed) == kCollection &&
                  constructed.trace == std::vector<std::string>{
                      "byte:4096", "word:4100", "word:4104",
                      "word:4108", "byte:4112", "word:4116",
                      "word:4120", "word:4124",
                      "router", "register:36864:2:re_join_stage:4096:5454160",
                      "router", "register:36864:2:re_leave_stage:4096:5485952",
                      "router", "register:36864:2:combat_over:4096:5748944"} &&
                  constructed.bytes[kCollection] == 0u &&
                  constructed.bytes[kCollection + 16u] == 0u &&
                  constructed.words[kCollection + 4u] == 0u &&
                  constructed.words[kCollection + 28u] == 0u,
              "constructor zeros both array headers before registration");

        Runtime destroyed;
        destroyed.words[kCollection + 4u] = 0x2200u;
        destroyed.words[kCollection + 8u] = 0x2200u;
        destroyed.words[kCollection + 12u] = 0x2210u;
        destroyed.words[kCollection + 20u] = 0x3300u;
        destroyed.words[kCollection + 24u] = 0x3300u;
        destroyed.words[kCollection + 28u] = 0x3310u;
        Check(pal4::ActorManagerPlayerCollection::Destroy(
                  kCollection, destroyed) == 0x55u &&
                  destroyed.trace == std::vector<std::string>{
                      "read:4100", "read:4104", "read:4104",
                      "read:4100", "read:4100", "word:4104",
                      "read:4120", "read:4116", "word:4120",
                      "read:4116", "free:13056",
                      "word:4116", "word:4120", "word:4124",
                      "read:4100", "free:8704", "word:4100",
                      "word:4104", "word:4108"} &&
                  destroyed.words[kCollection + 4u] == 0u &&
                  destroyed.words[kCollection + 8u] == 0u &&
                  destroyed.words[kCollection + 12u] == 0u &&
                  destroyed.words[kCollection + 20u] == 0u &&
                  destroyed.words[kCollection + 24u] == 0u &&
                  destroyed.words[kCollection + 28u] == 0u,
              "destructor clears actors, frees second then first array");

        Runtime changing;
        changing.words[kCollection + 4u] = 0x2200u;
        changing.words[kCollection + 8u] = 0x2204u;
        changing.words[0x2200u] = 0u;
        changing.words[kCollection + 20u] = 0x2300u;
        changing.words[kCollection + 24u] = 0x2300u;
        changing.append_on_slot388 = true;
        changing.change_second_begin_on_slot388 = true;
        changing.mutate_on_second_free = true;
        Check(pal4::ActorManagerPlayerCollection::Destroy(
                  kCollection, changing) == 0x55u &&
                  std::find(changing.trace.begin(), changing.trace.end(),
                            "free:13056") != changing.trace.end() &&
                  std::find(changing.trace.begin(), changing.trace.end(),
                            "free:17408") != changing.trace.end(),
              "destructor reads both pointers after preceding callbacks");

        Runtime missing;
        Check(pal4::ActorManagerPlayerCollection::AddPlayer(
                  kCollection, 0u, 3u, missing) == 0u &&
                  missing.trace == std::vector<std::string>{
                      "null:cbRoleManager::addRole:role pointer is null"},
              "null player reports exact diagnostic and stops");

        Runtime direct;
        direct.words[kCollection + 8u] = 0x3000u;
        Check(pal4::ActorManagerPlayerCollection::AddPlayer(
                  kCollection, 0x2000u, 4u, direct) == 0x5000u &&
                  direct.trace == std::vector<std::string>{
                      "allocate:1320", "construct:20480:8192",
                      "base-name:20480", "name:20480:Xuan Xiao",
                      "read:4104", "insert:4096:12288:20480",
                      "state:20480:4", "event:re_new_added:20480:1"},
              "direct player state follows insertion and precedes event");

        Runtime action;
        action.name.clear();
        Check(pal4::ActorManagerPlayerCollection::AddPlayer(
                  kCollection, 0x2000u, 6u, action) == 0x5000u &&
                  action.trace[3] == "name:20480:" &&
                  action.trace[6] == "action:8192" &&
                  action.trace[7] == "state:20480:9" &&
                  action.trace[8] == "event:re_new_added:20480:1",
              "state six obtains player action state after insertion");

        Runtime allocation_failure;
        allocation_failure.allocated = 0u;
        Check(pal4::ActorManagerPlayerCollection::AddPlayer(
                  kCollection, 0x2000u, 6u, allocation_failure) == 0u &&
                  allocation_failure.trace == std::vector<std::string>{
                      "allocate:1320"},
              "allocation failure stops before native null dereference");

        Runtime empty;
        empty.words[kCollection + 4u] = 0x3000u;
        empty.words[kCollection + 8u] = 0x3000u;
        Check(pal4::ActorManagerPlayerCollection::SyncPlayers(
                  kCollection, empty) == 0x3000u &&
                  empty.trace == std::vector<std::string>{
                      "read:4100", "read:4104"},
              "empty player array returns its end without manager lookup");

        Runtime sync;
        sync.words[kCollection + 4u] = 0x3000u;
        sync.words[kCollection + 8u] = 0x3008u;
        sync.words[0x3000u] = 0x4000u;
        sync.words[0x3004u] = 0x5000u;
        sync.words[0x4000u + 1192u] = 7u;
        sync.words[0x5000u + 1192u] = 8u;
        sync.players[8u] = 0x8000u;
        Check(pal4::ActorManagerPlayerCollection::SyncPlayers(
                  kCollection, sync) == 0x3008u &&
                  sync.trace == std::vector<std::string>{
                      "read:4100", "read:4104", "read:12288",
                      "read:17576", "player-manager", "find:28672:7",
                      "read:4104", "read:12292", "read:21672",
                      "player-manager", "find:28672:8", "copy:20480:32768",
                      "read:4104"},
              "sync skips missing player and copies matching actor");

        Runtime growing;
        growing.words[kCollection + 4u] = 0x3000u;
        growing.words[kCollection + 8u] = 0x3004u;
        growing.words[0x3000u] = 0x4000u;
        growing.words[0x3004u] = 0x5000u;
        growing.words[0x4000u + 1192u] = 1u;
        growing.words[0x5000u + 1192u] = 2u;
        growing.players[1u] = 0x8000u;
        growing.players[2u] = 0x9000u;
        growing.append_on_copy = true;
        Check(pal4::ActorManagerPlayerCollection::SyncPlayers(
                  kCollection, growing) == 0x3008u &&
                  growing.trace.back() == "read:4104" &&
                  growing.trace[12] == "copy:20480:36864",
              "sync re-reads end after a callback appends an actor");

        Runtime iterators;
        iterators.words[kCollection + 4u] = 0x3000u;
        iterators.words[kCollection + 20u] = 0x4000u;
        Check(pal4::ActorManagerPlayerCollection::FirstCandidateIterator(
                  kCollection, 0x8000u, iterators) == 0x8000u &&
                  pal4::ActorManagerPlayerCollection::SecondCandidateIterator(
                      kCollection, 0x8004u, iterators) == 0x8004u &&
                  iterators.trace == std::vector<std::string>{
                      "read:4100", "allocate:12", "word:24576",
                      "word:24580", "word:24584", "word:24576",
                      "word:32768", "read:4116", "allocate:12",
                      "word:24576", "word:24580", "word:24584",
                      "word:24576", "word:32772"} &&
                  iterators.words[0x8000u] == 0x6000u &&
                  iterators.words[0x8004u] == 0x6000u &&
                  iterators.words[0x6000u] ==
                      pal4::ActorManagerPlayerCandidateIterator::kVtable &&
                  iterators.words[0x6004u] == 0x4000u &&
                  iterators.words[0x6008u] == kCollection + 16u,
              "candidate wrappers use distinct array bases and output slots");

        Runtime no_iterator;
        no_iterator.iterator = 0u;
        Check(pal4::ActorManagerPlayerCollection::FirstCandidateIterator(
                  kCollection, 0x8000u, no_iterator) == 0x8000u &&
                  no_iterator.words[0x8000u] == 0u &&
                  no_iterator.trace == std::vector<std::string>{
                      "read:4100", "allocate:12", "word:32768"},
              "iterator allocation failure still initializes output");

        Runtime clone;
        clone.words[0x8000u + 4u] = 0x3000u;
        clone.words[0x8000u + 8u] = kCollection;
        Check(pal4::ActorManagerPlayerCandidateIterator::Clone(
                  0x8000u, clone) == 0x6000u &&
                  clone.words[0x6000u] ==
                      pal4::ActorManagerPlayerCandidateIterator::kVtable &&
                  clone.words[0x6004u] == 0x3000u &&
                  clone.words[0x6008u] == kCollection &&
                  clone.trace == std::vector<std::string>{
                      "allocate:12", "word:24576", "read:32772",
                      "word:24580", "read:32776", "word:24584",
                      "word:24576"},
              "candidate clone allocates before reading descriptor fields");

        Runtime failed_clone;
        failed_clone.iterator = 0u;
        Check(pal4::ActorManagerPlayerCandidateIterator::Clone(
                  0x8000u, failed_clone) == 0u &&
                  failed_clone.trace == std::vector<std::string>{"allocate:12"},
              "candidate clone failure does not read descriptor");

        Runtime moving;
        moving.words[0x6004u] = 0x3000u;
        moving.words[0x6008u] = kCollection;
        moving.words[kCollection + 4u] = 0x3000u;
        moving.words[kCollection + 8u] = 0x3008u;
        Check(pal4::ActorManagerPlayerCandidateIterator::Count(
                  0x6000u, moving) == 2u &&
                  pal4::ActorManagerPlayerCandidateIterator::AtBegin(
                      0x6000u, moving) &&
                  !pal4::ActorManagerPlayerCandidateIterator::AtEnd(
                      0x6000u, moving) &&
                  pal4::ActorManagerPlayerCandidateIterator::Previous(
                      0x6000u, moving) == 1u &&
                  moving.words[0x6004u] == 0x3000u &&
                  pal4::ActorManagerPlayerCandidateIterator::Next(
                      0x6000u, moving) == 0u &&
                  moving.words[0x6004u] == 0x3004u &&
                  pal4::ActorManagerPlayerCandidateIterator::Next(
                      0x6000u, moving) == 0u &&
                  pal4::ActorManagerPlayerCandidateIterator::Next(
                      0x6000u, moving) == 1u &&
                  moving.words[0x6004u] == 0x3008u &&
                  pal4::ActorManagerPlayerCandidateIterator::Previous(
                      0x6000u, moving) == 0u &&
                  moving.words[0x6004u] == 0x3004u,
              "candidate count and bidirectional movement honor both bounds");

        Runtime empty_candidate;
        empty_candidate.words[0x6008u] = kCollection;
        Check(pal4::ActorManagerPlayerCandidateIterator::Count(
                  0x6000u, empty_candidate) == 0u &&
                  empty_candidate.trace == std::vector<std::string>{
                      "read:24584", "read:4100"},
              "candidate count skips end read when array begin is null");

        Runtime reversed_candidate;
        reversed_candidate.words[0x6008u] = kCollection;
        reversed_candidate.words[kCollection + 4u] = 0x3004u;
        reversed_candidate.words[kCollection + 8u] = 0x3000u;
        Check(pal4::ActorManagerPlayerCandidateIterator::Count(
                  0x6000u, reversed_candidate) == 0xffffffffu,
              "candidate count preserves signed arithmetic shift bits");

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            Runtime deleting;
            Check(pal4::ActorManagerPlayerCandidateIterator::Delete(
                      0x6000u, static_cast<std::uint8_t>(flags), deleting) ==
                      0x6000u &&
                      deleting.words[0x6000u] ==
                          pal4::ActorManagerPlayerCandidateIterator::kBaseVtable &&
                      deleting.trace.size() == (flags & 1u ? 2u : 1u),
                  "candidate deleting destructor honors low flag bit");
        }

        Runtime find;
        find.words[kCollection + 20u] = 0x3000u;
        find.words[kCollection + 24u] = 0x3008u;
        find.words[0x3000u] = 0x4000u;
        find.words[0x3004u] = 0x5000u;
        find.actor_values[0x4000u] = 7u;
        find.actor_values[0x5000u] = 8u;
        Check(pal4::ActorManagerPlayerCollection::FindSecondActor(
                  kCollection, 8u, find) == 0x5000u &&
                  find.trace == std::vector<std::string>{
                      "read:4116", "read:4120", "read:12288",
                      "value20:16384", "read:4116", "read:4120",
                      "read:12292", "value20:20480"},
              "second-array search returns the first matching actor");

        Runtime absent;
        absent.words[kCollection + 20u] = 0u;
        Check(pal4::ActorManagerPlayerCollection::FindSecondActor(
                  kCollection, 8u, absent) == 0u &&
                  absent.trace == std::vector<std::string>{"read:4116"},
              "second-array search stops at null begin");

        Runtime no_updates;
        Check(pal4::ActorManagerPlayerCollection::UpdateActors(
                  kCollection, 0x3f800000u, no_updates) == 0u &&
                  no_updates.trace == std::vector<std::string>{
                      "read:4116", "read:4100"},
              "empty update has defined zero result");

        Runtime updates;
        updates.words[kCollection + 20u] = 0x3000u;
        updates.words[kCollection + 24u] = 0x3004u;
        updates.words[0x3000u] = 0x4000u;
        updates.words[0x3004u] = 0x5000u;
        updates.words[kCollection + 4u] = 0x6000u;
        updates.words[kCollection + 8u] = 0x6004u;
        updates.words[0x6000u] = 0u;
        updates.words[0x6004u] = 0x7000u;
        updates.append_on_child_update = true;
        updates.append_on_animation_update = true;
        Check(pal4::ActorManagerPlayerCollection::UpdateActors(
                  kCollection, 0x3f800000u, updates) == 2u &&
                  updates.trace[3] == "child56:16384:1065353216" &&
                  updates.trace[7] == "child56:20480:1065353216" &&
                  updates.trace[13] == "animation:0:1065353216" &&
                  updates.trace[17] == "animation:29400:1065353216",
              "update visits appended second and first array entries");

        Runtime released;
        released.words[0x4000u + 816u] = 0x7000u;
        pal4::ActorManagerPlayerCollection::ReleaseActor(0x4000u, released);
        released.words[0x5000u + 816u] = 0u;
        pal4::ActorManagerPlayerCollection::ReleaseActor(0x5000u, released);
        pal4::ActorManagerPlayerCollection::ReleaseActor(0u, released);
        Check(released.trace == std::vector<std::string>{
                  "read:17200", "release:16384:28672", "delete:16384:1",
                  "read:21296", "release:20480:8657252", "delete:20480:1"},
              "release logs explicit or empty name before deleting actor");

        Runtime clear;
        clear.words[kCollection + 4u] = 0x3000u;
        clear.words[kCollection + 8u] = 0x3004u;
        clear.words[0x3000u] = 0x4000u;
        clear.words[0x3004u] = 0x5000u;
        clear.words[0x4000u + 816u] = 0x7000u;
        clear.words[0x5000u + 816u] = 0x8000u;
        clear.words[kCollection + 20u] = 0x6000u;
        clear.words[kCollection + 24u] = 0x6004u;
        clear.append_on_slot388 = true;
        Check(pal4::ActorManagerPlayerCollection::Clear(
                  kCollection, clear) == 0x6004u &&
                  clear.words[kCollection + 8u] == 0x3000u &&
                  clear.words[kCollection + 24u] == 0x6000u &&
                  std::count(clear.trace.begin(), clear.trace.end(),
                             "slot388:16384") == 1 &&
                  std::count(clear.trace.begin(), clear.trace.end(),
                             "slot388:20480") == 1 &&
                  std::count(clear.trace.begin(), clear.trace.end(),
                             "delete:16384:1") == 1 &&
                  std::count(clear.trace.begin(), clear.trace.end(),
                             "delete:20480:1") == 1,
              "clear visits grown first array, releases actors and resets ends");

        Runtime events;
        pal4::ActorManagerPlayerCollection::RegisterEventHandlers(
            kCollection, events);
        Check(events.trace == std::vector<std::string>{
                  "router", "register:36864:2:re_join_stage:4096:5454160",
                  "router", "register:36864:2:re_leave_stage:4096:5485952",
                  "router", "register:36864:2:combat_over:4096:5748944"},
              "player handlers register three ordered events");

        Runtime empty_leave;
        empty_leave.words[kCollection + 20u] = 0x3000u;
        empty_leave.words[kCollection + 24u] = 0x3000u;
        Check(pal4::ActorManagerPlayerCollection::OnLeaveStage(
                  kCollection, 0x9000u, empty_leave) == 0x3000u &&
                  empty_leave.dialog_calls == 0u &&
                  empty_leave.trace == std::vector<std::string>{
                      "read:4116", "read:4120"},
              "empty leave-stage array skips parameter decoding");

        Runtime leave;
        leave.words[kCollection + 20u] = 0x3000u;
        leave.words[kCollection + 24u] = 0x300cu;
        leave.words[0x3000u] = 3u;
        leave.words[0x3004u] = 7u;
        leave.words[0x3008u] = 9u;
        leave.dialog_value = 7u;
        Check(pal4::ActorManagerPlayerCollection::OnLeaveStage(
                  kCollection, 0x9000u, leave) == 0x300cu &&
                  leave.dialog_calls == 2u &&
                  leave.words[0x3000u] == 3u &&
                  leave.words[0x3004u] == 9u &&
                  leave.words[kCollection + 24u] == 0x3008u &&
                  leave.trace == std::vector<std::string>{
                      "read:4116", "read:4120", "dialog:36864",
                      "read:12288", "read:4120", "dialog:36864",
                      "read:12292", "read:4120", "read:12296",
                      "word:12292", "word:4120"},
              "leave stage removes matching middle value and returns old end");

        Runtime no_match;
        no_match.words[kCollection + 20u] = 0x3000u;
        no_match.words[kCollection + 24u] = 0x3008u;
        no_match.words[0x3000u] = 3u;
        no_match.words[0x3004u] = 7u;
        no_match.dialog_value = 8u;
        Check(pal4::ActorManagerPlayerCollection::OnLeaveStage(
                  kCollection, 0x9000u, no_match) == 0x3008u &&
                  no_match.dialog_calls == 2u &&
                  no_match.words[kCollection + 24u] == 0x3008u &&
                  no_match.words[0x3004u] == 7u,
              "leave stage preserves unmatched values and end pointer");

        Runtime last_match;
        last_match.words[kCollection + 20u] = 0x3000u;
        last_match.words[kCollection + 24u] = 0x3008u;
        last_match.words[0x3000u] = 3u;
        last_match.words[0x3004u] = 7u;
        last_match.dialog_value = 7u;
        Check(pal4::ActorManagerPlayerCollection::OnLeaveStage(
                  kCollection, 0x9000u, last_match) == 0x3008u &&
                  last_match.words[kCollection + 24u] == 0x3004u &&
                  last_match.words[0x3000u] == 3u,
              "leave stage removes final matching value without copying");
        std::cout << "actor_manager_player_collection_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
