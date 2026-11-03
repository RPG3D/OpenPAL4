#include "pal4/actor_manager_target_selection_controller.h"
#include "pal4/world_manager_frame_updater.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t kController = 0x1000u;
constexpr std::uint32_t kGroup = 0xc000u;
constexpr std::uint32_t kHead = 0xd000u;
constexpr std::uint32_t kNode = 0xe000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Subscription {
    std::uint32_t mode;
    std::uint32_t key;
    std::uint32_t owner;
    std::uint32_t callback;
};

struct Runtime final : pal4::IActorManagerTargetSelectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::string, std::uint32_t> keys{
        {"start_select_target", 0x9000u},
        {"stop_select_target", 0x9004u},
        {"close_system", 0x9008u},
        {"combat_over", 0x900cu},
        {"combat_prepare", 0x9010u},
    };
    std::map<std::uint32_t, std::uint32_t> groups;
    std::vector<std::string> trace;
    std::vector<Subscription> subscriptions;
    std::uint32_t inactive = 0u;
    std::uint32_t active = 1u;
    std::uint32_t simple_result = 0x55u;
    bool mutate_payload_on_dispatch = false;
    bool disable_on_simple_dispatch = false;
    bool throw_on_iterator_next = false;
    bool throw_on_selection_clone = false;
    std::uint32_t preferred_iterator = 0x20000u;
    std::uint32_t fallback_iterator = 0x20004u;
    std::uint32_t selection_iterator = 0x20008u;
    bool selection_has_data = false;
    bool selection_record_present = true;
    std::uint32_t selection_target_type = 2u;
    std::vector<std::uint32_t> preferred_actors;
    std::vector<std::uint32_t> fallback_actors;
    std::vector<std::uint32_t> selection_actors;
    std::map<std::uint32_t, std::size_t> iterator_positions;
    std::map<std::uint32_t, std::uint32_t> radii;
    std::map<std::uint32_t, std::uint32_t> helper_results;
    std::map<std::uint32_t, double> distances;
    std::map<std::uint32_t, bool> target_states;
    std::array<std::uint32_t, 4> dispatched_words{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        trace.emplace_back("read-byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.emplace_back("word:" + std::to_string(address) +
                           ":" + std::to_string(value));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        trace.emplace_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void InitializeSelectionData(std::uint32_t address,
                                 std::uint32_t value) override {
        trace.emplace_back("selection-data:" + std::to_string(address) +
                           ":" + std::to_string(value));
        words[address] = value;
        words[address + 4u] = 0u;
        words[address + 8u] = 0u;
        words[address + 12u] = 6u;
    }
    std::uint32_t EventKey(std::string_view name) override {
        trace.emplace_back("key:" + std::string(name));
        return keys.at(std::string(name));
    }
    std::uint32_t EventRouter() override {
        trace.emplace_back("router");
        return 0xb000u;
    }
    std::uint32_t Subscribe(std::uint32_t router, std::uint32_t mode,
                            std::uint32_t key, std::uint32_t owner,
                            std::uint32_t callback) override {
        Check(router == 0xb000u, "subscription router");
        trace.emplace_back("subscribe:" + std::to_string(key));
        subscriptions.push_back({mode, key, owner, callback});
        return 0xa000u + 4u *
            static_cast<std::uint32_t>(subscriptions.size() - 1u);
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0xb000u, "lookup router");
        trace.emplace_back("group:" + std::to_string(key));
        return groups[key];
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        trace.emplace_back("release:" + std::to_string(slot));
    }
    void FreeNode(std::uint32_t node) override {
        trace.emplace_back("free:" + std::to_string(node));
    }
    std::uint32_t CombatSystem() override {
        trace.emplace_back("combat-system");
        return 0xf000u;
    }
    std::uint32_t CombatInactive(std::uint32_t combat) override {
        Check(combat == 0xf000u, "combat inactivity owner");
        trace.emplace_back("combat-inactive");
        return inactive;
    }
    std::uint32_t CombatActive(std::uint32_t combat) override {
        Check(combat == 0xf000u, "combat active owner");
        trace.emplace_back("combat-active");
        return active;
    }
    std::uint32_t InputManager() override {
        trace.emplace_back("input-manager");
        return 0x10000u;
    }
    std::uint32_t DispatchAddress(std::uint32_t router,
                                  std::string_view event,
                                  std::uint32_t payload,
                                  std::uint32_t mode) override {
        Check(router == 0xb000u && mode == 1u, "address event router");
        trace.emplace_back("dispatch-address:" + std::string(event) +
                           ":" + std::to_string(payload));
        if (mutate_payload_on_dispatch) words[kController + 44u] = 99u;
        return 0x44u;
    }
    std::uint32_t DispatchWords(
        std::uint32_t router, std::string_view event,
        const std::array<std::uint32_t, 4>& payload,
        std::uint32_t mode) override {
        Check(router == 0xb000u && mode == 1u, "word event router");
        trace.emplace_back("dispatch-words:" + std::string(event));
        dispatched_words = payload;
        return 0x44u;
    }
    std::uint32_t DispatchSimple(std::uint32_t router,
                                 std::string_view event,
                                 std::uint8_t payload,
                                 std::uint32_t mode) override {
        Check(router == 0xb000u && mode == 1u && payload == 0u,
              "simple event payload");
        trace.emplace_back("dispatch-simple:" + std::string(event));
        if (disable_on_simple_dispatch) bytes[kController + 82u] = 0u;
        return simple_result;
    }
    void CheckTargetState(std::uint32_t target) override {
        trace.emplace_back("check-target:" + std::to_string(target));
    }
    std::uint32_t DispatchActor(std::uint32_t router,
                                std::string_view event,
                                std::uint32_t actor,
                                std::uint32_t mode) override {
        Check(router == 0xb000u && mode == 1u,
              "actor event router");
        trace.emplace_back("dispatch-actor:" + std::string(event) +
                           ":" + std::to_string(actor));
        return 0x44u;
    }
    std::uint32_t GameDatabase() override {
        trace.emplace_back("game-database");
        return 0x30000u;
    }
    std::uint32_t FindSelectionRecord(
        std::uint32_t database, std::uint32_t key,
        pal4::SelectionRecordKind kind) override {
        trace.emplace_back("find-record:" + std::to_string(database) +
                           ":" + std::to_string(key) + ":" +
                           std::to_string(static_cast<unsigned>(kind)));
        return selection_record_present ? 0x60000u : 0u;
    }
    std::uint32_t RecordTargetType(
        std::uint32_t record, pal4::SelectionRecordKind kind) override {
        Check(record == 0x60000u, "target type record");
        trace.emplace_back("record-type:" +
                           std::to_string(static_cast<unsigned>(kind)));
        return selection_target_type;
    }
    void ReportSelectionError(std::string_view kind,
                              std::uint32_t actor_name,
                              std::uint32_t value) override {
        trace.emplace_back("selection-error:" + std::string(kind) + ":" +
                           std::to_string(actor_name) + ":" +
                           std::to_string(value));
    }
    void ReportTargetTypeError(bool known_type,
                               std::uint32_t value) override {
        trace.emplace_back("type-error:" +
                           std::to_string(known_type ? 1u : 0u) + ":" +
                           std::to_string(value));
    }
    std::uint32_t SelectionDialog(std::uint32_t parameter) override {
        Check(parameter == kController + 40u, "selection dialog parameter");
        trace.emplace_back("selection-dialog");
        return 0x40000u;
    }
    std::uint32_t CreateSelectionIterator(std::uint32_t dialog,
                                          bool alternate) override {
        Check(dialog == 0x40000u, "selection iterator dialog");
        trace.emplace_back(alternate ? "selection-alternate" :
                                       "selection-primary");
        return 0x50000u;
    }
    std::uint32_t CloneSelectionIterator(
        std::uint32_t temporary) override {
        Check(temporary == 0x50000u, "selection clone source");
        trace.emplace_back("selection-clone");
        if (throw_on_selection_clone)
            throw std::runtime_error("selection clone failed");
        return selection_iterator;
    }
    void DeleteTemporaryIterator(std::uint32_t temporary,
                                 std::uint32_t flags) override {
        Check(temporary == 0x50000u && flags == 1u,
              "selection temporary deletion");
        trace.emplace_back("selection-temp-delete");
    }
    bool IteratorHasData(std::uint32_t iterator) override {
        Check(iterator == selection_iterator, "selection data iterator");
        trace.emplace_back("iterator-has-data");
        return selection_has_data;
    }
    std::uint32_t InitializeInputState(std::uint32_t input) override {
        Check(input == 0x10000u, "input initialization owner");
        trace.emplace_back("initialize-input");
        return 0x66u;
    }
    std::uint32_t SetTargetSelected(std::uint32_t target,
                                    std::uint32_t selected) override {
        trace.emplace_back("selected:" + std::to_string(target) +
                           ":" + std::to_string(selected));
        return selected == 0u ? 0x88u : 0x99u;
    }
    std::uint32_t GameConfigWord(std::uint32_t index) override {
        trace.emplace_back("config:" + std::to_string(index));
        return index == 13u ? 0x2222u : 0x1111u;
    }
    std::uint32_t ActorRadius(std::uint32_t actor) override {
        trace.emplace_back("radius:" + std::to_string(actor));
        return radii[actor];
    }
    std::uint32_t ActorManagerHelper(std::uint32_t context,
                                     std::uint32_t first,
                                     std::uint32_t second,
                                     std::uint32_t radius) override {
        Check(first == 0x1111u && second == 0x2222u,
              "helper config arguments");
        trace.emplace_back("helper:" + std::to_string(context) +
                           ":" + std::to_string(radius));
        return helper_results[radius];
    }
    double DistanceSquared(std::uint32_t projected,
                           std::uint32_t reference) override {
        trace.emplace_back("distance:" + std::to_string(projected) +
                           ":" + std::to_string(reference));
        return distances[projected];
    }
    std::uint32_t ActorManagerSingleton() override {
        trace.emplace_back("actor-manager");
        return 0x30000u;
    }
    std::uint32_t PreferredIterator(std::uint32_t manager) override {
        Check(manager == 0x30000u, "preferred iterator manager");
        trace.emplace_back("preferred-iterator");
        return preferred_iterator;
    }
    std::uint32_t FallbackIterator(
        std::uint32_t player_collection) override {
        Check(player_collection == 0x30020u,
              "fallback iterator collection");
        trace.emplace_back("fallback-iterator");
        return fallback_iterator;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        trace.emplace_back("at-end:" + std::to_string(iterator));
        const auto& actors = iterator == preferred_iterator
            ? preferred_actors : iterator == fallback_iterator
                ? fallback_actors : selection_actors;
        return iterator_positions[iterator] >= actors.size();
    }
    std::uint32_t IteratorValue(std::uint32_t iterator) override {
        trace.emplace_back("iterator-value:" + std::to_string(iterator));
        const auto& actors = iterator == preferred_iterator
            ? preferred_actors : iterator == fallback_iterator
                ? fallback_actors : selection_actors;
        return actors.at(iterator_positions[iterator]);
    }
    void IteratorNext(std::uint32_t iterator) override {
        trace.emplace_back("iterator-next:" + std::to_string(iterator));
        if (throw_on_iterator_next)
            throw std::runtime_error("iterator advance failed");
        ++iterator_positions[iterator];
    }
    std::uint32_t DeleteIterator(std::uint32_t iterator,
                                 std::uint32_t flags) override {
        trace.emplace_back("iterator-delete:" + std::to_string(iterator) +
                           ":" + std::to_string(flags));
        return iterator;
    }
    bool TargetStateNotThree(std::uint32_t target) override {
        trace.emplace_back("state-not-three:" + std::to_string(target));
        return target_states[target];
    }
};

