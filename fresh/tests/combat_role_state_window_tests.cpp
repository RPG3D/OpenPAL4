#include "pal4/combat_role_state_window.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr auto kWindow = 0x4000u;
constexpr auto kQueue = 0x5000u;
constexpr auto kHead = 0x6000u;
constexpr auto kName = 0x8000u;
constexpr auto kSystem = 0x9000u;
constexpr auto kActorManager = 0x25000u;
constexpr auto kCallback = 0xa000u;
constexpr auto kNode = 0xb000u;
constexpr std::array<std::uint32_t, 9> kKeyGlobals{{
    0x8b62e0u, 0x8b6310u, 0x8b62e4u, 0x8b62f0u, 0x8b8b78u,
    0x8b5958u, 0x8b599cu, 0x8b62f0u, 0x8b8b84u,
}};

struct MockEntryRuntime final : pal4::ITimerQueueEntryRuntime {
    void EraseName(pal4::TimerQueueEntryState&,
                   std::uint32_t, std::uint32_t) override {}
    void SetTextFlag(pal4::TimerQueueEntryState&,
                     std::uint8_t) override {}
    std::uint8_t ReadReferenceCount(std::uint32_t) override { return 0u; }
    void WriteReferenceCount(std::uint32_t, std::uint8_t) override {}
    bool AllocateName(pal4::TimerQueueEntryState&,
                      std::uint32_t, std::uint8_t) override { return false; }
    void CopyName(std::uint32_t, std::uint32_t,
                  std::uint32_t) override {}
    void SetNameSize(pal4::TimerQueueEntryState&,
                     std::uint32_t) override {}
    void ReleaseCallback(std::uint32_t, std::uint32_t) override {}
    std::uint32_t FreeName(std::uint32_t) override { return 0u; }
};

