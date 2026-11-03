#include "pal4/portable/combat_state_lifecycle.h"
#include "pal4/pal_game_iv_components.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
using pal4::portable::CombatStateLifecycleBridge;
using pal4::portable::CombatStateOwnerGeneration;

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

template<class F> void Reject(F&& call) {
    bool rejected = false;
    try { call(); } catch (const std::exception&) { rejected = true; }
    Check(rejected, "expected Combat lifecycle rejection");
}

struct Services final : pal4::ICombatSceneServices,
    pal4::ICombatSceneFrameRuntime, pal4::ICombatSystemFrameRenderer,
    pal4::ICombatSceneUpdateServices {
    std::vector<std::string> calls;
    CombatStateLifecycleBridge* bridge{};
    CombatStateOwnerGeneration generation{};
    std::uint32_t game_flag{9}, mode_flag{8}, saved[2]{}, document{}, frame_state{},
        update_state{1}, next_state{}, xml_destroys{};
    bool inactive{}, begin_success{},
        fail_update{}, left_before_xml_destroy{},
        try_leave_during_combat_update{}, rejected_leave_during_combat_update{};

    void EnableUi() override { calls.emplace_back("enable-ui"); }
    void HideConsole() override { calls.emplace_back("hide-console"); }
    void HideScenarioDialog() override { calls.emplace_back("hide-scenario"); }
    void CloseHint() override { calls.emplace_back("close-hint"); }
    std::uint32_t GameFlag(std::uint32_t offset) override {
        Check(offset == 1412u || offset == 1364u, "original Combat flag offset");
        return offset == 1412u ? game_flag : mode_flag;
    }
    void SetGameFlag(std::uint32_t offset, std::uint32_t value) override {
        Check(offset == 1412u || offset == 1364u, "original Combat flag write");
        if (offset == 1412u) game_flag = value; else mode_flag = value;
        calls.emplace_back(offset == 1412u ? "flag-1412" : "flag-1364");
    }
    std::uint32_t SavedFlag(std::uint32_t index) override { return saved[index]; }
    void SetSavedFlag(std::uint32_t index, std::uint32_t value) override {
        saved[index] = value;
        calls.emplace_back(index == 0u ? "save-flag-1412" : "save-flag-1364");
    }
    void AssignRequest(std::uint32_t value) override {
        document = value;
        calls.emplace_back("copy-request");
    }
    void SetupCamera() override { calls.emplace_back("combat-camera-defaults"); }
    bool ForceNormal() override { return false; }
    std::uint32_t ChoiceMode() override { return 1u; }
    void SetWaitingState() override { calls.emplace_back("waiting"); }
    void ShowChoice() override { calls.emplace_back("choice"); }
    void ActivateCamera(bool quick) override {
        calls.emplace_back(quick ? "main-camera" : "combat-camera");
    }
    std::uint8_t BeginCombat(std::uint32_t mode) override {
        Check(mode == 1u, "quick Entry mode forwards original mode");
        calls.emplace_back("begin-combat");
        return begin_success ? 1u : 0u;
    }
    void CloseCombatUi() override { calls.emplace_back("close-combat-ui"); }

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 20u) return 88u;
        if (address == 538u) return frame_state;
        throw std::logic_error("unexpected frame word read");
    }
    std::uint32_t CombatSystem() override { return 30u; }
    bool IsCombatSystemInactive(std::uint32_t system) override {
        Check(system == CombatSystem(), "combat system identity");
        return inactive;
    }
    std::uint32_t ArenaManager() override { return 40u; }
    void ProcessArenaFrame(std::uint32_t manager, std::uint32_t frame) override {
        Check(manager == 40u && frame == 88u, "underlying arena frame arguments");
        calls.emplace_back("arena-render");
    }
    void Render(std::uint32_t frame) override {
        Check(frame == 20u, "Combat renderer receives native frame token");
        calls.emplace_back("combat-render");
    }

    void AdvanceCombat(float elapsed) override {
        Check(elapsed == 0.25f, "Combat update delta");
        calls.emplace_back("combat-update");
        if (try_leave_during_combat_update) {
            try {
                bridge->Leave(generation, 1u);
            } catch (const std::logic_error&) {
                rejected_leave_during_combat_update = true;
            }
        }
        if (fail_update) throw std::runtime_error("Combat update failure");
    }
    void UpdateUI(double elapsed) override {
        Check(elapsed == 0.25, "Combat UI delta");
        calls.emplace_back("ui-update");
    }
    std::uint32_t State() override { return update_state; }
    bool Alternate() override { return false; }
    std::uint32_t Result() override { return 1u; }
    void CleanupScriptCache() override { calls.emplace_back("script-cache-cleanup"); }
    std::uint32_t ConstructXmlDocument() override {
        calls.emplace_back("construct-result-xml");
        return 90u;
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t doc) override {
        Check(doc == 90u, "temporary result XML identity");
        left_before_xml_destroy = bridge && !bridge->Active();
        ++xml_destroys;
        calls.emplace_back("destroy-result-xml");
        return 77u;
    }
    void WriteResult(std::uint32_t doc, std::uint32_t result, bool tree) override {
        Check(doc == 90u && result == 1u && !tree, "original CombatResult write");
        calls.emplace_back("write-result");
    }
    void Transition(std::uint32_t state, std::uint32_t doc) override {
        Check(state == 1u && doc == 90u && bridge,
              "Combat outcome transitions with owned temporary XML");
        calls.emplace_back("transition-scene");
        bridge->Leave(generation, state);
    }
};