void SetOneSubscription(Runtime& runtime, std::uint32_t handle,
                        std::uint32_t key) {
    runtime.words[kController] = handle;
    runtime.words[kController + 4u] = key;
    runtime.groups[key] = kGroup;
    runtime.words[kGroup + 4u] = kHead;
    runtime.words[kGroup + 8u] = 1u;
    runtime.words[kHead] = kNode;
    runtime.words[kHead + 4u] = kNode;
    runtime.words[kNode] = kHead;
    runtime.words[kNode + 4u] = kHead;
    runtime.words[kNode + 12u] = handle;
}

void ConstructionAndRegistration() {
    Runtime runtime;
    runtime.bytes[kController + 83u] = 0xaau;
    Check(pal4::ActorManagerTargetSelectionController::Construct(
              kController, runtime) == kController &&
              runtime.trace[0] == "word:4096:0" &&
              runtime.trace[9] == "word:4132:0" &&
              runtime.trace[10] == "selection-data:4136:0" &&
              runtime.trace[17] == "byte:4176" &&
              runtime.words[kController + 40u] == 0u &&
              runtime.words[kController + 52u] == 6u &&
              runtime.words[kController + 56u] == 0u &&
              runtime.words[kController + 72u] == 2u &&
              runtime.words[kController + 76u] == 0u &&
              runtime.bytes[kController + 80u] == 0u &&
              runtime.bytes[kController + 82u] == 0u &&
              runtime.bytes[kController + 83u] == 0xaau,
          "constructor initializes fields before subscribing");

    Check(runtime.subscriptions.size() == 5u &&
              runtime.subscriptions[0].mode == 2u &&
              runtime.subscriptions[0].key == 0x9000u &&
              runtime.subscriptions[0].callback == 0x53bb80u &&
              runtime.subscriptions[1].mode == 2u &&
              runtime.subscriptions[1].callback == 0x53bbd0u &&
              runtime.subscriptions[2].mode == 3u &&
              runtime.subscriptions[3].mode == 3u &&
              runtime.subscriptions[4].mode == 3u &&
              runtime.subscriptions[4].key == 0x9010u &&
              runtime.subscriptions[4].callback == 0x53bbd0u &&
              std::all_of(runtime.subscriptions.begin(),
                          runtime.subscriptions.end(),
                          [](const Subscription& entry) {
                              return entry.owner == kController;
                          }) &&
              runtime.words[kController + 32u] == 0xa010u &&
              runtime.words[kController + 36u] == 0x9010u,
          "registration stores five ordered modes, keys and callbacks");

    const auto first_key = std::find(runtime.trace.begin(), runtime.trace.end(),
                                     "key:start_select_target");
    Check(first_key != runtime.trace.end() &&
              std::count(runtime.trace.begin(), first_key, "read:4096") == 1 &&
              std::count(runtime.trace.begin(), first_key, "read:4128") == 1,
          "registration checks every old handle before first new event");
}