struct Runtime final : pal4::ICombatRoleStateWindowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> byte_reads;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> byte_writes;
    struct ColourOp {
        std::uint32_t control;
        std::array<std::uint32_t, 4> rgba_bits;
        bool operator==(const ColourOp&) const = default;
    };
    std::vector<ColourOp> colour_ops;
    struct TintActor {
        bool state3;
        std::int32_t status_kind;
        std::int32_t element;
    };
    std::array<std::uint32_t, 3> tint_roles{};
    std::map<std::uint32_t, TintActor> tint_actors;
    std::vector<std::uint32_t> tint_slot_queries;
    std::map<std::string, std::uint32_t> controls;
    std::vector<std::string> images;
    std::vector<std::uint32_t> fonts;
    struct Click {
        std::uint32_t control;
        pal4::CombatRoleStateClickEvent event;
        std::uint32_t function;
        std::uint32_t owner;
    };
    std::vector<Click> clicks;
    std::vector<std::uint32_t> released_subscriptions;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> updates;
    struct DisplayOp {
        std::string method;
        std::uint32_t control;
        std::uint32_t value;
        bool operator==(const DisplayOp&) const = default;
    };
    std::vector<DisplayOp> display_ops;
    std::vector<std::pair<std::uint32_t, std::string>> texts;
    std::vector<std::string> character_calls;
    std::map<std::uint32_t, std::int32_t> stats;
    std::map<std::uint32_t, std::int32_t> maximums;
    std::uint32_t character_actor = 0x42000u;
    std::int32_t character_slot = 1;
    std::array<std::vector<std::int32_t>, 2> statuses;
    std::array<std::size_t, 2> status_positions{};
    std::vector<bool> status_open_order;
    std::vector<std::uint32_t> status_release_order;
    std::vector<std::uint32_t> event_parameters;
    std::uint32_t actor_manager_queries = 0u;
    struct Actor {
        std::uint32_t address;
        bool state3;
        bool state_zero;
        std::int32_t action_count;
    };
    std::vector<Actor> candidates;
    std::size_t candidate_index = 0u;
    std::uint32_t iterator_releases = 0u;
    std::vector<std::uint32_t> active_roles;
    std::size_t active_role_index = 0u;
    std::uint32_t active_iterator_releases = 0u;
    bool combat_state_ending = false;
    MockEntryRuntime entry_runtime;
    std::uint32_t registration_count = 0u;
    std::uint32_t callback_allocation_count = 0u;
    std::uint32_t node_allocation_count = 0u;
    std::uint32_t assigned_callback = 0u;
    std::uint32_t assigned_period = 0u;
    std::uint32_t assigned_destination = 0u;
    std::uint32_t released_callback = 0u;
    std::uint32_t release_flags = 0u;
    std::uint32_t removed_queue = 0u;
    std::uint32_t removed_name = 0u;
    std::uint32_t parent = 0u;
    bool visible = false;
    bool fail_callback_allocation = false;
    std::uint32_t cleanup_result = 0x5bu;

    Runtime() {
        words[kSystem + 412u] = kQueue;
        words[kQueue + 4u] = kHead;
        words[kHead] = kHead;
        words[kHead + 4u] = kHead;
        for (std::uint32_t index = 0u; index < kKeyGlobals.size(); ++index)
            words[kKeyGlobals[index]] = 0x200u + index;
        words[0x8b62f0u] = 0x203u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        byte_reads.push_back(address);
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        byte_writes.emplace_back(address, value);
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        if (size == 12u) {
            calls.emplace_back("allocate_callback");
            ++callback_allocation_count;
            return fail_callback_allocation ? 0u : kCallback;
        }
        Check(size == 40u, "timer allocates a 40-byte node");
        calls.emplace_back("allocate_node");
        ++node_allocation_count;
        return kNode;
    }
    void LoadLayout(const std::uint32_t window,
                    const std::uint32_t layout_name) override {
        calls.emplace_back("layout");
        Check(window == kWindow && layout_name == 0x1234u,
              "role-state constructor forwards layout name");
    }
    std::uint32_t GetImage(const std::string_view imageset,
                           const std::string_view image) override {
        if (images.empty()) calls.emplace_back("images");
        Check((imageset == "kuang0" && images.size() < 28u) ||
              (imageset == "zi0" && images.size() >= 28u),
              "image groups follow IDA order");
        Check(
              words[kWindow] == pal4::CombatRoleStateWindow::kVtable,
              "image loading follows vtable installation");
        for (std::uint32_t index = 0u; index < 9u; ++index)
            Check(words[kWindow + 40u + 8u * index] == 0u &&
                  words[kWindow + 44u + 8u * index] == 0u,
                  "all nine subscription pairs are clear before image loading");
        images.emplace_back(std::string(imageset) + "/" + std::string(image));
        return 0xd000u + static_cast<std::uint32_t>(images.size());
    }
    std::string ConvertToUtf8(const std::string& name) override {
        return name;
    }
    std::uint32_t GetWindowByName(const std::uint32_t window,
                                  const std::string& name) override {
        Check(window == kWindow, "control lookup uses role-state window");
        const auto control = 0x10000u +
            0x100u * static_cast<std::uint32_t>(controls.size());
        Check(controls.emplace(name, control).second,
              "each role-state control is looked up once");
        return control;
    }
    void SetFont(const std::uint32_t control,
                 const std::string_view font) override {
        Check(font == "Figures5", "role-state text uses Figures5");
        fonts.push_back(control);
    }
    float GetWidth(const std::uint32_t control) override {
        return static_cast<float>(control / 0x100u);
    }
    std::uint32_t SubscribeClick(
        const std::uint32_t control,
        const pal4::CombatRoleStateClickEvent event,
        const std::uint32_t callback) override {
        Check(callback == kCallback &&
              words[callback] == pal4::CombatRoleStateWindow::kClickCallbackVtable,
              "click callback uses the role-state vtable");
        clicks.push_back({control, event, words[callback + 4u],
                          words[callback + 8u]});
        return 0xe000u + static_cast<std::uint32_t>(clicks.size());
    }
    void ReleaseSubscription(const std::uint32_t subscription) override {
        released_subscriptions.push_back(subscription);
    }
    std::uint32_t CleanupTemporaryString() override {
        return cleanup_result;
    }
    void SetParent(const std::uint32_t window,
                   const bool enabled,
                   const std::uint32_t target) override {
        calls.emplace_back("parent");
        Check(window == kWindow, "visibility acts on role-state window");
        visible = enabled;
        parent = target;
    }
    void SetImage(const std::uint32_t control,
                  const std::uint32_t image) override {
        calls.emplace_back("update");
        updates.push_back(static_cast<std::uint32_t>(updates.size()));
        display_ops.push_back({"image", control, image});
    }
    void SetWidth(const std::uint32_t control, const float width) override {
        display_ops.push_back({"width", control,
                               std::bit_cast<std::uint32_t>(width)});
    }
    void SetText(const std::uint32_t control,
                 const std::string_view value) override {
        texts.emplace_back(control, value);
        display_ops.push_back({"text", control, 0u});
    }
    void SetControlVisible(const std::uint32_t control,
                           const bool enabled) override {
        display_ops.push_back({"visible", control,
                               static_cast<std::uint32_t>(enabled)});
    }
    void SetControlEnabled(const std::uint32_t control,
                           const bool enabled) override {
        display_ops.push_back({"enabled", control,
                               static_cast<std::uint32_t>(enabled)});
    }
    bool IsCombatStateEnding() override { return combat_state_ending; }
    std::uint32_t OpenCandidateIterator() override {
        candidate_index = 0u;
        return 0x21000u;
    }
    std::uint32_t OpenActiveRoleIterator() override {
        active_role_index = 0u;
        return 0x24000u;
    }
    bool IteratorAtEnd(const std::uint32_t iterator) override {
        if (iterator == 0x24000u)
            return active_role_index == active_roles.size();
        Check(iterator == 0x21000u, "candidate iterator handle is preserved");
        return candidate_index == candidates.size();
    }
    std::uint32_t CurrentActor(const std::uint32_t iterator) override {
        if (iterator == 0x24000u)
            return active_roles.at(active_role_index);
        Check(iterator == 0x21000u, "current actor uses candidate iterator");
        return candidates.at(candidate_index).address;
    }
    void AdvanceIterator(const std::uint32_t iterator) override {
        if (iterator == 0x24000u) {
            ++active_role_index;
            return;
        }
        Check(iterator == 0x21000u, "candidate advance uses iterator");
        ++candidate_index;
    }
    void ReleaseIterator(const std::uint32_t iterator) override {
        Check(iterator == 0x21000u, "candidate release uses iterator");
        ++iterator_releases;
    }
    std::uint32_t ReleaseActiveRoleIterator(
        const std::uint32_t iterator) override {
        Check(iterator == 0x24000u,
              "active role iterator is released with its own handle");
        ++active_iterator_releases;
        return 0x75u;
    }
    bool ActorCheckState3(const std::uint32_t actor) override {
        const auto tint = tint_actors.find(actor);
        if (tint != tint_actors.end()) return tint->second.state3;
        return candidates.at(candidate_index).address == actor &&
               candidates.at(candidate_index).state3;
    }
    bool ActorIsStateZero(const std::uint32_t actor) override {
        Check(candidates.at(candidate_index).address == actor,
              "state query uses current actor");
        return candidates.at(candidate_index).state_zero;
    }
    std::int32_t ActorActionCount(const std::uint32_t actor) override {
        if (actor == character_actor) {
            character_calls.emplace_back("slot");
            return character_slot;
        }
        Check(candidates.at(candidate_index).address == actor,
              "action count uses current actor");
        return candidates.at(candidate_index).action_count;
    }
    std::int32_t ActorStat(const std::uint32_t actor,
                           const std::uint32_t stat) override {
        Check(actor == character_actor, "stat query uses character actor");
        character_calls.push_back("stat" + std::to_string(stat));
        return stats.at(stat);
    }
    std::int32_t ActorStatMaximum(const std::uint32_t actor,
                                  const std::uint32_t stat) override {
        Check(actor == character_actor, "maximum query uses character actor");
        character_calls.push_back("max" + std::to_string(stat));
        return maximums.at(stat);
    }
    std::uint32_t GetIconImage(const std::uint32_t portrait,
                               const std::uint32_t category,
                               const std::uint32_t variant) override {
        Check(portrait == 2u && category == 1u && variant == 0u,
              "character icon uses portrait and fixed catalog selectors");
        character_calls.emplace_back("icon");
        return 0x43000u;
    }
    void SetBackgroundImage(const std::uint32_t control,
                            const std::uint32_t image) override {
        display_ops.push_back({"background", control, image});
    }
    std::uint32_t OpenStatusIterator(const std::uint32_t actor,
                                     const bool secondary) override {
        Check(actor == character_actor, "status list uses character actor");
        status_open_order.push_back(secondary);
        status_positions[static_cast<std::size_t>(secondary)] = 0u;
        return secondary ? 0x23000u : 0x22000u;
    }
    bool StatusIteratorAtEnd(const std::uint32_t iterator) override {
        const auto source = iterator == 0x23000u ? 1u : 0u;
        Check(iterator == 0x22000u || iterator == 0x23000u,
              "status iterator handle is valid");
        return status_positions[source] == statuses[source].size();
    }
    std::int32_t StatusValue(const std::uint32_t iterator) override {
        const auto source = iterator == 0x23000u ? 1u : 0u;
        return statuses[source].at(status_positions[source]);
    }
    void AdvanceStatusIterator(const std::uint32_t iterator) override {
        const auto source = iterator == 0x23000u ? 1u : 0u;
        ++status_positions[source];
    }
    std::uint8_t ReleaseStatusIterator(const std::uint32_t iterator) override {
        status_release_order.push_back(iterator);
        return iterator == 0x22000u ? 0x7au : 0x6bu;
    }
    std::uint32_t GetDialogDataFromParam(
        const std::uint32_t parameter) override {
        event_parameters.push_back(parameter);
        return parameter == 0u ? 0u : character_actor;
    }
    std::uint32_t ActorManager() override {
        ++actor_manager_queries;
        return kActorManager;
    }
    std::uint32_t ActorAtRoleSlot(const std::uint32_t slot) override {
        tint_slot_queries.push_back(slot);
        return tint_roles.at(slot);
    }
    std::int32_t ActorStatusKind(const std::uint32_t actor) override {
        return tint_actors.at(actor).status_kind;
    }
    std::int32_t ActorElement(const std::uint32_t actor) override {
        return tint_actors.at(actor).element;
    }
    void SetImageColours(
        const std::uint32_t control,
        const std::array<std::uint32_t, 4>& rgba_bits) override {
        colour_ops.push_back({control, rgba_bits});
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("combat_system");
        return kSystem;
    }
    std::uint32_t CreateName(const std::string_view text) override {
        calls.emplace_back("name");
        Check(text == "FlushRoleHead", "timer identifier matches IDA string");
        return kName;
    }
    std::uint8_t CleanupName(const std::uint32_t name) override {
        calls.emplace_back("cleanup_name");
        Check(name == kName, "temporary timer name is released");
        return 0x5au;
    }
    void RemoveTimerByName(const std::uint32_t queue,
                           const std::uint32_t name) override {
        calls.emplace_back("remove_timer");
        removed_queue = queue;
        removed_name = name;
    }
    std::uint32_t EventKey(const std::string_view) override {
        throw std::runtime_error("role state uses global keys");
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x7000u;
    }
    std::uint32_t Register(const std::uint32_t router,
                           const std::uint32_t mode,
                           const std::uint32_t key,
                           const std::uint32_t owner,
                           const std::uint32_t) override {
        calls.emplace_back("register");
        Check(router == 0x7000u && mode == 2u && owner == kWindow &&
              key == words[kKeyGlobals[registration_count]],
              "constructor subscribes each role-state event in order");
        return 0xc000u + 0x100u * registration_count++;
    }
    std::uint32_t FindEventGroup(const std::uint32_t,
                                 const std::uint32_t) override {
        return 0u;
    }
    void ReleaseOwnedSlot(const std::uint32_t) override {}
    void FreeNode(const std::uint32_t) override {}
    void CopyName(pal4::TimerQueueEntryState& entry,
                  const std::uint32_t source) override {
        calls.emplace_back("copy_name");
        Check(source == kName, "timer entry copies the FlushRoleHead name");
        entry.name = {};
    }
    void AssignTimerData(
        const std::uint32_t destination,
        const pal4::TimerQueueEntryState& entry) override {
        calls.emplace_back("assign_timer");
        assigned_destination = destination;
        assigned_callback = entry.callback;
        assigned_period = entry.initial_period_bits;
        Check(entry.current_period_bits == assigned_period &&
              entry.owns_callback == (entry.callback != 0u),
              "timer entry duplicates period and callback ownership");
    }
    void ReleaseCallback(const std::uint32_t callback,
                         const std::uint32_t flags) override {
        calls.emplace_back("release_callback");
        released_callback = callback;
        release_flags = flags;
    }
    pal4::ITimerQueueEntryRuntime& EntryRuntime() override {
        return entry_runtime;
    }
};