struct RegisteredCombatState final : pal4::IPalGameIVComponentsRuntime {
    static constexpr std::uint32_t kGame = 0x4000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_set<std::uint32_t> documents;
    std::vector<std::uint32_t> freed;
    std::uint32_t next{0x10000u}, state{};

    RegisteredCombatState() {
        state = Allocate(72u);
        (void)pal4::PalGameIVComponents::ConstructCombatState(state, *this);
        Check(pal4::PalGameIVStateRegistry::AddEntry(kGame, state, *this),
              "registered state ID 3");
        Check(pal4::PalGameIVStateRegistry::FindEntryById(kGame, 3u, *this) == state,
              "Combat state resolves from real PALGameIV registry");
    }
    std::uint32_t ReadWord(std::uint32_t address) override { return words[address]; }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        const auto address = next;
        next += (bytes + 15u) & ~15u;
        return address;
    }
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        return address;
    }
    std::uint32_t StateId(std::uint32_t object) override {
        return ReadWord(object) == 0x849480u ? 3u : 0u;
    }
    void DeleteState(std::uint32_t object, std::uint32_t flags) override {
        Check(object == state && flags == 1u && documents.erase(object + 12u) == 1u,
              "registered Combat state deleting destructor owns its XML");
        WriteWord(object, 0x8485c8u);
    }
    std::uint32_t EmptyCleanupResult() override { return 0u; }
    std::uint32_t ConstructComponent(pal4::PalGameIVComponentKind kind,
                                     std::uint32_t object) override {
        Check(kind == pal4::PalGameIVComponentKind::CombatXmlDocument &&
              object == state + 12u, "default embedded Combat XML constructor");
        documents.insert(object);
        return object;
    }
    void Unregister() {
        (void)pal4::PalGameIVStateRegistry::Cleanup(kGame, *this);
        Check(documents.empty() && ReadWord(state) == 0x8485c8u,
              "PALGameIV cleanup retires registered Combat owner");
    }
};

struct Readiness final : pal4::portable::ICombatStateReadiness {
    Readiness(RegisteredCombatState& owner_registry,
              CombatStateOwnerGeneration owner_generation)
        : registry(owner_registry), generation(owner_generation) {}

    RegisteredCombatState& registry;
    CombatStateOwnerGeneration generation;
    bool ready{true};
    void ValidateRegisteredCombatOwner(CombatStateOwnerGeneration owner,
                                       std::uint32_t state_id) const override {
        if (!ready || owner != generation || state_id != 3u ||
            pal4::PalGameIVStateRegistry::FindEntryById(
                RegisteredCombatState::kGame, state_id, registry) != registry.state)
            throw std::logic_error("Combat owner/services are not ready");
    }
    bool IsRegisteredCombatOwner(CombatStateOwnerGeneration owner,
                                 std::uint32_t state_id) const override {
        return owner == generation && state_id == 3u &&
            pal4::PalGameIVStateRegistry::FindEntryById(
                RegisteredCombatState::kGame, state_id, registry) == registry.state;
    }
};