void UnregisterAndReplace() {
    Runtime runtime;
    SetOneSubscription(runtime, 0xa000u, 0x9000u);
    runtime.words[kController + 12u] = 0x9004u;
    pal4::ActorManagerTargetSelectionController::UnsubscribeEvents(
        kController, runtime);
    Check(runtime.words[kHead] == kHead &&
              runtime.words[kHead + 4u] == kHead &&
              runtime.words[kGroup + 8u] == 0u &&
              runtime.words[kController] == 0u &&
              runtime.words[kController + 4u] == 0u &&
              runtime.words[kController + 12u] == 0x9004u &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "release:57352") != runtime.trace.end() &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "free:57344") != runtime.trace.end(),
          "unsubscribe unlinks matching node and keeps key of empty handle");

    Runtime missing;
    SetOneSubscription(missing, 0xa000u, 0x9000u);
    missing.words[kNode + 12u] = 0xa100u;
    pal4::ActorManagerTargetSelectionController::UnsubscribeEvents(
        kController, missing);
    Check(missing.words[kHead] == kNode &&
              missing.words[kGroup + 8u] == 1u &&
              missing.words[kController] == 0u &&
              missing.words[kController + 4u] == 0u &&
              std::none_of(missing.trace.begin(), missing.trace.end(),
                           [](const std::string& item) {
                               return item.rfind("release:", 0u) == 0u ||
                                      item.rfind("free:", 0u) == 0u;
                           }),
          "missing handler clears pair without unlinking another node");

    Runtime replaced;
    SetOneSubscription(replaced, 0xa100u, 0x9000u);
    Check(pal4::ActorManagerTargetSelectionController::SubscribeEvents(
              kController, replaced) == 0xa010u &&
              replaced.subscriptions.size() == 5u &&
              std::find(replaced.trace.begin(), replaced.trace.end(),
                        "free:57344") <
                  std::find(replaced.trace.begin(), replaced.trace.end(),
                            "subscribe:36864"),
          "resubscription removes old callback before registering new ones");
}

void ResetHelpers() {
    Runtime pair;
    Check(pal4::ActorManagerTargetSelectionController::ResetTargetPair(
              kController, pair) == 0u &&
              pair.trace == std::vector<std::string>{
                  "word:4152:0", "word:4156:0"},
          "target pair resets offsets 56 and 60");

    Runtime first;
    Check(pal4::ActorManagerTargetSelectionController::ResetCommandStateFirst(
              kController, first) == 0u &&
              first.trace == std::vector<std::string>{
                  "word:4140:0", "word:4160:0", "word:4164:0"},
          "first command reset writes selected target before later fields");

    Runtime second;
    Check(pal4::ActorManagerTargetSelectionController::ResetCommandStateSecond(
              kController, second) == 0u &&
              second.trace == std::vector<std::string>{
                  "word:4160:0", "word:4140:0", "word:4164:0"},
          "second command reset writes later field before selected target");
}

void SelectionCallbacks() {
    Runtime start;
    Check(pal4::ActorManagerTargetSelectionController::StartSelecting(
              kController, {1u, 2u, 3u, 4u}, start) == 0x66u &&
              start.trace == std::vector<std::string>{
                  "word:4136:1", "word:4140:2", "word:4144:3",
                  "byte:4178", "word:4148:4", "word:4164:0",
                  "byte:4176", "selection-dialog", "read:4148",
                  "word:4168:1",
                  "selection-dialog", "read:4168",
                  "selection-alternate", "selection-clone",
                  "selection-temp-delete", "iterator-has-data",
                  "iterator-delete:131080:1", "input-manager",
                  "initialize-input"} &&
              start.bytes[kController + 82u] == 1u &&
              start.words[kController + 72u] == 1u,
          "start selection stores payload and prepares input in order");

    Runtime stop;
    stop.words[0xf000u + 508u] = 6u;
    stop.words[kController + 64u] = 0x7000u;
    stop.words[kController + 44u] = 0x8000u;
    stop.bytes[kController + 82u] = 1u;
    Check(pal4::ActorManagerTargetSelectionController::StopSelecting(
              kController, {1u, 2u, 3u, 4u}, stop) == 0x55u &&
              stop.trace == std::vector<std::string>{
                  "combat-system", "read:61948", "read:4160",
                  "selected:28672:0", "router",
                  "dispatch-simple:hide_monster_tips", "byte:4178",
                  "word:4140:0", "word:4160:0", "word:4164:0"} &&
              stop.words[kController + 64u] == 0u &&
              stop.bytes[kController + 82u] == 0u,
          "stop selection unselects before hide event and clears state");

    Runtime mode_seven;
    mode_seven.words[0xf000u + 508u] = 7u;
    mode_seven.words[kController + 64u] = 0x7000u;
    Check(pal4::ActorManagerTargetSelectionController::StopSelecting(
              kController, {}, mode_seven) == 0x55u &&
              std::find(mode_seven.trace.begin(), mode_seven.trace.end(),
                        "selected:28672:0") == mode_seven.trace.end(),
          "combat mode seven skips target unselection");
}