void ConstructAndSubscribe() {
    Runtime runtime;
    Check(pal4::CombatRoleStateWindow::Construct(
              kWindow, 0x1234u, runtime) == kWindow &&
          runtime.words[kWindow] == pal4::CombatRoleStateWindow::kVtable &&
          runtime.registration_count == 9u &&
          runtime.calls[0] == "layout" &&
          runtime.calls[1] == "images",
          "constructor loads layout, images and all subscriptions in order");
    Check(runtime.images.size() == 32u && runtime.controls.size() == 61u &&
          runtime.clicks.size() == 4u && runtime.fonts.size() == 9u &&
          runtime.released_subscriptions ==
              std::vector<std::uint32_t>{0xe001u, 0xe002u, 0xe003u,
                                         0xe004u},
          "constructor initializes all images, controls, fonts and clicks");
    for (std::uint32_t index = 0u; index < 9u; ++index)
        Check(runtime.words[kWindow + 40u + 8u * index] ==
                  0xc000u + 0x100u * index &&
              runtime.words[kWindow + 44u + 8u * index] ==
                  runtime.words[kKeyGlobals[index]],
              "constructor saves all nine event handles and keys");
}

void InitializeControlsLayout() {
    Runtime runtime;
    runtime.words[kWindow] = pal4::CombatRoleStateWindow::kVtable;
    constexpr std::array<std::string_view, 28> status_images{{
        "ding", "feng", "jing", "mian", "luan", "kuang", "shui",
        "huo", "lei", "fen", "tu", "jin", "bi", "jie", "ying",
        "ning", "bi2", "sheng", "wu1", "fang1", "su1", "yun1",
        "ling1", "wu2", "fang2", "su2", "yun2", "ling2",
    }};
    constexpr std::array<std::string_view, 4> portraits{{
        "tianhezi", "lingshazi", "menlizi", "murongzi",
    }};
    for (std::uint32_t index = 0u; index < 29u; ++index)
        runtime.words[kWindow + 288u + 4u * index] = 0xffffffffu;
    Check(pal4::CombatRoleStateWindow::InitializeControls(
              kWindow, runtime) == runtime.cleanup_result,
          "control initialization returns temporary string cleanup result");
    Check(runtime.images.size() == 32u, "all 32 images are requested");
    for (std::uint32_t index = 0u; index < status_images.size(); ++index) {
        Check(runtime.images[index] ==
                  std::string("kuang0/") + std::string(status_images[index]) &&
              runtime.words[kWindow + 292u + 4u * index] == 0xd001u + index,
              "status image name and destination match IDA");
    }
    for (std::uint32_t index = 0u; index < portraits.size(); ++index)
        Check(runtime.images[28u + index] ==
                  std::string("zi0/") + std::string(portraits[index]) &&
              runtime.words[kWindow + 272u + 4u * index] == 0xd01du + index,
              "portrait name and destination match IDA");
    Check(runtime.words[kWindow + 288u] == 0u &&
          runtime.words[kWindow + 404u] == runtime.controls.at(
              "CombatRoleState/ImgStatus10"),
          "initial clear includes the reserved first slot and status table");

    constexpr std::array<std::string_view, 9> names{{
        "BtnSubstitution", "ImageFrame", "ImgName", "StaticRoleHp",
        "StaticRoleRage", "StaticRoleMp", "ImageHp", "ImageMp",
        "ImageRage",
    }};
    constexpr std::array<std::uint32_t, 9> offsets{{
        256u, 232u, 244u, 148u, 160u, 172u, 196u, 208u, 220u,
    }};
    constexpr std::array<std::uint32_t, 3> width_offsets{{
        112u, 124u, 136u,
    }};
    for (std::uint32_t slot = 0u; slot < 3u; ++slot) {
        const auto suffix = std::to_string(slot + 1u);
        for (std::uint32_t index = 0u; index < names.size(); ++index) {
            const auto control = runtime.controls.at(
                std::string("CombatRoleState/") + std::string(names[index]) + suffix);
            Check(runtime.words[kWindow + offsets[index] + 4u * slot] == control,
                  "role control pointer matches its offset");
            if (index >= 3u && index <= 5u)
                Check(runtime.fonts[3u * slot + index - 3u] == control,
                      "each text control receives Figures5");
            if (index >= 6u)
                Check(runtime.words[kWindow + width_offsets[index - 6u] +
                                    4u * slot] ==
                          std::bit_cast<std::uint32_t>(
                              runtime.GetWidth(control)),
                      "each bar caches its float width");
        }
        const auto portrait = runtime.controls.at(
            "CombatRoleState/ImageRole" + suffix);
        Check(runtime.words[kWindow + 184u + 4u * slot] == portrait &&
              runtime.words[portrait + 280u] == slot,
              "portrait stores the zero-based role index");
        for (std::uint32_t status = 0u; status < 10u; ++status) {
            const auto name = "CombatRoleState/ImgStatus" + suffix +
                              std::to_string(status);
            Check(runtime.words[kWindow + 404u + 40u * slot +
                                4u * status] == runtime.controls.at(name),
                  "status controls match all 30 indexed slots");
        }
        const auto& click = runtime.clicks[slot];
        Check(click.control == runtime.controls.at(
                  "CombatRoleState/BtnSubstitution" + suffix) &&
              click.event == pal4::CombatRoleStateClickEvent::ButtonClicked &&
              click.function == pal4::CombatRoleStateWindow::kButtonCallback &&
              click.owner == kWindow,
              "substitution button callback fields match IDA");
    }
    Check(runtime.controls.size() == 61u &&
          runtime.words[kWindow + 268u] == runtime.controls.at(
              "CombatRoleState/PanelRoleState") &&
          runtime.clicks[3].control == runtime.words[kWindow + 196u] &&
          runtime.clicks[3].event ==
              pal4::CombatRoleStateClickEvent::MouseClicked &&
          runtime.clicks[3].function ==
              pal4::CombatRoleStateWindow::kMouseCallback &&
          runtime.clicks[3].owner == kWindow,
          "panel and final mouse callback match the IDA offsets");
}