void EntryRenderUpdateAndLeave() {
    constexpr CombatStateOwnerGeneration generation = 0xabcdu;
    RegisteredCombatState registered;
    Readiness readiness{registered, generation};
    Services services;
    services.generation = generation;
    CombatStateLifecycleBridge bridge(generation, readiness, services, services,
                                      services, services);
    services.bridge = &bridge;
    Check(bridge.StateId() == 3u && bridge.CurrentPhase() ==
          CombatStateLifecycleBridge::Phase::Ready, "registered PALIV_COMBAT identity");

    Check(bridge.Enter(generation, 1u, 55u), "actual shared Entry accepted");
    Check(bridge.Active() && services.document == 55u && services.game_flag == 0u &&
          services.mode_flag == 1u && services.saved[0] == 9u && services.saved[1] == 8u,
          "Entry copies request and saves/sets original flags");
    Check(services.calls == std::vector<std::string>({
          "enable-ui", "hide-console", "hide-scenario", "close-hint",
          "save-flag-1412", "flag-1412", "hide-console", "save-flag-1364",
          "flag-1364", "copy-request", "combat-camera-defaults", "main-camera",
          "begin-combat"}), "shared Entry call order");

    services.calls.clear();
    services.try_leave_during_combat_update = true;
    services.update_state = 1u;
    (void)bridge.Update(generation, 0.25f);
    Check(services.rejected_leave_during_combat_update && bridge.Active() &&
          services.game_flag == 0u && services.mode_flag == 1u &&
          services.calls == std::vector<std::string>({"combat-update", "ui-update"}),
          "Leave is rejected during Combat advance before the outcome transition");
    services.try_leave_during_combat_update = false;

    services.calls.clear();
    services.frame_state = 0u;
    bridge.Render(generation, 20u);
    Check(services.calls == std::vector<std::string>({"arena-render", "combat-render"}),
          "state-zero frame renders underlying arena then Combat");
    services.calls.clear();
    services.frame_state = 1u;
    bridge.Render(generation, 20u);
    Check(services.calls == std::vector<std::string>({"combat-render"}),
          "active Combat frame skips underlying arena");

    services.calls.clear();
    services.frame_state = 7u;
    services.update_state = 7u;
    services.bridge = &bridge;
    Check(bridge.Update(generation, 0.25f) == 77u && !bridge.Active() &&
          bridge.CurrentPhase() == CombatStateLifecycleBridge::Phase::Ready &&
          services.left_before_xml_destroy && services.xml_destroys == 1u,
          "outcome transition synchronously Leaves before result XML destruction");
    Check(services.calls == std::vector<std::string>({
          "combat-update", "ui-update", "construct-result-xml", "write-result",
          "transition-scene", "flag-1412", "flag-1364", "close-combat-ui",
          "close-hint", "destroy-result-xml"}), "shared outcome/Leave ordering");

    Check(bridge.Enter(generation, 1u, 56u), "same registered owner can re-enter");
    bridge.Leave(generation, 1u);
    Check(!bridge.Active() && services.game_flag == 9u && services.mode_flag == 8u,
          "explicit Leave restores saved game flags");
    registered.Unregister();
    bridge.Retire(generation);
    Check(bridge.CurrentPhase() == CombatStateLifecycleBridge::Phase::Retired,
          "unbound owner can retire");
    Reject([&] { bridge.Render(generation, 20u); });
}

void ReadinessOwnerAndFailureGates() {
    constexpr CombatStateOwnerGeneration generation = 0xdef0u;
    RegisteredCombatState registered;
    Readiness readiness{registered, generation};
    Services services;
    services.generation = generation;
    readiness.ready = false;
    Reject([&] { CombatStateLifecycleBridge bridge(generation, readiness, services,
        services, services, services); });

    readiness.ready = true;
    CombatStateLifecycleBridge bridge(generation, readiness, services, services,
                                      services, services);
    services.bridge = &bridge;
    Reject([&] { (void)bridge.Enter(generation + 1u, 1u, 55u); });
    Reject([&] { (void)bridge.Enter(generation, 1u, 0u); });
    Check(services.calls.empty() && bridge.CurrentPhase() ==
          CombatStateLifecycleBridge::Phase::Ready, "invalid Entry has no effects");

    Check(bridge.Enter(generation, 1u, 55u), "entry before failure injection");
    services.calls.clear();
    readiness.ready = false;
    Reject([&] { bridge.Render(generation, 20u); });
    Reject([&] { (void)bridge.Update(generation, 0.25f); });
    Reject([&] { bridge.Leave(generation, 1u); });
    Check(services.calls.empty(), "unready dependencies reject before side effects");

    readiness.ready = true;
    services.fail_update = true;
    Reject([&] { (void)bridge.Update(generation, 0.25f); });
    Check(bridge.CurrentPhase() == CombatStateLifecycleBridge::Phase::Faulted,
          "partial shared update faults the owner closed");
    services.fail_update = false;
    bridge.Leave(generation, 1u);
    Check(!bridge.NeedsLeave() && bridge.CurrentPhase() ==
          CombatStateLifecycleBridge::Phase::Faulted,
          "faulted active owner still executes original Leave cleanup");
    Reject([&] { bridge.Retire(generation); });
    registered.Unregister();
    bridge.Retire(generation);
}

} // namespace

int main() {
    try {
        EntryRenderUpdateAndLeave();
        ReadinessOwnerAndFailureGates();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
