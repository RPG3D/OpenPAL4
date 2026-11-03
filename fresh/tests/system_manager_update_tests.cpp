#include "pal4/system_manager_update.h"

#include <cmath>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <sstream>
#include <string_view>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISystemManagerUpdateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, float> rates;
    std::map<std::uint32_t, float> properties;
    std::map<std::uint32_t, bool> object_ready;
    std::map<std::uint32_t, std::uint8_t> actor_ready;
    std::map<std::uint32_t, std::uint32_t> dialog_data;
    std::map<std::uint32_t, std::uint32_t> internal;
    std::map<std::uint32_t, std::vector<std::uint32_t>> actor_groups;
    std::map<std::uint32_t, std::size_t> iterator_positions;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> started;
    std::vector<std::vector<std::uint32_t>> released;
    bool inactive = false;
    std::uint32_t state = 2;
    std::uint32_t release_result = 0x3456u;
    std::uint32_t pause_count = 0;
    std::uint32_t skill_count = 0;
    std::uint32_t ready_queries = 0;
    std::uint32_t property_sets = 0;
    std::uint32_t global_count = 0;
    std::uint32_t virtual_four_count = 0;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        trace.emplace_back("active");
        return bytes[address];
    }
    std::uint32_t DialogDataFromParam(
        const std::uint32_t parameter) override {
        trace.emplace_back("dialog-data");
        return dialog_data[parameter];
    }
    std::uint32_t InternalObject(
        const std::uint32_t parameter) override {
        trace.emplace_back("internal");
        return internal[parameter];
    }
    std::uint32_t CombatSystemAddress() override {
        trace.emplace_back("combat-address");
        return 0x8123u;
    }
    bool CombatInactive() override {
        trace.emplace_back("combat-inactive");
        return inactive;
    }
    float ObjectRate(const std::uint32_t object) override {
        trace.emplace_back("rate");
        return rates[object];
    }
    float ObjectProperty(const std::uint32_t object) override {
        trace.emplace_back("property");
        return properties[object];
    }
    void SetObjectProperty(const std::uint32_t object,
                           const float value) override {
        trace.emplace_back("set-property");
        ++property_sets;
        properties[object] = value;
    }
    std::uint32_t CombatState() override {
        trace.emplace_back("state");
        return state;
    }
    void SetCombatState(const std::uint32_t value) override {
        trace.emplace_back("set-state");
        state = value;
    }
    void TouchSkillSystem() override {
        trace.emplace_back("skill");
        ++skill_count;
    }
    void TouchGlobalSystem() override {
        trace.emplace_back("global");
        ++global_count;
    }
    bool ObjectReady(const std::uint32_t object) override {
        trace.emplace_back("object-ready");
        return object_ready[object];
    }
    std::uint32_t BeginActors(const std::uint32_t group) override {
        Check(group < 2u, "actor group index");
        trace.emplace_back(group == 0u ? "begin-first" : "begin-second");
        ++ready_queries;
        const auto iterator = 0xa000u + group;
        iterator_positions[iterator] = 0;
        return iterator;
    }
    bool IteratorFinished(const std::uint32_t iterator) override {
        return iterator_positions[iterator] >=
               actor_groups[iterator - 0xa000u].size();
    }
    std::uint32_t IteratorCurrent(
        const std::uint32_t iterator) override {
        return actor_groups[iterator - 0xa000u][iterator_positions[iterator]];
    }
    void IteratorAdvance(const std::uint32_t iterator) override {
        trace.emplace_back("advance");
        ++iterator_positions[iterator];
    }
    std::uint8_t ActorReady(const std::uint32_t actor) override {
        trace.emplace_back("actor-ready");
        return actor_ready[actor];
    }
    void ReleaseActorIterator(const std::uint32_t iterator) override {
        trace.emplace_back(iterator == 0xa001u ? "release-second" :
                                               "release-first");
    }
    void PauseManager(const std::uint32_t manager) override {
        Check(manager == 0x1000u, "pause manager address");
        trace.emplace_back("pause");
        ++pause_count;
    }
    void StartObject(const std::uint32_t object) override {
        trace.emplace_back("start");
        started.push_back(object);
    }
    void InvokeManagerSlot4(const std::uint32_t owner) override {
        Check(owner == 0x1000u, "slot-four owner");
        trace.emplace_back("virtual-four");
        ++virtual_four_count;
    }
    std::uint32_t ReleaseEvents(
        const std::vector<std::uint32_t>& objects) override {
        trace.emplace_back("release");
        released.push_back(objects);
        return release_result;
    }
};

