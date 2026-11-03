#include "pal4/pal_actor_control.h"
#include "pal4/actor_control_database.h"
#include "pal4/actor_control_motion.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PalActorControlRuntimeStub {
    void ConstructGameComponent(std::uint32_t* storage) override {
        events.emplace_back("component");
        Check(storage == state->words.data() + 23u,
              "component begins at byte offset 92");
        MaybeThrow("component");
        storage[0] = 0x2300u;
    }
    void ConstructDatabase(std::uint32_t* storage) override {
        events.emplace_back("database");
        Check(storage == state->words.data() + 35u &&
                  state->words[23] == 0x2300u,
              "database begins at byte offset 140 after component");
        MaybeThrow("database");
        storage[0] = 0x3500u;
    }
    void ConstructMotion(std::uint32_t* storage) override {
        events.emplace_back("motion");
        Check(storage == state->words.data() + 165u &&
                  state->words[35] == 0x3500u,
              "motion begins at byte offset 660 after database");
        MaybeThrow("motion");
        storage[0] = 0x1650u;
    }
    void ConstructDNameNode(std::uint32_t* storage) override {
        const auto offset = static_cast<std::size_t>(
            storage - state->words.data());
        events.emplace_back("name " + std::to_string(offset));
        Check(offset == 186u || offset == 203u || offset == 279u,
              "DNameNode uses one of three observed offsets");
        MaybeThrow("name " + std::to_string(offset));
        storage[0] = 0x1000u + static_cast<std::uint32_t>(offset);
    }
    void ConstructNamedNode(std::uint32_t* storage,
                            std::string_view name) override {
        events.emplace_back(std::string(name));
        const auto offset = static_cast<std::size_t>(
            storage - state->words.data());
        Check((name == "FL" && offset == 209u) ||
                  (name == "FR" && offset == 216u),
              "named floor nodes use observed strings and offsets");
        Check(state->words[205] == 0x842660u &&
                  state->words[207] == 0x842654u,
              "floor nodes follow two member-vtable installations");
        MaybeThrow(std::string(name));
        storage[0] = name == "FL" ? 0xf1u : 0xf2u;
    }
    void DestructDatabase(std::uint32_t* storage) override {
        events.emplace_back("cleanup database");
        Check(storage == state->words.data() + 35u,
              "unwind destroys embedded database");
    }
    void DestructDNameNode(std::uint32_t* storage) override {
        const auto offset = static_cast<std::size_t>(
            storage - state->words.data());
        events.emplace_back("cleanup name " + std::to_string(offset));
    }
    void ResetMemberVtable(std::uint32_t* storage) override {
        const auto offset = static_cast<std::size_t>(
            storage - state->words.data());
        events.emplace_back("cleanup member " + std::to_string(offset));
    }
    void DestructNamedNode(std::uint32_t* storage) override {
        const auto offset = static_cast<std::size_t>(
            storage - state->words.data());
        events.emplace_back("cleanup named " + std::to_string(offset));
    }
    void SetNamedNodeErrorFlag(std::uint32_t* field,
                               std::uint8_t value) override {
        const auto offset = static_cast<std::size_t>(
            field - state->words.data());
        events.emplace_back("flag " + std::to_string(offset));
        Check(value == 1u && (offset == 217u || offset == 210u),
              "named node text receives error flag one");
    }
    std::int32_t GameConfig(std::uint32_t index) override {
        events.emplace_back("config " + std::to_string(index));
        Check(index <= 1u, "constructor reads only first two settings");
        MaybeThrow("config " + std::to_string(index));
        return config[index];
    }
    std::uint32_t Logger() override {
        events.emplace_back("logger");
        return 0x9000u;
    }
    std::uint32_t Log(std::uint32_t logger,
                      std::string_view format,
                      std::string_view source,
                      std::uint32_t line) override {
        events.emplace_back(line == 79u ? "log begin" : "log done");
        Check(logger == 0x9000u &&
                  source.find("ActorController\\palActorController.cpp") !=
                      std::string_view::npos &&
                  ((line == 79u &&
                    format.find("initializing begin") !=
                        std::string_view::npos) ||
                   (line == 108u &&
                    format.find("initialize ok") !=
                        std::string_view::npos)),
              "startup logs use observed source, line and message");
        return line == 79u ? 0x77u : 0x88u;
    }
    void PrepareScripts() override {
        events.emplace_back("prepare scripts");
    }
    void CleanupScripts() override {
        events.emplace_back("cleanup scripts");
    }
    void InitializeComponent(std::uint32_t* storage) override {
        events.emplace_back("init component");
        Check(storage == state->words.data() + 23u &&
                  state->words[183] == 0u &&
                  state->words[223] == 0u &&
                  state->words[6] == 0u,
              "component starts after initial state reset");
    }
    void InitializeDatabase(std::uint32_t* storage) override {
        events.emplace_back("init database");
        Check(storage == state->words.data() + 35u,
              "database init uses embedded object");
        MaybeThrow("init database");
    }
    void InitializeMotion(std::uint32_t* storage) override {
        events.emplace_back("init motion");
        Check(storage == state->words.data() + 165u,
              "motion init follows database");
    }
    void ShutdownComponent(std::uint32_t* storage) override {
        events.emplace_back("shutdown component");
        Check(storage == state->words.data() + 23u,
              "shutdown starts with component");
    }
    void ShutdownDatabase(std::uint32_t* storage,
                          std::uint32_t context) override {
        events.emplace_back("shutdown database");
        Check(storage == state->words.data() + 35u &&
                  context == 0x1234u,
              "database shutdown receives borrowed context explicitly");
    }
    void ShutdownMotion(std::uint32_t* storage) override {
        events.emplace_back("shutdown motion");
        Check(storage == state->words.data() + 165u,
              "shutdown ends with motion");
    }
    std::uint8_t ResetController(pal4::PalActorControlState&) override {
        events.emplace_back("reset controller");
        Check(state->words[0] == 1u,
              "controller flag clears after reset callback");
        return 0x5au;
    }
    void UpdateComponent(std::uint32_t* storage) override {
        events.emplace_back("update component");
        Check(storage == state->words.data() + 23u,
              "component update uses offset 92");
    }
    void UpdateDatabase(std::uint32_t* storage) override {
        events.emplace_back("update database");
        Check(storage == state->words.data() + 35u,
              "database update uses offset 140");
    }
    std::uint32_t UpdateMotion(std::uint32_t* storage) override {
        events.emplace_back("update motion");
        Check(storage == state->words.data() + 165u,
              "motion update uses offset 660");
        return 0xabcdefu;
    }
    void ProcessComponentInput(std::uint32_t* storage,
                               std::uint32_t actor) override {
        events.emplace_back("input component");
        Check(storage == state->words.data() + 23u &&
                  actor == state->words[1] &&
                  state->words[183] == 0u,
              "input clears flag before forwarding actor to component");
    }
    void ProcessDatabaseInput(std::uint32_t* storage,
                              std::uint32_t actor,
                              std::uint32_t context) override {
        events.emplace_back("input database");
        Check(storage == state->words.data() + 35u &&
                  actor == state->words[1] && context == 0x1234u,
              "database input receives actor and borrowed context");
    }
    std::uint32_t ProcessMotionInput(std::uint32_t* storage,
                                     std::uint32_t actor) override {
        events.emplace_back("input motion");
        Check(storage == state->words.data() + 165u &&
                  actor == state->words[1],
              "motion input receives current actor");
        return 0x88u;
    }
    std::uint32_t SetActorAnimationFlag(std::uint32_t actor,
                                        std::uint32_t value) override {
        events.emplace_back("animation flag");
        Check(actor == state->words[1] && value == 1u,
              "non-null input enables actor animation flag");
        return 0x99u;
    }
    void ProcessComponentInputWithUi(std::uint32_t* storage,
                                     std::uint32_t actor) override {
        events.emplace_back("ui input component");
        Check(storage == state->words.data() + 23u &&
                  actor == state->words[1],
              "UI input starts with component slot 20");
    }
    void ProcessDatabaseInputWithUi(std::uint32_t* storage,
                                    std::uint32_t actor,
                                    std::uint32_t context) override {
        events.emplace_back("ui input database");
        Check(storage == state->words.data() + 35u &&
                  actor == state->words[1] && context == 0x1234u,
              "UI database input receives actor and context");
    }
    void ProcessMotionInputWithUi(std::uint32_t* storage,
                                  std::uint32_t actor) override {
        events.emplace_back("ui input motion");
        Check(storage == state->words.data() + 165u &&
                  actor == state->words[1],
              "UI motion input uses slot 20");
    }

    void MaybeThrow(const std::string& step) {
        if (fail_at == step) throw std::runtime_error(step);
    }

    pal4::PalActorControlState* state{};
    std::int32_t config[2]{6, 4};
    std::string fail_at;
    std::vector<std::string> events;
};