void DisplayResetAndCandidateGate() {
    Runtime runtime;
    constexpr std::uint32_t slot = 1u;
    constexpr std::uint32_t offset = 4u * slot;
    for (const auto field : {148u, 160u, 172u, 184u, 196u, 208u,
                             220u, 232u, 256u})
        runtime.words[kWindow + field + offset] = 0x30000u + field;
    for (std::uint32_t role = 0u; role < 3u; ++role)
        runtime.words[kWindow + 256u + 4u * role] = 0x31000u + role;
    runtime.words[0x31001u + 280u] = 0xffffffffu;
    const std::vector<Runtime::DisplayOp> reset{{"image", 0x30000u + 184u, 0u},
        {"width", 0x30000u + 196u, 0u},
        {"width", 0x30000u + 220u, 0u},
        {"width", 0x30000u + 208u, 0u},
        {"text", 0x30000u + 148u, 0u},
        {"text", 0x30000u + 160u, 0u},
        {"text", 0x30000u + 172u, 0u},
        {"visible", 0x30000u + 232u, 0u}};
    pal4::CombatRoleStateWindow::UpdateDisplay(kWindow, 3, runtime);
    Check(runtime.display_ops.empty(), "indices at least three are ignored");
    pal4::CombatRoleStateWindow::UpdateDisplay(kWindow, 1, runtime);
    Check(runtime.display_ops.size() == reset.size() + 1u &&
          std::equal(reset.begin(), reset.end(), runtime.display_ops.begin()) &&
          runtime.display_ops.back() ==
              Runtime::DisplayOp{"enabled", 0x31001u, 0u} &&
          runtime.words[0x31001u + 280u] == 0u &&
          runtime.iterator_releases == 0u,
          "inactive combat clears this slot and disables only its button");

    runtime.combat_state_ending = true;
    runtime.display_ops.clear();
    pal4::CombatRoleStateWindow::UpdateDisplay(kWindow, 1, runtime);
    Check(runtime.display_ops.size() == reset.size() + 3u &&
          runtime.display_ops[8] ==
              Runtime::DisplayOp{"enabled", 0x31000u, 0u} &&
          runtime.display_ops[9] ==
              Runtime::DisplayOp{"enabled", 0x31001u, 0u} &&
          runtime.display_ops[10] ==
              Runtime::DisplayOp{"enabled", 0x31002u, 0u} &&
          runtime.iterator_releases == 1u,
          "empty candidate range disables and releases all buttons");

    runtime.candidates = {{0x40000u, true, true, 4},
                          {0x40001u, false, false, 4},
                          {0x40002u, false, true, 2},
                          {0x40003u, false, true, 3}};
    runtime.display_ops.clear();
    pal4::CombatRoleStateWindow::UpdateDisplay(kWindow, 1, runtime);
    Check(runtime.candidate_index == 3u &&
          runtime.iterator_releases == 2u &&
          runtime.display_ops.size() == reset.size() + 3u &&
          runtime.display_ops[8] ==
              Runtime::DisplayOp{"enabled", 0x31000u, 1u} &&
          runtime.display_ops[9] ==
              Runtime::DisplayOp{"enabled", 0x31001u, 1u} &&
          runtime.display_ops[10] ==
              Runtime::DisplayOp{"enabled", 0x31002u, 1u},
          "eligible candidate enables all buttons and stops the scan");
}