constexpr auto manager = 0x1000u;

void PropertyProgression() {
    Runtime runtime;
    constexpr auto object = 0x5000u;
    runtime.rates[object] = 1.5f;
    runtime.properties[object] = 10.0f;
    Check(!pal4::SystemManagerUpdate::AdvanceObject(object, 20.0f, runtime) &&
          runtime.properties[object] == 40.0f &&
          runtime.trace == std::vector<std::string>{
              "rate", "property", "set-property"},
          "rate/property/set order below threshold");
    Check(pal4::SystemManagerUpdate::AdvanceObject(object, 20.0f, runtime) &&
          runtime.properties[object] == 64.0f,
          "property clamps above 64 and reports ready");
    runtime.rates[object] = 0.0f;
    Check(pal4::SystemManagerUpdate::AdvanceObject(object, 1.0f, runtime) &&
          runtime.properties[object] == 64.0f,
          "exact threshold is ready");
    runtime.properties[object] = std::numeric_limits<float>::quiet_NaN();
    Check(!pal4::SystemManagerUpdate::AdvanceObject(object, 1.0f, runtime) &&
          std::isnan(runtime.properties[object]),
          "unordered float remains unready");
}

void EventSelectionAndStates() {
    Runtime runtime;
    runtime.rates[0x5000u] = 5.0f;
    runtime.rates[0x5010u] = 10.0f;
    runtime.rates[0x5020u] = 10.0f;
    const std::vector<std::uint32_t> ready{0x5000u, 0x5010u, 0x5020u};
    Check(!pal4::SystemManagerUpdate::ProcessEvents(manager, {}, runtime) &&
          runtime.trace.empty(), "empty queue returns before combat lookup");
    Check(pal4::SystemManagerUpdate::ProcessEvents(manager, ready, runtime) &&
          runtime.started == std::vector<std::uint32_t>{0x5010u},
          "highest rate selected and first equal-rate object wins");
    runtime.started.clear();
    runtime.state = 1;
    Check(pal4::SystemManagerUpdate::ProcessEvents(manager, ready, runtime) &&
          runtime.skill_count == 1 && runtime.pause_count == 1 &&
          runtime.ready_queries == 0 && runtime.started.empty(),
          "combat state one pauses without readiness query");
    runtime.state = 3;
    Check(pal4::SystemManagerUpdate::ProcessEvents(manager, ready, runtime) &&
          runtime.pause_count == 2 && runtime.ready_queries == 2 &&
          runtime.state == 3 && runtime.started.empty(),
          "combat state three waits when objects are not ready");
    runtime.actor_groups[0] = {0x7000u};
    runtime.actor_groups[1] = {0x8000u};
    runtime.actor_ready[0x7000u] = true;
    runtime.actor_ready[0x8000u] = true;
    Check(pal4::SystemManagerUpdate::ProcessEvents(manager, ready, runtime) &&
          runtime.state == 2 && runtime.started ==
              std::vector<std::uint32_t>{0x5010u},
          "ready combat state advances and starts selected object");
    runtime.bytes[manager + 124u] = 1;
    runtime.started.clear();
    Check(pal4::SystemManagerUpdate::ProcessEvents(manager, ready, runtime) &&
          runtime.started.empty(), "active system does not start another object");
}