void InputPaths() {
    Runtime inactive;
    inactive.inactive = 0x34u;
    Check(pal4::ActorManagerTargetSelectionController::ProcessInput(
              kController, 0.1f, inactive) == 0x34u &&
              inactive.trace == std::vector<std::string>{
                  "combat-system", "combat-inactive"},
          "inactive combat stops before second singleton lookup");

    Runtime inactive_state;
    inactive_state.active = 0u;
    Check(pal4::ActorManagerTargetSelectionController::ProcessInput(
              kController, 0.1f, inactive_state) == 0u &&
              inactive_state.trace == std::vector<std::string>{
                  "combat-system", "combat-inactive",
                  "combat-system", "combat-active"},
          "inactive combat state stops before input lookup");

    Runtime start;
    start.words[0x10000u + 0x21e4u] = 1u;
    start.words[0x10000u + 0x21d8u] = 0x11u;
    start.words[kController + 64u] = 0x7000u;
    start.words[kController + 40u] = 1u;
    start.words[kController + 44u] = 2u;
    start.words[kController + 48u] = 3u;
    start.words[kController + 52u] = 4u;
    start.words[0xf000u + 508u] = 6u;
    start.mutate_payload_on_dispatch = true;
    Check(pal4::ActorManagerTargetSelectionController::ProcessInput(
              kController, 0.1f, start) == 0x55u &&
              start.words[kController + 64u] == 0u &&
              start.words[kController + 76u] == 0u &&
              std::find(start.trace.begin(), start.trace.end(),
                        "dispatch-address:re_start_ui_control:4136") <
                  std::find(start.trace.begin(), start.trace.end(),
                            "read:4140") &&
              std::find(start.trace.begin(), start.trace.end(),
                        "dispatch-simple:hide_monster_tips") <
                  std::find(start.trace.begin(), start.trace.end(),
                            "word:4160:0"),
          "start input dispatches live payload then stops without timer update");

    Runtime start_without_target;
    start_without_target.words[0x10000u + 0x21e4u] = 1u;
    start_without_target.words[0x10000u + 0x21d8u] = 1u;
    Check(pal4::ActorManagerTargetSelectionController::ProcessInput(
              kController, 0.1f, start_without_target) == 0u &&
              std::find(start_without_target.trace.begin(),
                        start_without_target.trace.end(),
                        "dispatch-address:re_start_ui_control:4136") ==
                  start_without_target.trace.end(),
          "start input with no target exits without selection or timer");

    Runtime select;
    select.words[0x10000u + 0x21d4u] = 1u;
    select.words[0x10000u + 0x21c8u] = 0x21u;
    select.words[kController + 64u] = 0x7000u;
    select.words[kController + 40u] = 1u;
    select.words[kController + 48u] = 3u;
    select.words[kController + 52u] = 4u;
    select.words[kController + 76u] = std::bit_cast<std::uint32_t>(0.2f);
    Check(pal4::ActorManagerTargetSelectionController::ProcessInput(
              kController, 0.05f, select) == 0x100u &&
              select.words[kController + 44u] == 0x7000u &&
              select.dispatched_words ==
                  std::array<std::uint32_t, 4>{1u, 0x7000u, 3u, 4u} &&
              std::bit_cast<float>(select.words[kController + 76u]) == 0.25f &&
              std::find(select.trace.begin(), select.trace.end(),
                        "dispatch-words:re_select_command_target") <
                  std::find(select.trace.begin(), select.trace.end(),
                            "check-target:28672"),
          "select input stores target, dispatches snapshot and advances timer");

    Runtime timer;
    timer.words[kController + 76u] = std::bit_cast<std::uint32_t>(0.25f);
    Check(pal4::ActorManagerTargetSelectionController::ProcessInput(
              kController, 0.1f, timer) == timer.preferred_iterator &&
              timer.trace.back() == "iterator-delete:131072:1" &&
              timer.words[kController + 76u] == 0u,
          "timer strictly above threshold searches closest actor");

    Runtime equal;
    equal.words[kController + 76u] = std::bit_cast<std::uint32_t>(0.3f);
    Check(pal4::ActorManagerTargetSelectionController::ProcessInput(
              kController, 0.0f, equal) == 0x4000u &&
              std::find(equal.trace.begin(), equal.trace.end(),
                        "preferred-iterator") == equal.trace.end(),
          "timer equal to stored float threshold does not search");

    Runtime unordered;
    Check(pal4::ActorManagerTargetSelectionController::ProcessInput(
              kController, std::numeric_limits<float>::quiet_NaN(),
              unordered) == 0x4100u,
          "unordered timer forwards masked x87 comparison flags");
}