void CharacterDisplayAndStatWidths() {
    Runtime runtime;
    runtime.character_slot = 3;
    Check(pal4::CombatRoleStateWindow::UpdateCharacterDisplay(
              kWindow, runtime.character_actor, runtime) == 3 &&
          runtime.character_calls == std::vector<std::string>{"slot"} &&
          runtime.display_ops.empty(),
          "character slot at least three returns without touching controls");

    runtime.character_calls.clear();
    runtime.character_slot = 1;
    runtime.words[runtime.character_actor + 1192u] = 2u;
    runtime.words[kWindow + 272u + 8u] = 0x44000u;
    runtime.stats = {{7u, 50}, {9u, 30}, {8u, 60}};
    runtime.maximums = {{0u, 200}, {2u, 60}};
    constexpr std::uint32_t slot = 4u;
    for (const auto field : {112u, 124u, 136u, 148u, 160u, 172u,
                             184u, 196u, 208u, 220u, 232u, 244u,
                             256u})
        runtime.words[kWindow + field + slot] = 0x45000u + field;
    runtime.words[kWindow + 112u + slot] = std::bit_cast<std::uint32_t>(100.0f);
    runtime.words[kWindow + 124u + slot] = std::bit_cast<std::uint32_t>(80.0f);
    runtime.words[kWindow + 136u + slot] = std::bit_cast<std::uint32_t>(120.0f);
    Check(pal4::CombatRoleStateWindow::UpdateCharacterDisplay(
              kWindow, runtime.character_actor, runtime) ==
              static_cast<std::int32_t>(runtime.cleanup_result),
          "character display returns string-buffer cleanup result");
    Check(runtime.character_calls == std::vector<std::string>{
              "slot", "icon", "stat7", "stat9", "stat8", "stat7",
              "max0", "stat9", "max2", "stat8"},
          "character stats and maximums are queried in assembly order");
    Check(runtime.texts ==
              std::vector<std::pair<std::uint32_t, std::string>>{
                  {0x45000u + 148u, "50"},
                  {0x45000u + 172u, "30"},
                  {0x45000u + 160u, "60"}},
          "HP, MP and rage text use the expected control offsets");
    const std::vector<Runtime::DisplayOp> expected{{
        "visible", 0x45000u + 232u, 1u},
        {"image", 0x45000u + 184u, 0x43000u},
        {"text", 0x45000u + 148u, 0u},
        {"text", 0x45000u + 172u, 0u},
        {"text", 0x45000u + 160u, 0u},
        {"width", 0x45000u + 196u,
         std::bit_cast<std::uint32_t>(25.0f)},
        {"width", 0x45000u + 208u,
         std::bit_cast<std::uint32_t>(40.0f)},
        {"width", 0x45000u + 220u,
         std::bit_cast<std::uint32_t>(72.0f)},
        {"background", 0x45000u + 244u, 0x44000u}};
    Check(runtime.display_ops == expected &&
          runtime.words[0x45000u + 256u + 280u] ==
              runtime.character_actor,
          "character display preserves CEGUI order, widths and button data");
}