void CombatScanAndDispatch() {
    Runtime runtime;
    runtime.words[manager + 112u] = 0x2000u;
    runtime.words[manager + 116u] = 0x2008u;
    runtime.words[0x2000u] = 0x5000u;
    runtime.words[0x2004u] = 0x5010u;
    runtime.rates[0x5000u] = 2.0f;
    runtime.rates[0x5010u] = 0.5f;
    runtime.properties[0x5000u] = 30.0f;
    runtime.properties[0x5010u] = 20.0f;
    Check(pal4::SystemManagerUpdate::Dispatch(manager, 0.02f, runtime) ==
              0x56u &&
          runtime.properties[0x5000u] == 64.0f &&
          runtime.properties[0x5010u] == 30.0f &&
          runtime.released ==
              std::vector<std::vector<std::uint32_t>>{{0x5000u}} &&
          runtime.started == std::vector<std::uint32_t>{0x5000u},
          "combat scan scales seconds to milliseconds and dispatch truncates return");
    Runtime denied;
    denied.state = 0;
    Check(pal4::SystemManagerUpdate::Dispatch(manager, 5.0f, denied) == 0 &&
          denied.trace == std::vector<std::string>{"active", "state"},
          "gate denies combat lookup and scan");
}

void OutOfCombatRepeatsUntilEvent() {
    Runtime runtime;
    runtime.inactive = true;
    runtime.words[manager + 112u] = 0x2000u;
    runtime.words[manager + 116u] = 0x2004u;
    runtime.words[0x2000u] = 0x5000u;
    runtime.rates[0x5000u] = 1.0f;
    runtime.properties[0x5000u] = 10.0f;
    Check(pal4::SystemManagerUpdate::Dispatch(manager, 0.001f, runtime) ==
              0x56u &&
          runtime.property_sets == 3 &&
          runtime.properties[0x5000u] == 64.0f &&
          runtime.released ==
              std::vector<std::vector<std::uint32_t>>{{0x5000u}},
          "out-of-combat repeats fixed 20-unit steps until event");
    Runtime empty;
    empty.inactive = true;
    Check(pal4::SystemManagerUpdate::Dispatch(manager, 1.0f, empty) ==
              0x56u &&
          empty.released ==
              std::vector<std::vector<std::uint32_t>>{{}},
          "empty out-of-combat list releases empty batch once");
}

void StateGateAndBufferSwap() {
    Runtime runtime;
    runtime.words[manager + 112u] = 0x2000u;
    runtime.words[manager + 116u] = 0x2010u;
    runtime.words[manager + 120u] = 0x2100u;
    Check(pal4::SystemManagerUpdate::SwapBuffers(manager, runtime) ==
              0x2010u &&
          runtime.words[manager + 116u] == 0x2000u &&
          runtime.words[manager + 120u] == 0x2100u,
          "buffer swap returns old end and preserves capacity");
    for (const auto state : {0u, 1u, 2u, 3u, 4u, 7u, 8u, 0xffffffffu}) {
        runtime.state = state;
        const auto allowed = state != 0u &&
                             (static_cast<std::int32_t>(state) <= 3 || state > 7u);
        Check(pal4::SystemManagerUpdate::CanUpdate(manager, runtime) == allowed,
              "signed combat-state update gate");
    }
    runtime.bytes[manager + 124u] = 2u;
    runtime.trace.clear();
    Check(pal4::SystemManagerUpdate::IsActive(manager, runtime) == 2u &&
          !pal4::SystemManagerUpdate::CanUpdate(manager, runtime) &&
          runtime.trace == std::vector<std::string>{"active", "active"},
          "active byte is returned verbatim and short-circuits combat lookup");
}

void ActorReadinessAndReleaseOrder() {
    Runtime runtime;
    runtime.words[manager + 112u] = 0x2000u;
    runtime.words[manager + 116u] = 0x2004u;
    runtime.words[0x2000u] = 0x5000u;
    Check(!pal4::SystemManagerUpdate::AllObjectsReady(manager, runtime) &&
          runtime.ready_queries == 0,
          "unready manager object stops before actor iterators");
    runtime.object_ready[0x5000u] = true;
    runtime.actor_groups[0] = {0x7000u, 0x7010u};
    runtime.actor_groups[1] = {0x8000u, 0x8010u};
    runtime.actor_ready[0x7010u] = true;
    runtime.actor_ready[0x8010u] = true;
    runtime.trace.clear();
    Check(pal4::SystemManagerUpdate::AllObjectsReady(manager, runtime) &&
          runtime.ready_queries == 2 && runtime.skill_count == 2 &&
          runtime.trace[runtime.trace.size() - 2] == "release-second" &&
          runtime.trace.back() == "release-first",
          "both actor groups find a ready member and release in reverse order");
    runtime.actor_groups[1].clear();
    Check(!pal4::SystemManagerUpdate::AllObjectsReady(manager, runtime) &&
          runtime.ready_queries == 4 && runtime.skill_count == 4,
          "empty second actor group fails after first succeeds");
}