void TestConstruction() {
    pal4::PalActorControlState state;
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    runtime.state = &state;
    Check(&pal4::PalActorControl::Construct(state, runtime) == &state &&
              runtime.events == std::vector<std::string>{
                  "component", "database", "motion", "name 186",
                  "name 203", "FL", "FR", "name 279",
                  "config 1", "config 0"},
          "constructor builds subobjects before configured state");
    Check(state.words[205] == 0x842660u &&
              state.words[206] == 0u &&
              state.words[207] == 0x842654u &&
              state.words[208] == 0u &&
              state.words[209] == 0xf1u &&
              state.words[216] == 0xf2u &&
              state.words[279] == 0x1117u &&
              state.words[0] == 1u && state.words[1] == 0u &&
              state.words[2] == 0x43160000u &&
              state.words[3] == 1u && state.words[4] == 1u &&
              state.words[9] == 0x41900000u &&
              std::bit_cast<float>(state.words[12]) == 10.0f &&
              std::bit_cast<float>(state.words[13]) == 20.0f,
          "constructor stores vtables, fixed values and config ratios");
    for (const auto index : {5u, 6u, 7u, 8u, 10u, 11u,
                             14u, 15u, 16u, 17u, 18u,
                             20u, 21u, 22u, 183u, 184u,
                             185u, 188u, 195u, 202u, 223u})
        Check(state.words[index] == 0u,
              "constructor clears each observed field");
    Check(state.words[19] == 0xa5a5a5a5u &&
              state.words[23] == 0x2300u &&
              state.words[35] == 0x3500u &&
              state.words[165] == 0x1650u &&
              state.words[280] == 0xa5a5a5a5u,
          "constructor retains unobserved fields and subobject writes");
}