void TargetUpdateAndSwitch() {
    using Controller = pal4::ActorManagerTargetSelectionController;

    Runtime disabled;
    Check(Controller::UpdateTarget(kController, 7u, disabled) == 1u &&
              disabled.trace == std::vector<std::string>{
                  "read-byte:4178"},
          "disabled target update has no external effects");

    Runtime current;
    current.bytes[kController + 82u] = 1u;
    current.words[kController + 68u] = 0x7000u;
    current.words[kController + 56u] = 0x8800u;
    current.words[kController + 60u] = 0x9000u;
    current.words[kController + 64u] = 0x6000u;
    current.words[7u] = 0x1111u;
    current.words[11u] = 0x2222u;
    current.words[0x9000u] = 0xb000u;
    current.radii[0x7000u] = 5u;
    current.radii[0x8000u] = 6u;
    current.helper_results[5u] = true;
    current.helper_results[6u] = 0xa000u;
    current.distances[0xa000u] = 2.5;
    current.selection_actors = {0x8000u};
    Check(Controller::UpdateTarget(kController, 7u, current) == 1u &&
              current.words[kController + 64u] == 0x8000u &&
              current.words[kController + 68u] == 0x7000u &&
              std::find(current.trace.begin(), current.trace.end(),
                        "helper:36864:5") <
                  std::find(current.trace.begin(), current.trace.end(),
                            "distance:40960:45056") &&
              std::find(current.trace.begin(), current.trace.end(),
                        "iterator-delete:131080:1") <
                  std::find(current.trace.begin(), current.trace.end(),
                            "selected:24576:0") &&
              current.trace.back() == "selected:32768:1",
          "valid current tips actor skips scans and switches active actor");

    Runtime hidden;
    hidden.bytes[kController + 82u] = 1u;
    hidden.words[kController + 68u] = 0x7000u;
    hidden.radii[0x7000u] = 5u;
    hidden.disable_on_simple_dispatch = true;
    Check(Controller::UpdateTarget(kController, 7u, hidden) == 1u &&
              hidden.words[kController + 68u] == 0u &&
              std::find(hidden.trace.begin(), hidden.trace.end(),
                        "dispatch-simple:hide_monster_tips") <
                  std::find(hidden.trace.begin(), hidden.trace.end(),
                            "word:4164:0") &&
              std::find(hidden.trace.begin(), hidden.trace.end(),
                        "read:4152") == hidden.trace.end(),
          "failed tips gate hides actor and respects callback-disabled state");

    Runtime preferred;
    preferred.bytes[kController + 82u] = 1u;
    preferred.bytes[kController + 80u] = 1u;
    preferred.preferred_actors = {0x7000u, 0x7004u};
    preferred.target_states[0x7000u] = false;
    preferred.target_states[0x7004u] = true;
    preferred.radii[0x7004u] = 9u;
    preferred.helper_results[9u] = true;
    preferred.words[kController + 76u] = 0x1234u;
    Check(Controller::UpdateTarget(kController, 8u, preferred) == 1u &&
              preferred.words[kController + 76u] == 0x1234u &&
              std::find(preferred.trace.begin(), preferred.trace.end(),
                        "state-not-three:28672") !=
                  preferred.trace.end() &&
              std::find(preferred.trace.begin(), preferred.trace.end(),
                        "radius:28672") == preferred.trace.end() &&
              std::find(preferred.trace.begin(), preferred.trace.end(),
                        "iterator-delete:131072:1") !=
                  preferred.trace.end() &&
              std::find(preferred.trace.begin(), preferred.trace.end(),
                        "fallback-iterator") == preferred.trace.end(),
          "preferred scan rejects state gate then accepts next actor");

    Runtime fallback;
    fallback.bytes[kController + 82u] = 1u;
    fallback.fallback_actors = {0x8000u};
    fallback.radii[0x8000u] = 9u;
    fallback.helper_results[9u] = true;
    fallback.words[kController + 76u] = 0x1234u;
    Check(Controller::UpdateTarget(kController, 8u, fallback) == 1u &&
              fallback.words[kController + 76u] == 0x1234u &&
              std::find(fallback.trace.begin(), fallback.trace.end(),
                        "iterator-delete:131072:1") <
                  std::find(fallback.trace.begin(), fallback.trace.end(),
                            "fallback-iterator") &&
              std::find(fallback.trace.begin(), fallback.trace.end(),
                        "iterator-delete:131076:1") !=
                  fallback.trace.end(),
          "fallback scan runs after empty preferred scan and preserves timer");

    Runtime exhausted;
    exhausted.bytes[kController + 82u] = 1u;
    exhausted.words[kController + 76u] = 0x1234u;
    Check(Controller::UpdateTarget(kController, 8u, exhausted) == 1u &&
              exhausted.words[kController + 76u] == 0u &&
              std::find(exhausted.trace.begin(), exhausted.trace.end(),
                        "word:4172:0") != exhausted.trace.end(),
          "two exhausted scans reset target search timer");

    Runtime throwing;
    throwing.bytes[kController + 82u] = 1u;
    throwing.preferred_actors = {0x7000u};
    throwing.throw_on_iterator_next = true;
    bool caught = false;
    try {
        static_cast<void>(Controller::UpdateTarget(kController, 8u,
                                                   throwing));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.trace.back() ==
                        "iterator-delete:131072:1",
          "iterator is deleted when candidate traversal throws");

    Runtime state_gate;
    Check(Controller::CandidateAllowed(kController, 0x7000u,
                                       state_gate) &&
              state_gate.trace == std::vector<std::string>{
                  "read-byte:4176"},
          "disabled candidate gate passes without target-state query");
    state_gate.trace.clear();
    state_gate.bytes[kController + 80u] = 1u;
    state_gate.target_states[0x7000u] = false;
    Check(!Controller::CandidateAllowed(kController, 0x7000u,
                                        state_gate) &&
              state_gate.trace == std::vector<std::string>{
                  "read-byte:4176", "state-not-three:28672"},
          "enabled candidate gate forwards target state");

    Runtime no_actor;
    Check(Controller::SwitchActiveActor(kController, 0u, no_actor) == 0u &&
              no_actor.trace.empty(),
          "null active-actor switch has defined zero result");
    Runtime same_actor;
    same_actor.words[kController + 64u] = 0x7000u;
    Check(Controller::SwitchActiveActor(kController, 0x7000u,
                                        same_actor) == 0u &&
              same_actor.trace == std::vector<std::string>{"read:4160"},
          "same actor is left selected without callbacks");
    Runtime switch_actor;
    switch_actor.words[kController + 64u] = 0x7000u;
    Check(Controller::SwitchActiveActor(kController, 0x8000u,
                                        switch_actor) == 0x99u &&
              switch_actor.trace == std::vector<std::string>{
                  "read:4160", "selected:28672:0",
                  "word:4160:32768", "selected:32768:1"},
          "actor switch unselects previous before storing and selecting new");
}