void UpdateCallback() {
    Runtime runtime;
    runtime.state = 1;
    pal4::SystemManagerUpdate::Update(manager, runtime);
    Check(runtime.trace == std::vector<std::string>{"state"},
          "state one exits update immediately");
    runtime.state = 2;
    runtime.trace.clear();
    pal4::SystemManagerUpdate::Update(manager, runtime);
    Check(runtime.skill_count == 1 && runtime.global_count == 1 &&
          runtime.virtual_four_count == 0,
          "inactive manager touches global services but does not finalize");
    runtime.bytes[manager + 124u] = 1;
    runtime.trace.clear();
    pal4::SystemManagerUpdate::Update(manager, runtime);
    Check(runtime.skill_count == 6 && runtime.global_count == 2 &&
          runtime.virtual_four_count == 0,
          "active manager without ready actors leaves slot four unused");
    runtime.actor_groups[0] = {0x7000u};
    runtime.actor_groups[1] = {0x8000u};
    runtime.actor_ready[0x7000u] = true;
    runtime.actor_ready[0x8000u] = true;
    runtime.trace.clear();
    pal4::SystemManagerUpdate::Update(manager, runtime);
    Check(runtime.skill_count == 11 && runtime.global_count == 3 &&
          runtime.virtual_four_count == 1 &&
          runtime.trace.back() == "virtual-four",
          "fully ready active manager invokes slot four after skill access");
}

void RemoveAndDialogEvents() {
    Runtime runtime;
    runtime.words[manager + 112u] = 0x2000u;
    runtime.words[manager + 116u] = 0x200cu;
    runtime.words[0x2000u] = 0x5000u;
    runtime.words[0x2004u] = 0x5010u;
    runtime.words[0x2008u] = 0x5020u;
    Check(pal4::SystemManagerUpdate::RemoveObject(
              manager, 0x5010u, runtime) == 0x200cu &&
          runtime.words[0x2004u] == 0x5020u &&
          runtime.words[manager + 116u] == 0x2008u,
          "middle object removal shifts tail and returns old end");
    Check(pal4::SystemManagerUpdate::RemoveObject(
              manager, 0xdeadu, runtime) == 0x2008u &&
          runtime.words[manager + 116u] == 0x2008u,
          "missing object leaves array unchanged");
    runtime.dialog_data[0x9000u] = 0x7000u;
    runtime.internal[0x9000u] = 0x5000u;
    runtime.actor_ready[0x7000u] = 0xfeu;
    Check(pal4::SystemManagerUpdate::HandleDialogParam(
              manager, 0x9000u, runtime) == 0xfeu &&
          runtime.words[manager + 116u] == 0x2008u,
          "ready actor forwards exact nonzero byte without removal");
    runtime.actor_ready[0x7000u] = 0;
    Check(pal4::SystemManagerUpdate::HandleDialogParam(
              manager, 0x9000u, runtime) == 0x08u &&
          runtime.words[manager + 116u] == 0x2004u &&
          runtime.words[0x2000u] == 0x5020u,
          "unready dialog actor removes internal object and returns old-end byte");
    (void)pal4::SystemManagerUpdate::RemoveObject(manager, 0x5020u, runtime);
    Check(runtime.words[manager + 116u] == 0x2000u &&
          pal4::SystemManagerUpdate::RemoveObject(
              manager, 0x5020u, runtime) == 0x2000u,
          "last and empty-list removals retain old end convention");
}