void TestZeroDivisor() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.state = &state;
    runtime.config[0] = 0;
    runtime.config[1] = 0;
    static_cast<void>(pal4::PalActorControl::Construct(state, runtime));
    Check(std::isinf(std::bit_cast<float>(state.words[12])) &&
              std::isinf(std::bit_cast<float>(state.words[13])),
          "zero configuration divisors follow floating-point behavior");
}

void TestExceptionUnwind() {
    struct Case {
        const char* failure;
        std::vector<std::string> cleanup;
    };
    for (const auto& scenario : {
         Case{"database", {}},
         Case{"FL", {"cleanup member 207", "cleanup member 205",
                      "cleanup name 203", "cleanup name 186",
                      "cleanup database"}},
         Case{"config 1", {"cleanup named 216", "cleanup named 209",
                            "cleanup member 207", "cleanup member 205",
                            "cleanup name 203", "cleanup name 186",
                            "cleanup database"}}}) {
        pal4::PalActorControlState state{};
        Runtime runtime;
        runtime.state = &state;
        runtime.fail_at = scenario.failure;
        bool caught = false;
        try {
            static_cast<void>(pal4::PalActorControl::Construct(state,
                                                                 runtime));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && state.words[23] == 0x8425d4u,
              "unwind restores component base vtable");
        if (runtime.fail_at == "config 1")
            Check(state.words[165] == 0x8425d4u,
                  "unwind restores motion base vtable");
        Check(runtime.events.size() >= scenario.cleanup.size(),
              "unwind records cleanup calls");
        const auto start = runtime.events.end() - scenario.cleanup.size();
        Check(std::equal(start, runtime.events.end(),
                         scenario.cleanup.begin()),
              "unwind releases completed subobjects in reverse order");
    }
}

void TestDestruction() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.state = &state;
    static_cast<void>(pal4::PalActorControl::Construct(state, runtime));
    runtime.events.clear();
    pal4::PalActorControl::Destruct(state, runtime);
    Check(runtime.events == std::vector<std::string>{
              "cleanup name 279", "flag 217", "flag 210",
              "cleanup name 203", "cleanup name 186",
              "cleanup database"} &&
              state.words[216] == 0x8425a0u &&
              state.words[209] == 0x8425a0u &&
              state.words[207] == 0x8425a0u &&
              state.words[205] == 0x8425a0u &&
              state.words[165] == 0x8425d4u &&
              state.words[23] == 0x8425d4u,
          "destructor follows named-node, motion, database and base order");
}

void TestStubComposition() {
    pal4::PalActorControlState state{};
    pal4::PalActorControlRuntimeStub runtime;
    static_cast<void>(pal4::PalActorControl::Construct(state, runtime));
    Check(state.words[35] ==
              pal4::ActorControlDatabase::kVtable &&
              state.words[165] ==
                  pal4::ActorControlMotion::kVtable,
          "default runtime composes reconstructed database and motion");
    Check(pal4::PalActorControl::Initialize(state, 0x1000u,
                                             runtime) == 0u &&
              state.words[206] == 0x108cu &&
              state.words[208] == 0x108cu &&
              state.words[215] == 0x425300u &&
              state.words[222] == 0x425300u,
          "default runtime initializes callbacks and composes child methods");
    Check(pal4::PalActorControl::UpdateComponents(state, runtime) == 1u,
          "default component update returns the original motion slot result");
    Check(pal4::PalActorControl::Shutdown(state, 0u, runtime) == 0u &&
              state.words[0] == 0u,
          "default runtime shuts down composed child methods");
    Check(pal4::PalActorControl::ProcessInput(state, 0u,
                                               runtime) == 1u &&
              state.words[35u + 127u] == 0u &&
              state.words[165u + 56u / 4u] == 1u,
          "default input composes database and motion reset methods");
    pal4::PalActorControl::Destruct(state, runtime);
    Check(state.words[35] == 0x8425d4u &&
              state.words[165] == 0x8425d4u,
          "default runtime composes reconstructed database teardown");
}