void CharacterStatusImages() {
    Runtime runtime;
    const auto actor = runtime.character_actor;
    Check(pal4::CombatRoleStateWindow::UpdateCharacterImages(
              kWindow, actor, runtime) == 0u &&
          runtime.status_open_order.empty() &&
          runtime.display_ops.empty(),
          "clean actor does not refresh its status images");
    runtime.bytes[actor + 892u] = 1u;
    runtime.character_slot = 3;
    Check(pal4::CombatRoleStateWindow::UpdateCharacterImages(
              kWindow, actor, runtime) == 3u &&
          runtime.bytes[actor + 892u] == 0u &&
          runtime.status_open_order.empty(),
          "dirty flag clears before rejecting out-of-range role");

    runtime.character_slot = 1;
    runtime.bytes[actor + 892u] = 1u;
    runtime.character_calls.clear();
    runtime.statuses[0] = {-1, 1, 29, 2};
    runtime.statuses[1] = {3};
    for (std::uint32_t index = 0u; index < 10u; ++index)
        runtime.words[kWindow + 404u + 40u + 4u * index] =
            0x51000u + index;
    for (std::uint32_t value = 1u; value <= 28u; ++value)
        runtime.words[kWindow + 288u + 4u * value] =
            0x52000u + value;
    Check(pal4::CombatRoleStateWindow::UpdateCharacterImages(
              kWindow, actor, runtime) == 0x7au &&
          runtime.bytes[actor + 892u] == 0u &&
          runtime.character_calls == std::vector<std::string>{"slot"} &&
          runtime.status_open_order == std::vector<bool>{false, true} &&
          runtime.status_release_order ==
              std::vector<std::uint32_t>{0x23000u, 0x22000u} &&
          runtime.status_positions == std::array<std::size_t, 2>{4u, 1u},
          "status refresh combines both lists and releases in reverse order");
    Check(runtime.display_ops.size() == 26u,
          "status refresh clears ten controls and fills three valid values");
    for (std::uint32_t index = 0u; index < 10u; ++index) {
        Check(runtime.display_ops[2u * index] ==
                  Runtime::DisplayOp{"visible", 0x51000u + index, 0u} &&
              runtime.display_ops[2u * index + 1u] ==
                  Runtime::DisplayOp{"image", 0x51000u + index, 0u},
              "all status controls clear in index order");
    }
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        Check(runtime.display_ops[20u + 2u * index] ==
                  Runtime::DisplayOp{"image", 0x51000u + index,
                                     0x52001u + index} &&
              runtime.display_ops[21u + 2u * index] ==
                  Runtime::DisplayOp{"visible", 0x51000u + index, 1u},
              "valid status values map to image slots one through 28");
    }

    runtime.bytes[actor + 892u] = 1u;
    runtime.statuses[0] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    runtime.statuses[1] = {12};
    runtime.display_ops.clear();
    runtime.status_open_order.clear();
    runtime.status_release_order.clear();
    Check(pal4::CombatRoleStateWindow::UpdateCharacterImages(
              kWindow, actor, runtime) == 0x7au &&
          runtime.status_positions == std::array<std::size_t, 2>{10u, 0u} &&
          runtime.status_open_order == std::vector<bool>{false, true} &&
          runtime.status_release_order ==
              std::vector<std::uint32_t>{0x23000u, 0x22000u} &&
          runtime.display_ops.size() == 40u,
          "ten image capacity stops both iterators but releases each");
}

void AllCharacterImages() {
    Runtime runtime;
    Check(pal4::CombatRoleStateWindow::UpdateAllCharacterImages(
              kWindow, runtime) == 0x75u &&
          runtime.active_role_index == 0u &&
          runtime.active_iterator_releases == 1u &&
          runtime.byte_reads.empty(),
          "empty active role collection releases its iterator");

    runtime.active_roles = {runtime.character_actor, 0x43000u};
    runtime.character_slot = 3;
    runtime.bytes[runtime.character_actor + 892u] = 1u;
    runtime.byte_reads.clear();
    Check(pal4::CombatRoleStateWindow::UpdateAllCharacterImages(
              kWindow, runtime) == 0x75u &&
          runtime.active_role_index == 2u &&
          runtime.active_iterator_releases == 2u &&
          runtime.byte_reads == std::vector<std::uint32_t>{
              runtime.character_actor + 892u, 0x43000u + 892u} &&
          runtime.bytes[runtime.character_actor + 892u] == 0u &&
          runtime.character_calls == std::vector<std::string>{"slot"} &&
          runtime.display_ops.empty(),
          "all active roles are forwarded in order, including clean actors");
}

void CharacterImageEventGate() {
    Runtime runtime;
    runtime.character_actor = 0x42005u;
    const auto actor = runtime.character_actor;
    Check(pal4::CombatRoleStateWindow::OnCharacterImageEvent(
              kWindow, 0u, runtime) == 0u &&
          runtime.byte_writes.empty(),
          "null image event leaves the actor state untouched");
    runtime.words[actor + 804u] = 2u;
    Check(pal4::CombatRoleStateWindow::OnCharacterImageEvent(
              kWindow, 0x100u, runtime) == 5u &&
          runtime.byte_writes.empty(),
          "nonmatching actor state returns the actor low byte");
    runtime.words[actor + 804u] = 1u;
    runtime.character_slot = 3;
    Check(pal4::CombatRoleStateWindow::OnCharacterImageEvent(
              kWindow, 0x200u, runtime) == 3u &&
          runtime.byte_writes ==
              std::vector<std::pair<std::uint32_t, std::uint8_t>>{
                  {actor + 892u, 1u}, {actor + 892u, 0u}} &&
          runtime.character_calls == std::vector<std::string>{"slot"} &&
          runtime.status_open_order.empty(),
          "matching actor state marks dirty before refreshing images");
}

void ButtonDisableAndCombatantVisibility() {
    Runtime runtime;
    for (std::uint32_t role = 0u; role < 3u; ++role)
        runtime.words[kWindow + 256u + 4u * role] = 0x71000u + role;
    pal4::CombatRoleStateWindow::DisableSubstitutionButtons(kWindow, runtime);
    Check(runtime.display_ops == std::vector<Runtime::DisplayOp>{
              {"enabled", 0x71000u, 0u},
              {"enabled", 0x71001u, 0u},
              {"enabled", 0x71002u, 0u}} &&
          runtime.actor_manager_queries == 0u,
          "disable entry touches only the three substitution buttons");

    runtime.display_ops.clear();
    runtime.active_roles = {runtime.character_actor};
    runtime.character_slot = 3;
    runtime.words[kActorManager + 36u] = 0x72000u;
    runtime.words[kActorManager + 40u] = 0x72000u + 3u * 4u;
    Check(pal4::CombatRoleStateWindow::RefreshCombatantVisibility(
              kWindow, runtime) == 0x75u &&
          runtime.actor_manager_queries == 1u &&
          runtime.active_iterator_releases == 1u &&
          runtime.byte_writes ==
              std::vector<std::pair<std::uint32_t, std::uint8_t>>{
                  {runtime.character_actor + 892u, 1u},
                  {runtime.character_actor + 892u, 0u}} &&
          runtime.display_ops == std::vector<Runtime::DisplayOp>{
              {"visible", 0x71000u, 0u},
              {"visible", 0x71001u, 0u},
              {"visible", 0x71002u, 0u}},
          "three actor slots hide buttons and refresh each active role");

    runtime.display_ops.clear();
    runtime.byte_writes.clear();
    runtime.words[kActorManager + 40u] = 0x72000u + 4u * 4u;
    Check(pal4::CombatRoleStateWindow::RefreshCombatantVisibility(
              kWindow, runtime) == 0x75u &&
          runtime.actor_manager_queries == 2u &&
          runtime.active_iterator_releases == 2u &&
          runtime.display_ops == std::vector<Runtime::DisplayOp>{
              {"visible", 0x71000u, 1u},
              {"visible", 0x71001u, 1u},
              {"visible", 0x71002u, 1u}} &&
          runtime.byte_writes.size() == 2u,
          "more than three actor slots reveal buttons and refresh roles");

    runtime.display_ops.clear();
    runtime.active_roles.clear();
    runtime.words[kActorManager + 36u] = 0u;
    Check(pal4::CombatRoleStateWindow::RefreshCombatantVisibility(
              kWindow, runtime) == 0x75u &&
          runtime.display_ops == std::vector<Runtime::DisplayOp>{
              {"visible", 0x71000u, 0u},
              {"visible", 0x71001u, 0u},
              {"visible", 0x71002u, 0u}},
          "null actor range hides all substitution buttons");
}