void CombatStateCallbacks() {
    Runtime runtime;
    runtime.dialog_data[0x9000u] = 0x7000u;
    Check(pal4::SystemManagerUpdate::SetCombatStateFromParam(
              0x9000u, runtime) == 0 && runtime.state == 2 &&
          runtime.trace ==
              std::vector<std::string>{"dialog-data", "actor-ready"},
          "unready actor cannot set combat state");
    runtime.actor_ready[0x7000u] = 1;
    Check(pal4::SystemManagerUpdate::SetCombatStateFromParam(
              0x9000u, runtime) == 0x23u && runtime.state == 3,
          "ready actor sets combat state three and returns instance low byte");
    runtime.state = 2;
    runtime.trace.clear();
    Check(pal4::SystemManagerUpdate::UpdateCombatState(
              manager, runtime) == 0x23u && runtime.state == 2,
          "non-three state returns combat instance byte after manager update");
    runtime.state = 3;
    Check(pal4::SystemManagerUpdate::UpdateCombatState(
              manager, runtime) == 0 && runtime.state == 3,
          "state three remains when actor groups are unready");
    runtime.actor_groups[0] = {0x7000u};
    runtime.actor_groups[1] = {0x8000u};
    runtime.actor_ready[0x8000u] = 1;
    Check(pal4::SystemManagerUpdate::UpdateCombatState(
              manager, runtime) == 0x23u && runtime.state == 2,
          "state three advances to two when both actor groups are ready");
}

// Offline observations retain their own clocks and properties. The fixture
// supplies fitted rate candidates; this calls the production algorithm rather
// than reproducing its arithmetic in the comparison tool.
void CapturedProgress(const char* path) {
    std::ifstream input(path);
    Check(input.good(), "cannot open captured ATB fixture");
    Runtime runtime;
    std::map<std::string, std::size_t> counts;
    std::string line;
    std::size_t line_number{};
    constexpr auto object=0x5000u;
    while (std::getline(input,line)) {
        ++line_number;
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);
        std::string side,extra;
        std::uint32_t sequence{},slot{},delta{},rate{},before{},after{};
        if (!(row>>side>>sequence>>slot>>delta>>rate>>before>>after) || row>>extra)
            throw std::runtime_error("invalid captured ATB fixture row "+std::to_string(line_number));
        const auto seconds=std::bit_cast<float>(delta);
        const auto fitted_rate=std::bit_cast<float>(rate);
        Check((side=="original" || side=="native") && std::isfinite(seconds) && seconds>=0.f &&
              std::isfinite(fitted_rate) && fitted_rate>0.f,"invalid captured ATB operands");
        runtime.trace.clear();
        runtime.rates[object]=fitted_rate;
        runtime.properties[object]=std::bit_cast<float>(before);
        const auto ready=pal4::SystemManagerUpdate::AdvanceObject(object,seconds*1000.f,runtime);
        if (std::bit_cast<std::uint32_t>(runtime.properties[object])!=after ||
            ready!=(std::bit_cast<float>(after)>=64.f))
            throw std::runtime_error("captured ATB mismatch: "+side+" sequence="+
                std::to_string(sequence)+" slot="+std::to_string(slot)+" rate_bits="+std::to_string(rate));
        ++counts[side];
    }
    Check(input.eof() && counts["original"] && counts["native"],"captured ATB fixture requires both nonempty sides");
    std::cout<<"Captured ATB production replay: original="<<counts["original"]<<" native="<<counts["native"]
             <<" exact progress bits/readiness; supplied fitted rates, independent observed clocks; selection/actions outside scope\n";
}

}  // namespace

int main(int argc,char** argv) {
    try {
        if (argc==3 && std::string_view(argv[1])=="--trace-fixture") {
            CapturedProgress(argv[2]);
            return 0;
        }
        Check(argc==1,"usage: system_manager_update_tests [--trace-fixture FILE]");
        const pal4::SystemObjectPropertyRecord property{9.0f, 2.5f};
        Check(pal4::SystemObjectProperty::Get(property) == 2.5f,
              "property getter reads second float field");
        PropertyProgression();
        EventSelectionAndStates();
        CombatScanAndDispatch();
        OutOfCombatRepeatsUntilEvent();
        StateGateAndBufferSwap();
        ActorReadinessAndReleaseOrder();
        UpdateCallback();
        RemoveAndDialogEvents();
        CombatStateCallbacks();
        std::cout << "system_manager_update_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