void TestLifecycle() {
    pal4::PalActorControlState state;
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    runtime.state = &state;
    state.words[0] = 1u;
    Check(pal4::PalActorControl::Initialize(state, 0x4000u,
                                             runtime) == 0x88u &&
              state.words[206] == 0x408cu &&
              state.words[208] == 0x408cu &&
              state.words[215] == 0x425300u &&
              state.words[222] == 0x425300u &&
              runtime.events == std::vector<std::string>{
                  "logger", "log begin", "prepare scripts",
                  "init component", "init database", "init motion",
                  "cleanup scripts", "logger", "log done"},
          "initialization forwards logger and script calls in order");
    for (const auto index : {6u, 183u, 184u, 185u, 188u,
                             195u, 202u, 223u})
        Check(state.words[index] == 0u,
              "initialization resets observed fields");
    runtime.events.clear();
    Check(pal4::PalActorControl::UpdateComponents(state, runtime) ==
              0xabcdefu &&
              runtime.events == std::vector<std::string>{
                  "update component", "update database", "update motion"},
          "component update forwards the final virtual return");
    runtime.events.clear();
    Check(pal4::PalActorControl::Shutdown(state, 0x1234u,
                                           runtime) == 0x5au &&
              state.words[0] == 0u &&
              runtime.events == std::vector<std::string>{
                  "shutdown component", "shutdown database",
                  "shutdown motion", "reset controller"},
          "shutdown forwards result and clears controller last");
}

void TestInitializeFailureCleanup() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.state = &state;
    runtime.fail_at = "init database";
    bool failed = false;
    try {
        (void)pal4::PalActorControl::Initialize(state, 0x4000u, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.events == std::vector<std::string>{
              "logger", "log begin", "prepare scripts",
              "init component", "init database", "cleanup scripts"},
          "failed ActorControl initialization releases script scope");
}

void TestInputDispatch() {
    for (const bool has_actor : {false, true}) {
        pal4::PalActorControlState state{};
        Runtime runtime;
        runtime.state = &state;
        state.words[1] = has_actor ? 0x7000u : 0u;
        state.words[183] = 0x9999u;
        Check(pal4::PalActorControl::ProcessInput(
                  state, 0x1234u, runtime) ==
                  (has_actor ? 0x99u : 0x88u) &&
                  state.words[183] == 0u &&
                  runtime.events == (has_actor
                      ? std::vector<std::string>{
                            "input component", "input database",
                            "input motion", "animation flag"}
                      : std::vector<std::string>{
                            "input component", "input database",
                            "input motion"}),
              "input dispatch forwards three virtual calls and actor flag");
    }

    for (const bool has_actor : {false, true}) {
        pal4::PalActorControlState state{};
        Runtime runtime;
        runtime.state = &state;
        state.words[1] = has_actor ? 0x7000u : 0u;
        state.words[20] = 1u;
        state.words[21] = 2u;
        state.words[22] = 3u;
        Check(pal4::PalActorControl::ProcessInputAndClearVector(
                  state, 0x1234u, runtime) ==
                  (has_actor ? 0x99u : 0u) &&
                  state.words[20] == 0u && state.words[21] == 0u &&
                  state.words[22] == 0u &&
                  runtime.events == (has_actor
                      ? std::vector<std::string>{
                            "ui input component", "ui input database",
                            "ui input motion", "animation flag"}
                      : std::vector<std::string>{
                            "ui input component", "ui input database",
                            "ui input motion"}),
              "UI input clears vector after conditional animation call");
    }
}

}  // namespace

int main() {
    pal4::PalActorControlState accessors{};
    accessors.words[33] = 7u;
    Check(pal4::PalActorControl::SetControlWord(accessors,
                                                0x12345678u) ==
              0x12345678u &&
              pal4::PalActorControl::GetControlWord(accessors) ==
                  0x12345678u &&
              pal4::PalActorControl::GetMode(accessors) == 7u &&
              accessors.words[31] == 0u && accessors.words[34] == 0u,
          "control word setter and two adjacent getters");
    TestConstruction();
    TestZeroDivisor();
    TestExceptionUnwind();
    TestDestruction();
    TestStubComposition();
    TestLifecycle();
    TestInitializeFailureCleanup();
    TestInputDispatch();
}