void CombatantStatEventGate() {
    Runtime runtime;
    constexpr std::uint32_t parameter = 0x81000u;
    Check(pal4::CombatRoleStateWindow::OnCombatantStatEvent(
              kWindow, 0u, runtime) == 0 &&
          runtime.character_calls.empty(),
          "null stat event returns zero without querying a role");
    runtime.character_slot = 3;
    runtime.words[parameter + 4u] = 7u;
    Check(pal4::CombatRoleStateWindow::OnCombatantStatEvent(
              kWindow, parameter, runtime) == 3 &&
          runtime.display_ops.empty(),
          "out-of-range role returns its slot before reading the stat");

    runtime.character_slot = 1;
    runtime.character_calls.clear();
    runtime.words[parameter + 4u] = 5u;
    Check(pal4::CombatRoleStateWindow::OnCombatantStatEvent(
              kWindow, parameter, runtime) == 5 &&
          runtime.character_calls == std::vector<std::string>{"slot"} &&
          runtime.display_ops.empty(),
          "unrelated stat returns its ID without refreshing controls");

    runtime.words[runtime.character_actor + 1192u] = 2u;
    runtime.stats = {{7u, 50}, {9u, 30}, {8u, 60}};
    runtime.maximums = {{0u, 200}, {2u, 60}};
    for (const auto stat : {7u, 9u, 8u}) {
        runtime.words[parameter + 4u] = stat;
        runtime.character_calls.clear();
        runtime.display_ops.clear();
        Check(pal4::CombatRoleStateWindow::OnCombatantStatEvent(
                  kWindow, parameter, runtime) ==
                  static_cast<std::int32_t>(runtime.cleanup_result) &&
              runtime.character_calls == std::vector<std::string>{
                  "slot", "slot", "icon", "stat7", "stat9", "stat8",
                  "stat7", "max0", "stat9", "max2", "stat8"} &&
              runtime.display_ops.size() == 9u,
              "HP, MP and rage events each repopulate the role display");
    }
}

void CombatantColourRefresh() {
    constexpr std::uint32_t blink = 0x8f95b4u;
    constexpr std::array<std::uint32_t, 4> white{{
        0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u}};
    constexpr std::array<std::uint32_t, 4> black{{
        0u, 0u, 0u, 0x3f800000u}};
    constexpr std::array<std::uint32_t, 4> blue{{
        0x3ed6d6d7u, 0x3f119192u, 0x3f6bebecu, 0x3f800000u}};
    constexpr std::array<std::uint32_t, 4> pink{{
        0x3f4dcdceu, 0x3f169697u, 0x3f66e6e7u, 0x3f800000u}};
    constexpr std::array<std::uint32_t, 4> brown{{
        0x3f48c8c9u, 0x3f109091u, 0x3e808081u, 0x3f800000u}};
    constexpr std::array<std::uint32_t, 4> blink_dark{{
        0x3f139394u, 0x3f119192u, 0x3f4acacbu, 0x3f800000u}};
    constexpr std::array<std::uint32_t, 4> blink_bright{{
        0x3f51d1d2u, 0x3f4ececfu, 0x3f69e9eau, 0x3f800000u}};

    Runtime empty;
    Check(pal4::CombatRoleStateWindow::RefreshCombatantColours(
              kWindow, empty) == 0u &&
          empty.bytes[blink] == 1u &&
          empty.tint_slot_queries ==
              std::vector<std::uint32_t>{0u, 1u, 2u} &&
          empty.colour_ops.empty(),
          "timer checks three slots and toggles phase with no active actors");

    Runtime runtime;
    runtime.bytes[blink] = 1u;
    runtime.tint_roles = {0x91000u, 0x92000u, 0x93000u};
    runtime.tint_actors[0x91000u] = {true, 0, 0};
    runtime.tint_actors[0x92000u] = {false, 2, 1};
    runtime.tint_actors[0x93000u] = {false, 0, 3};
    for (std::uint32_t slot = 0u; slot < 3u; ++slot)
        runtime.words[kWindow + 184u + 4u * slot] = 0x94000u + slot;
    for (std::uint32_t colour = 0u; colour < 5u; ++colour)
        runtime.words[kWindow + 316u + 4u * colour] = 0x95000u + colour;
    for (std::uint32_t status = 0u; status < 10u; ++status) {
        const auto control = 0x96000u + status;
        runtime.words[kWindow + 404u + 80u + 4u * status] = control;
        runtime.words[control + 1712u] = 0x97000u;
    }
    runtime.words[0x96000u + 1712u] = 0x95000u;
    runtime.words[0x96009u + 1712u] = 0x95004u;
    Check(pal4::CombatRoleStateWindow::RefreshCombatantColours(
              kWindow, runtime) == 1u &&
          runtime.bytes[blink] == 0u &&
          runtime.tint_slot_queries ==
              std::vector<std::uint32_t>{0u, 1u, 2u} &&
          runtime.colour_ops == std::vector<Runtime::ColourOp>{
              {0x94000u, white}, {0x94000u, black},
              {0x94001u, white}, {0x94001u, blink_bright},
              {0x94002u, white}, {0x94002u, pink},
              {0x94002u, blue}, {0x94002u, brown}},
          "state, blinking and last matching status apply in IDA order");

    runtime.colour_ops.clear();
    runtime.tint_roles = {0u, 0x92000u, 0u};
    Check(pal4::CombatRoleStateWindow::RefreshCombatantColours(
              kWindow, runtime) == 0u &&
          runtime.bytes[blink] == 1u &&
          runtime.colour_ops == std::vector<Runtime::ColourOp>{
              {0x94001u, white}, {0x94001u, blink_dark}},
          "special state uses the alternate colour on the next tick");
}