void SelectionRefresh() {
    using Controller = pal4::ActorManagerTargetSelectionController;

    Runtime mode_two;
    mode_two.words[kController + 72u] = 2u;
    Check(Controller::RefreshSelection(kController, mode_two) == 2u &&
              mode_two.trace == std::vector<std::string>{
                  "selection-dialog", "read:4168"},
          "mode two obtains dialog before skipping iterator creation");

    Runtime primary;
    primary.words[kController + 72u] = 0u;
    Check(Controller::RefreshSelection(kController, primary) == 0x20008u &&
              primary.trace == std::vector<std::string>{
                  "selection-dialog", "read:4168",
                  "selection-primary", "selection-clone",
                  "selection-temp-delete", "iterator-has-data",
                  "iterator-delete:131080:1"},
          "primary iterator clone releases temporary before empty heap clone");

    Runtime alternate;
    alternate.words[kController + 72u] = 1u;
    alternate.words[kController + 64u] = 0x6000u;
    alternate.bytes[kController + 80u] = 1u;
    alternate.selection_has_data = true;
    alternate.selection_actors = {0x7000u, 0x8000u};
    alternate.target_states[0x7000u] = false;
    alternate.target_states[0x8000u] = true;
    Check(Controller::RefreshSelection(kController, alternate) == 0x20008u &&
              alternate.words[kController + 64u] == 0x8000u &&
              std::find(alternate.trace.begin(), alternate.trace.end(),
                        "selection-alternate") != alternate.trace.end() &&
              std::find(alternate.trace.begin(), alternate.trace.end(),
                        "iterator-next:131080") != alternate.trace.end() &&
              std::find(alternate.trace.begin(), alternate.trace.end(),
                        "selected:24576:0") <
                  std::find(alternate.trace.begin(), alternate.trace.end(),
                            "selected:32768:1") &&
              alternate.trace.back() == "iterator-delete:131080:1",
          "alternate refresh skips rejected actor then switches and cleans up");

    Runtime clone_failure;
    clone_failure.selection_iterator = 0u;
    Check(Controller::RefreshSelection(kController, clone_failure) == 0u &&
              clone_failure.trace.back() == "selection-temp-delete" &&
              std::find(clone_failure.trace.begin(),
                        clone_failure.trace.end(),
                        "iterator-has-data") == clone_failure.trace.end(),
          "clone allocation failure still deletes temporary iterator");

    Runtime clone_exception;
    clone_exception.throw_on_selection_clone = true;
    bool clone_caught = false;
    try {
        static_cast<void>(Controller::RefreshSelection(
            kController, clone_exception));
    } catch (const std::runtime_error&) {
        clone_caught = true;
    }
    Check(clone_caught &&
              clone_exception.trace.back() == "selection-temp-delete",
          "temporary iterator is deleted when cloning throws");

    Runtime traversal_exception;
    traversal_exception.selection_has_data = true;
    traversal_exception.selection_actors = {0x7000u};
    traversal_exception.bytes[kController + 80u] = 1u;
    traversal_exception.throw_on_iterator_next = true;
    bool traversal_caught = false;
    try {
        static_cast<void>(Controller::RefreshSelection(
            kController, traversal_exception));
    } catch (const std::runtime_error&) {
        traversal_caught = true;
    }
    Check(traversal_caught &&
              traversal_exception.trace.back() ==
                  "iterator-delete:131080:1",
          "heap iterator is deleted when candidate traversal throws");
}

void TargetModeAndType() {
    using Controller = pal4::ActorManagerTargetSelectionController;

    Runtime direct;
    direct.words[kController + 52u] = 4u;
    Check(Controller::DetermineTargetMode(kController, direct) == 1u &&
              direct.trace == std::vector<std::string>{
                  "byte:4176", "selection-dialog", "read:4148"} &&
              direct.bytes[kController + 80u] == 1u,
          "command four enables the state gate without a database lookup");

    struct Case {
        std::uint32_t command;
        std::uint32_t database;
        std::uint32_t status_offset;
        std::uint32_t kind;
    };
    for (const auto test : {Case{0u, 0x3000cu, 8u, 0u},
                            Case{1u, 0x3000cu, 4u, 1u},
                            Case{2u, 0x30000u, 8u, 2u}}) {
        for (const auto status : {4u, 5u}) {
            Runtime runtime;
            runtime.words[kController + 52u] = test.command;
            runtime.words[kController + 48u] = 0x1234u;
            runtime.words[0x60000u + test.status_offset] = status;
            runtime.selection_target_type = 1u;
            Check(Controller::DetermineTargetMode(kController, runtime) == 0u &&
                      runtime.bytes[kController + 80u] ==
                          (status == 5u ? 0u : 1u) &&
                      std::find(runtime.trace.begin(), runtime.trace.end(),
                                "find-record:" + std::to_string(test.database) +
                                ":4660:" + std::to_string(test.kind)) !=
                          runtime.trace.end() &&
                      std::find(runtime.trace.begin(), runtime.trace.end(),
                                "read:393" + std::to_string(
                                    test.status_offset == 4u ? 220u : 224u)) !=
                          runtime.trace.end() &&
                      runtime.trace.back() ==
                          "record-type:" + std::to_string(test.kind),
                  "record kind, lookup base and status offset determine mode");
        }
    }

    for (const auto command : {0u, 1u, 2u}) {
        Runtime missing;
        missing.words[kController + 52u] = command;
        missing.words[kController + 48u] = 0x1234u;
        missing.words[0x40000u + 816u] = 0x70000u;
        missing.selection_record_present = false;
        const char* kind = command == 0u ? "magic" :
                           command == 1u ? "stunt" : "property";
        Check(Controller::DetermineTargetMode(kController, missing) == 2u &&
                  missing.trace.back() ==
                      "selection-error:" + std::string(kind) +
                      ":458752:4660" &&
                  missing.bytes[kController + 80u] == 1u,
              "missing record reports its command kind and leaves gate set");
    }

    Runtime invalid;
    invalid.words[kController + 52u] = 9u;
    Check(Controller::DetermineTargetMode(kController, invalid) == 2u &&
              invalid.trace.back() ==
                  "selection-error:command:8657252:9",
          "invalid command reports the fallback actor name");

    Runtime type;
    Check(Controller::MapTargetType(1u, type) == 0u &&
              Controller::MapTargetType(2u, type) == 1u &&
              type.trace.empty(),
          "supported target types map without diagnostics");
    for (const auto value : {0u, 3u, 4u}) {
        type.trace.clear();
        type.words[0x8ef458u + 16u * value + 4u] = 0x71000u + value;
        Check(Controller::MapTargetType(value, type) == 2u &&
                  type.trace == std::vector<std::string>{
                      "read:" + std::to_string(
                          0x8ef458u + 16u * value + 4u),
                      "type-error:1:" + std::to_string(0x71000u + value)},
              "known unsupported type resolves its description");
    }
    type.trace.clear();
    type.words[0x8ef458u + 4u] = 0u;
    Check(Controller::MapTargetType(0u, type) == 2u &&
              type.trace.back() == "type-error:1:8657252",
          "null target description uses the shared empty text");
    type.trace.clear();
    Check(Controller::MapTargetType(5u, type) == 2u &&
              type.trace == std::vector<std::string>{"type-error:0:5"},
          "unknown target type is reported numerically");
}

void NearestCandidate() {
    using Controller = pal4::ActorManagerTargetSelectionController;
    constexpr std::uint32_t kCoordinates = 0xa0000u;

    Runtime no_first;
    Check(Controller::FindNearestCandidate(
              kController, kCoordinates, no_first) == 0u &&
              no_first.trace == std::vector<std::string>{"read:4152"},
          "missing first context returns before dialog access");
    Runtime no_second;
    no_second.words[kController + 56u] = 1u;
    Check(Controller::FindNearestCandidate(
              kController, kCoordinates, no_second) == 0u &&
              no_second.trace == std::vector<std::string>{
                  "read:4152", "read:4156"},
          "missing second context returns before dialog access");

    Runtime scan;
    scan.words[kController + 56u] = 1u;
    scan.words[kController + 60u] = 0x9000u;
    scan.words[kController + 72u] = 1u;
    scan.words[kCoordinates] = 0x1111u;
    scan.words[kCoordinates + 4u] = 0x2222u;
    scan.words[0x9000u] = 0xb000u;
    scan.bytes[kController + 80u] = 1u;
    scan.selection_actors = {0x7000u, 0x7004u, 0x7008u,
                             0x700cu, 0x7010u};
    scan.target_states[0x7000u] = false;
    for (const auto actor : {0x7004u, 0x7008u, 0x700cu, 0x7010u})
        scan.target_states[actor] = true;
    scan.radii[0x7004u] = 4u;
    scan.radii[0x7008u] = 8u;
    scan.radii[0x700cu] = 12u;
    scan.radii[0x7010u] = 16u;
    scan.helper_results[4u] = 0u;
    scan.helper_results[8u] = 0xa008u;
    scan.helper_results[12u] = 0xa00cu;
    scan.helper_results[16u] = 0xa010u;
    scan.distances[0xa008u] = 9.0;
    scan.distances[0xa00cu] = 4.0;
    scan.distances[0xa010u] = 4.0;
    Check(Controller::FindNearestCandidate(
              kController, kCoordinates, scan) == 0x700cu &&
              std::find(scan.trace.begin(), scan.trace.end(),
                        "selection-alternate") != scan.trace.end() &&
              std::find(scan.trace.begin(), scan.trace.end(),
                        "radius:28672") == scan.trace.end() &&
              std::find(scan.trace.begin(), scan.trace.end(),
                        "distance:40968:45056") != scan.trace.end() &&
              scan.trace.back() == "iterator-delete:131080:1",
          "alternate scan keeps closest accepted actor and first equal tie");

    Runtime unordered;
    unordered.words[kController + 56u] = 1u;
    unordered.words[kController + 60u] = 0x9000u;
    unordered.words[kCoordinates] = 0x1111u;
    unordered.words[kCoordinates + 4u] = 0x2222u;
    unordered.selection_actors = {0x8000u};
    unordered.radii[0x8000u] = 1u;
    unordered.helper_results[1u] = 0xa000u;
    unordered.distances[0xa000u] =
        std::numeric_limits<double>::quiet_NaN();
    Check(Controller::FindNearestCandidate(
              kController, kCoordinates, unordered) == 0u &&
              std::find(unordered.trace.begin(), unordered.trace.end(),
                        "selection-primary") != unordered.trace.end() &&
              unordered.trace.back() == "iterator-delete:131080:1",
          "unordered distance cannot become the nearest candidate");

    Runtime clone_exception;
    clone_exception.words[kController + 56u] = 1u;
    clone_exception.words[kController + 60u] = 1u;
    clone_exception.throw_on_selection_clone = true;
    bool clone_caught = false;
    try {
        static_cast<void>(Controller::FindNearestCandidate(
            kController, kCoordinates, clone_exception));
    } catch (const std::runtime_error&) {
        clone_caught = true;
    }
    Check(clone_caught &&
              clone_exception.trace.back() == "selection-temp-delete",
          "temporary collection is released when cloning throws");

    Runtime traversal_exception;
    traversal_exception.words[kController + 56u] = 1u;
    traversal_exception.words[kController + 60u] = 1u;
    traversal_exception.selection_actors = {0x8000u};
    traversal_exception.throw_on_iterator_next = true;
    bool traversal_caught = false;
    try {
        static_cast<void>(Controller::FindNearestCandidate(
            kController, kCoordinates, traversal_exception));
    } catch (const std::runtime_error&) {
        traversal_caught = true;
    }
    Check(traversal_caught &&
              traversal_exception.trace.back() ==
                  "iterator-delete:131080:1",
          "cloned iterator is released when traversal throws");
}

void TargetAccessAndDispatch() {
    using Controller = pal4::ActorManagerTargetSelectionController;

    Runtime access;
    access.words[kController + 64u] = 0x7000u;
    Check(Controller::CurrentActor(kController, access) == 0x7000u &&
              access.trace == std::vector<std::string>{"read:4160"},
          "current actor accessor reads only the actor field");
    access.trace.clear();
    Check(Controller::SwitchActiveActorThunk(
              kController, 0x8000u, access) == 0x99u &&
              access.trace == std::vector<std::string>{
                  "read:4160", "selected:28672:0", "word:4160:32768",
                  "selected:32768:1"},
          "switch thunk forwards actor and result to the switch entry");

    Runtime empty;
    Check(Controller::DispatchSelectedTarget(kController, empty) == 0u &&
              empty.trace == std::vector<std::string>{"read:4160"},
          "dispatch entry returns zero without an active actor");

    Runtime selected;
    selected.words[kController + 40u] = 0x11u;
    selected.words[kController + 44u] = 0x22u;
    selected.words[kController + 48u] = 0x33u;
    selected.words[kController + 52u] = 0x44u;
    selected.words[kController + 64u] = 0x7000u;
    Check(Controller::DispatchSelectedTarget(kController, selected) == 0x44u &&
              selected.trace == std::vector<std::string>{
                  "read:4160", "word:4140:28672", "read:4136",
                  "read:4140", "read:4144", "read:4148", "router",
                  "dispatch-words:re_select_command_target"} &&
              selected.dispatched_words ==
                  std::array<std::uint32_t, 4>{
                      0x11u, 0x7000u, 0x33u, 0x44u},
          "dispatch entry snapshots payload after storing the active actor");

    Runtime no_restart;
    Check(Controller::RestartUiControl(kController, no_restart) == 0u &&
              no_restart.trace == std::vector<std::string>{"read:4160"},
          "restart event ignores an empty target");

    Runtime restart;
    restart.words[kController + 64u] = 0x7000u;
    restart.words[0xf000u + 508u] = 6u;
    Check(Controller::RestartUiControl(kController, restart) == 0x55u &&
              restart.trace == std::vector<std::string>{
                  "read:4160", "router",
                  "dispatch-address:re_start_ui_control:4136",
                  "combat-system", "read:61948", "read:4160",
                  "selected:28672:0", "router",
                  "dispatch-simple:hide_monster_tips", "byte:4178",
                  "word:4140:0", "word:4160:0", "word:4164:0"},
          "restart event precedes target deselection and stop cleanup");
}