void EventWrappersAndSubstitutionRefresh() {
    Runtime runtime;
    runtime.character_slot = 3;
    Check(pal4::CombatRoleStateWindow::OnCharacterDisplayEvent(
              kWindow, 0u, runtime) == 0 &&
          pal4::CombatRoleStateWindow::OnCharacterDisplayEvent(
              kWindow, 0x1234u, runtime) == 3 &&
          runtime.event_parameters ==
              std::vector<std::uint32_t>{0u, 0x1234u} &&
          runtime.character_calls == std::vector<std::string>{"slot"} &&
          runtime.display_ops.empty(),
          "character event skips null and forwards the resolved actor");

    runtime.character_slot = 1;
    runtime.event_parameters.clear();
    runtime.character_calls.clear();
    runtime.words[kWindow + 256u + 4u] = 0x61000u;
    pal4::CombatRoleStateWindow::OnRoleDisplayEvent(
        kWindow, 0x5678u, runtime);
    Check(runtime.event_parameters ==
              std::vector<std::uint32_t>{0x5678u} &&
          runtime.character_calls == std::vector<std::string>{"slot"} &&
          runtime.display_ops.size() == 9u &&
          runtime.display_ops.back() ==
              Runtime::DisplayOp{"enabled", 0x61000u, 0u},
          "role event resolves actor slot and invokes display reset");

    runtime.display_ops.clear();
    for (std::uint32_t role = 0u; role < 3u; ++role)
        runtime.words[kWindow + 256u + 4u * role] = 0x62000u + role;
    pal4::CombatRoleStateWindow::RefreshSubstitutionButtons(kWindow, runtime);
    Check(runtime.iterator_releases == 1u &&
          runtime.display_ops == std::vector<Runtime::DisplayOp>{
              {"enabled", 0x62000u, 0u},
              {"enabled", 0x62001u, 0u},
              {"enabled", 0x62002u, 0u}},
          "status refresh disables all buttons for an empty candidate set");

    runtime.display_ops.clear();
    runtime.candidates = {{0x63000u, false, true, 3}};
    pal4::CombatRoleStateWindow::RefreshSubstitutionButtons(kWindow, runtime);
    Check(runtime.iterator_releases == 2u &&
          runtime.candidate_index == 0u &&
          runtime.display_ops == std::vector<Runtime::DisplayOp>{
              {"enabled", 0x62000u, 1u},
              {"enabled", 0x62001u, 1u},
              {"enabled", 0x62002u, 1u}},
          "status refresh enables all buttons for an eligible candidate");
}

void VisibleTimerAndHiddenRemoval() {
    Runtime runtime;
    runtime.calls.clear();
    Check(pal4::CombatRoleStateWindow::SetVisible(
              kWindow, true, 0x1230u, runtime) == 0x5au &&
          runtime.visible && runtime.parent == 0x1230u &&
          runtime.updates == std::vector<std::uint32_t>{0u, 1u, 2u} &&
          runtime.callback_allocation_count == 1u &&
          runtime.node_allocation_count == 1u &&
          runtime.words[kCallback] ==
              pal4::CombatRoleStateWindow::kTimerCallbackVtable &&
          runtime.words[kCallback + 4u] == kWindow &&
          runtime.words[kCallback + 8u] ==
              pal4::CombatRoleStateWindow::kRefreshCallback &&
          runtime.assigned_destination == kNode + 8u &&
          runtime.assigned_callback == kCallback &&
          runtime.assigned_period ==
              pal4::CombatRoleStateWindow::kTimerPeriodBits &&
          runtime.released_callback == kCallback &&
          runtime.release_flags == 1u &&
          runtime.words[kQueue + 8u] == 1u &&
          runtime.words[kHead] == kNode &&
          runtime.words[kHead + 4u] == kNode &&
          runtime.words[kNode] == kHead &&
          runtime.words[kNode + 4u] == kHead,
          "show schedules half-second refresh after updating three displays");
    Check(runtime.calls == std::vector<std::string>{
              "parent", "update", "update", "update", "name",
              "combat_system", "allocate_callback", "copy_name",
              "allocate_node", "assign_timer", "release_callback",
              "cleanup_name"},
          "show follows IDA parent, display, timer and cleanup order");

    runtime.calls.clear();
    Check(pal4::CombatRoleStateWindow::SetVisible(
              kWindow, false, 0x4560u, runtime) == 0x5au &&
          !runtime.visible && runtime.parent == 0x4560u &&
          runtime.removed_queue == kQueue &&
          runtime.removed_name == kName &&
          runtime.calls == std::vector<std::string>{
              "parent", "name", "combat_system", "remove_timer",
              "cleanup_name"},
          "hide removes the named timer without refreshing displays");
}

void NullCallbackTimer() {
    Runtime runtime;
    runtime.fail_callback_allocation = true;
    Check(pal4::CombatRoleStateWindow::SetVisible(
              kWindow, true, 0u, runtime) == 0x5au &&
          runtime.callback_allocation_count == 1u &&
          runtime.node_allocation_count == 1u &&
          runtime.assigned_callback == 0u &&
          runtime.released_callback == 0u &&
          runtime.words[kQueue + 8u] == 1u,
          "callback allocation failure still inserts an unowned timer entry");
}

}  // namespace

int main() {
    try {
        ConstructAndSubscribe();
        InitializeControlsLayout();
        DisplayResetAndCandidateGate();
        CharacterDisplayAndStatWidths();
        CharacterStatusImages();
        AllCharacterImages();
        CharacterImageEventGate();
        ButtonDisableAndCombatantVisibility();
        CombatantStatEventGate();
        CombatantColourRefresh();
        EventWrappersAndSubstitutionRefresh();
        VisibleTimerAndHiddenRemoval();
        NullCallbackTimer();
        std::cout << "combat_role_state_window_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