void ClosestActorSearch() {
    using Controller = pal4::ActorManagerTargetSelectionController;

    Runtime preferred;
    preferred.words[kController + 76u] = 0x1234u;
    preferred.words[kController + 60u] = 0x9000u;
    preferred.words[0x9000u] = 0xb000u;
    preferred.preferred_actors = {0x7000u, 0x7004u};
    preferred.radii[0x7000u] = 1u;
    preferred.radii[0x7004u] = 2u;
    preferred.helper_results[1u] = 0xa000u;
    preferred.helper_results[2u] = 0xa004u;
    preferred.distances[0xa000u] = 10.0;
    preferred.distances[0xa004u] = 2.0;
    Check(Controller::FindClosestActor(kController, preferred) ==
              preferred.preferred_iterator &&
              preferred.words[kController + 76u] == 0u &&
              preferred.words[kController + 68u] == 0x7004u &&
              preferred.trace.front() == "word:4172:0" &&
              std::find(preferred.trace.begin(), preferred.trace.end(),
                        "fallback-iterator") == preferred.trace.end() &&
              std::find(preferred.trace.begin(), preferred.trace.end(),
                        "word:4164:28676") <
                  std::find(preferred.trace.begin(), preferred.trace.end(),
                            "dispatch-actor:show_monster_tips:28676") &&
              preferred.trace.back() == "iterator-delete:131072:1",
          "preferred scan picks nearest actor and dispatches tips before cleanup");

    Runtime fallback;
    fallback.words[kController + 60u] = 0x9000u;
    fallback.words[0x9000u] = 0xb000u;
    fallback.preferred_actors = {0x7000u};
    fallback.radii[0x7000u] = 1u;
    fallback.fallback_actors = {0x8000u};
    fallback.radii[0x8000u] = 2u;
    fallback.helper_results[2u] = 0xa004u;
    fallback.distances[0xa004u] = 5.0;
    Check(Controller::FindClosestActor(kController, fallback) ==
              fallback.preferred_iterator &&
              fallback.words[kController + 68u] == 0x8000u &&
              std::find(fallback.trace.begin(), fallback.trace.end(),
                        "fallback-iterator") != fallback.trace.end() &&
              std::find(fallback.trace.begin(), fallback.trace.end(),
                        "iterator-delete:131076:1") <
                  std::find(fallback.trace.begin(), fallback.trace.end(),
                            "iterator-delete:131072:1") &&
              std::find(fallback.trace.begin(), fallback.trace.end(),
                        "dispatch-actor:show_monster_tips:32768") !=
                  fallback.trace.end(),
          "fallback scan runs after preferred failure and cleans both iterators");

    Runtime none;
    none.words[kController + 68u] = 0x7000u;
    Check(Controller::FindClosestActor(kController, none) ==
              none.preferred_iterator &&
              none.words[kController + 68u] == 0x7000u &&
              std::find_if(none.trace.begin(), none.trace.end(),
                           [](const std::string& item) {
                               return item.starts_with("dispatch-actor:");
                           }) == none.trace.end(),
          "empty scans leave old tips actor and send no event");

    Runtime throwing;
    throwing.preferred_actors = {0x7000u};
    throwing.throw_on_iterator_next = true;
    bool caught = false;
    try {
        static_cast<void>(Controller::FindClosestActor(
            kController, throwing));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
              throwing.trace.back() == "iterator-delete:131072:1",
          "preferred iterator is released when candidate scan throws");
}

void WorldManagerFrameWrapper() {
    struct WorldRuntime final : pal4::IWorldManagerFrameRuntime {
        Runtime& selection;
        bool fail = false;

        explicit WorldRuntime(Runtime& runtime) : selection(runtime) {}
        void UpdateWorldRenderParameters(std::uint32_t world,
                                          float delta) override {
            Check(world == kController + 116u && delta == 0.125f,
                  "world frame uses manager world member and source delta");
            selection.trace.emplace_back("world-render-parameters");
            if (fail) throw std::runtime_error("world update failed");
        }
    };

    Runtime selection;
    selection.inactive = 0x77u;
    WorldRuntime world(selection);
    Check(pal4::WorldManagerFrameUpdater::Update(
              kController, 0.125f, world, selection) == 0x77u &&
              selection.trace.front() == "world-render-parameters" &&
              selection.trace[1] == "combat-system",
          "world update precedes input processing and forwards its result");

    Runtime throwing_selection;
    WorldRuntime throwing_world(throwing_selection);
    throwing_world.fail = true;
    bool caught = false;
    try {
        (void)pal4::WorldManagerFrameUpdater::Update(
            kController, 0.125f, throwing_world, throwing_selection);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing_selection.trace.size() == 1u,
          "failed world update does not process selection input");
}

}  // namespace

int main() {
    try {
        ConstructionAndRegistration();
        UnregisterAndReplace();
        ResetHelpers();
        SelectionCallbacks();
        InputPaths();
        TargetUpdateAndSwitch();
        SelectionRefresh();
        TargetModeAndType();
        NearestCandidate();
        TargetAccessAndDispatch();
        ClosestActorSearch();
        WorldManagerFrameWrapper();
        std::cout << "actor_manager_target_selection_controller_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
