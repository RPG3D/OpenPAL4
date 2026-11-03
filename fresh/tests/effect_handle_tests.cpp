#include "pal4/effect_handle.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IEffectHandleMutationRuntime {
    static constexpr std::uint32_t kOwner = 0x5000u;
    static constexpr std::uint32_t kHead = 0x6000u;
    static constexpr std::uint32_t kNode = 0x6100u;
    std::map<std::uint32_t, std::uint32_t> words{{kOwner + 16u, kHead}};
    std::vector<std::string> calls;
    bool valid = true;
    bool registered = true;
    std::uint32_t candidate = kHead;
    pal4::EffectHandleVectorProperty last_property =
        pal4::EffectHandleVectorProperty::Rotation;
    std::array<float, 3u> last_vector{};
    bool last_additive = false;

    std::uint32_t InitializeSystem() override {
        calls.emplace_back("initialize");
        return 0x7000u;
    }
    bool Validate(std::uint32_t system, std::uint32_t owner) override {
        Check(system == 0x7000u && owner == kOwner,
              "validation receives system and handle owner");
        calls.emplace_back("validate");
        return valid;
    }
    std::uint32_t FindElement(
        std::uint32_t tree,
        const pal4::EffectSystemHandleWords& handle) override {
        Check(tree == kOwner + 12u && handle.system == kOwner,
              "lookup receives owner's embedded tree and handle");
        calls.emplace_back("find");
        return candidate;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t RegisterSystem() override {
        calls.emplace_back("register");
        return 0x8000u;
    }
    bool ContainsOwner(std::uint32_t system,
                       std::uint32_t owner) override {
        Check(system == 0x8000u && owner == kOwner,
              "property lookup checks the registered owner");
        calls.emplace_back("contains");
        return registered;
    }
    std::string ReadNameAttribute(std::uint32_t attributes) override {
        Check(attributes == 0x7014u,
              "name comes from dialog attribute storage");
        calls.emplace_back("name");
        return "Dialog A";
    }
    std::int32_t ReadConfigInt(std::uint32_t attributes,
                               std::string_view key) override {
        Check(attributes == 0x7014u && key == "Count",
              "integer read receives dialog attributes and key");
        calls.emplace_back("read config");
        return -17;
    }
    void WriteConfigInt(std::uint32_t attributes,
                        std::string_view key,
                        std::int32_t value) override {
        Check(attributes == 0x7014u && key.empty() && value == 23,
              "integer write accepts the native empty-key fallback");
        calls.emplace_back("write config");
    }
    std::uint8_t ApplyVector(
        pal4::EffectHandleVectorProperty property, std::uint32_t dialog,
        const std::array<float, 3u>& value, bool additive) override {
        Check(dialog == 0x7000u, "vector setter receives matched dialog");
        last_property = property;
        last_vector = value;
        last_additive = additive;
        calls.emplace_back("vector");
        return 0x7bu;
    }
    std::uint8_t ApplyParentFrame(std::uint32_t dialog,
                                  std::uint32_t frame) override {
        Check(dialog == 0x7000u && frame == 0x9100u,
              "parent frame setter receives dialog and frame");
        calls.emplace_back("parent");
        return 0x45u;
    }
    std::uint8_t ApplyVisibility(std::uint32_t dialog,
                                 std::uint32_t visibility) override {
        Check(dialog == 0x7000u && visibility == 3u,
              "visibility setter receives dialog and value");
        calls.emplace_back("visibility");
        return 0x46u;
    }
    std::uint8_t ApplyFacet(std::uint32_t dialog,
                            std::uint32_t first,
                            std::uint32_t second) override {
        Check(dialog == 0x7000u && first == 0x9200u &&
              second == 0x9300u,
              "facet setter receives both arguments and dialog");
        calls.emplace_back("facet");
        return 0x52u;
    }
    std::uint8_t PausePlayback(std::uint32_t dialog) override {
        Check(dialog == 0x7000u,
              "pause receives the matching Dialog object");
        calls.emplace_back("pause");
        return 0x63u;
    }
};

struct ReleaseRuntime final : pal4::IEffectHandleContentRuntime {
    static constexpr std::uint32_t kOwner = Runtime::kOwner;
    static constexpr std::uint32_t kHead = Runtime::kHead;
    static constexpr std::uint32_t kNode = Runtime::kNode;
    std::map<std::uint32_t, std::uint32_t> words{
        {kOwner + 16u, kHead}, {kNode + 12u, 12u},
        {kNode + 16u, 0x7000u}, {kNode + 20u, 2u},
        {0x7000u + 172u, 0x7100u},
        {kOwner + 32u, 0x9000u},
        {0x9000u, 0x9000u}, {0x9000u + 4u, 0x93cdc0u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool registered = true;
    std::uint32_t candidate = kNode;
    std::uint32_t predecessor = 0u;
    std::uint32_t inserted_parent = 0u;

    std::uint32_t InitializeSystem() override { return 0x8000u; }
    bool Validate(std::uint32_t, std::uint32_t) override { return true; }
    std::uint32_t RegisterSystem() override {
        calls.emplace_back("register");
        return 0x8000u;
    }
    bool ContainsOwner(std::uint32_t system,
                       std::uint32_t owner) override {
        Check(system == 0x8000u && owner == kOwner,
              "release validates the registered owner");
        calls.emplace_back("contains");
        return registered;
    }
    std::uint32_t FindElement(
        std::uint32_t tree,
        const pal4::EffectSystemHandleWords& handle) override {
        Check(tree == kOwner + 12u && handle.id == 12u,
              "release selects the owner's handle entry");
        calls.emplace_back("find");
        return candidate;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::string ReadNameAttribute(std::uint32_t) override { return {}; }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == kNode + 20u,
              "release updates only the entry reference count");
        words[address] = value;
        calls.push_back("count " + std::to_string(value));
    }
    void ResetDialogRuntimeState(std::uint32_t dialog) override {
        Check(dialog == 0x7000u, "release resets the matching Dialog");
        calls.emplace_back("reset dialog");
    }
    void DispatchOwnerRelease(
        std::uint32_t owner, pal4::EffectSystemHandleWords handle,
        std::uint32_t mode) override {
        Check(owner == kOwner && handle.id == 12u &&
              handle.system == kOwner && mode == 0u,
              "owner callback receives copied live handle and zero mode");
        calls.emplace_back("dispatch");
    }
    void ReturnEffect(std::uint32_t pool,
                      std::uint32_t dialog,
                      std::uint32_t mode) override {
        Check(pool == 0x8054u && dialog == 0x7000u && mode == 0u,
              "release returns Dialog to registered system pool");
        calls.emplace_back("return effect");
    }
    void EraseEntry(std::uint32_t tree, std::uint32_t node) override {
        Check(tree == kOwner + 12u && node == kNode,
              "release removes the selected embedded-tree entry");
        calls.emplace_back("erase");
    }
    void ProcessEffectList(std::uint32_t owner) override {
        Check(owner == kOwner, "release processes its owner effect list");
        calls.emplace_back("process");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kOwner + 36u,
              "pending insertion checks duplicate mode byte");
        return bytes[address];
    }
    std::uint32_t PendingNil() override { return 0x93cdc0u; }
    std::uint32_t Predecessor(std::uint32_t node) override {
        Check(node == 0x9200u,
              "pending lookup asks for search-parent predecessor");
        calls.emplace_back("predecessor");
        return predecessor;
    }
    void InsertPending(std::uint32_t tree, std::uint32_t child,
                       std::uint32_t parent, std::uint32_t dialog,
                       float time) override {
        Check(tree == kOwner + 28u && child == 0x93cdc0u &&
              dialog == 0x7000u && time == 1.25f,
              "pending record carries Dialog and stop time");
        inserted_parent = parent;
        calls.emplace_back("insert");
    }
    std::array<float, 16u> FrameTransform(std::uint32_t frame) override {
        Check(frame == 0x7100u, "content snapshots Dialog frame");
        calls.emplace_back("frame transform");
        std::array<float, 16u> transform{};
        transform[0u] = 1.0f;
        return transform;
    }
    float StopDialogPlayback(std::uint32_t dialog,
                             bool detach_frame) override {
        Check(dialog == 0x7000u && detach_frame,
              "content stops Dialog playback and detaches frame");
        calls.emplace_back("stop");
        return 1.25f;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x7100u, "content resets frame matrix");
        calls.emplace_back("identity");
    }
    void ApplyFrameTransform(
        std::uint32_t frame, const std::array<float, 16u>& transform,
        std::uint32_t mode) override {
        Check(frame == 0x7100u && transform[0u] == 1.0f && mode == 0u,
              "content restores saved frame transform");
        calls.emplace_back("apply transform");
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x7100u, "content updates objects after transform");
        calls.emplace_back("update objects");
    }
};

void TestCopyAndReset() {
    const pal4::EffectSystemHandleWords source{17u, 0xaaaau};
    pal4::EffectSystemHandleWords destination{5u, 6u};
    Check(&pal4::EffectHandle::CopyData(destination, source, 0x5000u) ==
              &destination &&
          destination.id == 17u && destination.system == 0x5000u &&
          source.system == 0xaaaau,
          "copy uses source identifier and explicit owner word");
    pal4::EffectHandle::ResetThunk(destination);
    Check(destination.id == 0xffffffffu && destination.system == 0u,
          "reset thunk clears both handle words");
    destination = {25u, 0x5000u};
    pal4::EffectHandle::Reset(destination);
    Check(destination.id == 0xffffffffu && destination.system == 0u,
          "direct reset clears both handle words");
}

void TestNeedsUpdate() {
    Runtime runtime;
    Check(!pal4::EffectHandle::DialogNeedsUpdate({12u, 0u}, runtime) &&
          runtime.calls.empty(),
          "null owner does not initialize the effect system");

    runtime.valid = false;
    Check(!pal4::EffectHandle::DialogNeedsUpdate(
              {12u, Runtime::kOwner}, runtime) &&
          runtime.calls == (std::vector<std::string>{
              "initialize", "validate"}),
          "invalid owner stops before tree lookup");

    runtime.valid = true;
    runtime.calls.clear();
    Check(!pal4::EffectHandle::DialogNeedsUpdate(
              {12u, Runtime::kOwner}, runtime) &&
          runtime.calls == (std::vector<std::string>{
              "initialize", "validate", "find", "read 20496",
              "read 20496"}),
          "missing candidate selects the header without reading a node key");

    runtime.candidate = Runtime::kNode;
    runtime.words[Runtime::kNode + 12u] = 13u;
    runtime.calls.clear();
    Check(!pal4::EffectHandle::DialogNeedsUpdate(
              {12u, Runtime::kOwner}, runtime),
          "candidate above identifier still selects the header");
    runtime.words[Runtime::kNode + 12u] = 12u;
    runtime.calls.clear();
    Check(pal4::EffectHandle::DialogNeedsUpdate(
              {12u, Runtime::kOwner}, runtime),
          "matching candidate differs from header and needs update");
    runtime.words[Runtime::kNode + 12u] = 11u;
    Check(pal4::EffectHandle::DialogNeedsUpdate(
              {12u, Runtime::kOwner}, runtime),
          "candidate below identifier also needs update");
}

void TestNameAndFrame() {
    Runtime runtime;
    Check(pal4::EffectHandle::GetName({12u, 0u}, runtime).empty() &&
          pal4::EffectHandle::GetFrame({12u, 0u}, runtime) == 0u &&
          runtime.calls.empty(),
          "null owner yields empty name and frame without registration");

    runtime.registered = false;
    Check(pal4::EffectHandle::GetName(
              {12u, Runtime::kOwner}, runtime).empty() &&
          runtime.calls == (std::vector<std::string>{
              "register", "contains"}),
          "unregistered owner stops before dialog lookup");

    runtime.registered = true;
    runtime.calls.clear();
    Check(pal4::EffectHandle::GetName(
              {12u, Runtime::kOwner}, runtime).empty() &&
          runtime.calls.back() == "read 20496",
          "header candidate yields an empty name");

    runtime.candidate = Runtime::kNode;
    runtime.words[Runtime::kNode + 12u] = 12u;
    runtime.words[Runtime::kNode + 16u] = 0x7000u;
    runtime.words[0x7000u + 172u] = 0xbeefu;
    runtime.calls.clear();
    Check(pal4::EffectHandle::GetName(
              {12u, Runtime::kOwner}, runtime) == "Dialog A" &&
          runtime.calls.back() == "name",
          "matched dialog reads Name from its attribute set");
    runtime.calls.clear();
    Check(pal4::EffectHandle::GetFrame(
              {12u, Runtime::kOwner}, runtime) == 0xbeefu &&
          runtime.calls.back() == "read 28844",
          "matched dialog returns its +172 frame pointer");
}

void TestMutations() {
    Runtime runtime;
    const pal4::EffectSystemHandleWords handle{12u, Runtime::kOwner};
    const std::array<float, 3u> vector{1.0f, 2.0f, 3.0f};
    runtime.candidate = Runtime::kNode;
    runtime.words[Runtime::kNode + 12u] = 12u;
    runtime.words[Runtime::kNode + 16u] = 0x7000u;
    Check(pal4::EffectHandle::SetRotation(handle, vector, true, runtime) ==
              0x7bu &&
          runtime.last_property ==
              pal4::EffectHandleVectorProperty::Rotation &&
          runtime.last_vector == vector && runtime.last_additive,
          "rotation forwards vector and additive mode");
    Check(pal4::EffectHandle::SetScale(handle, vector, false, runtime) ==
              0x7bu &&
          runtime.last_property ==
              pal4::EffectHandleVectorProperty::Scale &&
          !runtime.last_additive,
          "scale forwards vector and absolute mode");
    Check(pal4::EffectHandle::SetPosition(handle, vector, true, runtime) ==
              0x7bu &&
          runtime.last_property ==
              pal4::EffectHandleVectorProperty::Position,
          "position forwards to the matched dialog");
    Check(pal4::EffectHandle::SetParentFrame(
              handle, 0x9100u, runtime) == 0x45u &&
          runtime.calls.back() == "parent",
          "parent frame setter preserves the delegated low-byte return");
    Check(pal4::EffectHandle::SetVisibility(
              handle, 3u, runtime) == 0x46u &&
          runtime.calls.back() == "visibility",
          "visibility setter preserves the delegated low-byte return");

    runtime.candidate = 0x6005u;
    runtime.words[Runtime::kOwner + 16u] = 0x6005u;
    runtime.calls.clear();
    Check(pal4::EffectHandle::SetRotation(handle, vector, true, runtime) ==
              5u &&
          pal4::EffectHandle::SetParentFrame(
              handle, 0x9100u, runtime) == 5u &&
          pal4::EffectHandle::SetVisibility(handle, 3u, runtime) == 5u &&
          runtime.calls.end() == std::find(runtime.calls.begin(),
                                          runtime.calls.end(), "vector"),
          "missing dialog returns the header's low byte without mutation");
    runtime.registered = false;
    Check(pal4::EffectHandle::SetScale(handle, vector, false, runtime) == 0u,
          "unregistered owner suppresses transform mutation");
}

void TestConfigAndFacet() {
    Runtime runtime;
    const pal4::EffectSystemHandleWords handle{12u, Runtime::kOwner};
    Check(pal4::EffectHandle::GetConfigInt(handle, "Count", runtime) == 0 &&
          pal4::EffectHandle::SetConfigInt(handle, "", 23, runtime) == 0u &&
          pal4::EffectHandle::SetFacet(
              handle, 0x9200u, 0x9300u, runtime) == 0u,
          "missing dialog does not read or write config or facet");
    runtime.candidate = Runtime::kNode;
    runtime.words[Runtime::kNode + 12u] = 12u;
    runtime.words[Runtime::kNode + 16u] = 0x7000u;
    runtime.calls.clear();
    Check(pal4::EffectHandle::GetConfigInt(
              handle, "Count", runtime) == -17 &&
          runtime.calls.back() == "read config",
          "config getter returns converted integer from matching dialog");
    Check(pal4::EffectHandle::SetConfigInt(
              handle, "", 23, runtime) == 1u &&
          runtime.calls.back() == "write config",
          "config setter writes integer and reports success");
    Check(pal4::EffectHandle::SetFacet(
              handle, 0x9200u, 0x9300u, runtime) == 0x52u &&
          runtime.calls.back() == "facet",
          "facet setter preserves delegated low-byte result");
    runtime.registered = false;
    runtime.calls.clear();
    Check(pal4::EffectHandle::GetConfigInt(handle, "Count", runtime) == 0 &&
          pal4::EffectHandle::SetConfigInt(handle, "", 23, runtime) == 0u &&
          pal4::EffectHandle::SetFacet(
              handle, 0x9200u, 0x9300u, runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{
              "register", "contains", "register", "contains",
              "register", "contains"}),
          "invalid owner stops before all config and facet operations");
}

void TestPausePlayback() {
    Runtime runtime;
    const pal4::EffectSystemHandleWords handle{12u, Runtime::kOwner};
    Check(pal4::EffectHandle::PausePlayback(handle, runtime) == 0u,
          "missing Dialog returns the head's low byte");
    runtime.candidate = Runtime::kNode;
    runtime.words[Runtime::kNode + 12u] = 12u;
    runtime.words[Runtime::kNode + 16u] = 0x7000u;
    Check(pal4::EffectHandle::PausePlayback(handle, runtime) == 0x63u &&
          runtime.calls.back() == "pause",
          "matching Dialog receives pause and returns its low byte");
    runtime.registered = false;
    Check(pal4::EffectHandle::PausePlayback(handle, runtime) == 0u,
          "unregistered owner cannot pause playback");
}

void TestRelease() {
    ReleaseRuntime runtime;
    pal4::EffectSystemHandleWords empty{12u, 0u};
    pal4::EffectHandle::Release(empty, false, runtime);
    Check(empty.id == 12u && empty.system == 0u && runtime.calls.empty(),
          "null owner release leaves handle unchanged");

    runtime.registered = false;
    pal4::EffectSystemHandleWords invalid{12u, ReleaseRuntime::kOwner};
    pal4::EffectHandle::Release(invalid, false, runtime);
    Check(invalid.id == 12u &&
          runtime.calls == (std::vector<std::string>{
              "register", "contains"}),
          "unregistered owner leaves handle unchanged");

    runtime.registered = true;
    runtime.candidate = ReleaseRuntime::kHead;
    runtime.calls.clear();
    pal4::EffectHandle::Release(invalid, false, runtime);
    Check(invalid.id == 0xffffffffu && invalid.system == 0u &&
          runtime.calls == (std::vector<std::string>{
              "register", "contains", "find"}),
          "missing entry resets handle without releasing a Dialog");

    runtime.candidate = ReleaseRuntime::kNode;
    runtime.calls.clear();
    pal4::EffectSystemHandleWords retained{12u, ReleaseRuntime::kOwner};
    pal4::EffectHandle::Release(retained, false, runtime);
    Check(retained.id == 12u && retained.system == ReleaseRuntime::kOwner &&
          runtime.words.at(ReleaseRuntime::kNode + 20u) == 1u &&
          runtime.calls == (std::vector<std::string>{
              "register", "contains", "find", "count 1"}),
          "positive count decrements without dispatching cleanup");

    runtime.calls.clear();
    pal4::EffectHandle::Release(retained, false, runtime);
    Check(retained.id == 0xffffffffu && retained.system == 0u &&
          runtime.words.at(ReleaseRuntime::kNode + 20u) == 0u &&
          runtime.calls == (std::vector<std::string>{
              "register", "contains", "find", "count 0",
              "reset dialog", "dispatch", "register",
              "return effect", "erase", "process"}),
          "last reference releases Dialog then resets handle");

    ReleaseRuntime forced;
    pal4::EffectSystemHandleWords handle{12u, ReleaseRuntime::kOwner};
    pal4::EffectHandle::Release(handle, true, forced);
    Check(forced.words.at(ReleaseRuntime::kNode + 20u) == 0u &&
          handle.id == 0xffffffffu && handle.system == 0u,
          "forced release skips remaining reference count");
}

void TestContentUpdate() {
    ReleaseRuntime empty;
    pal4::EffectSystemHandleWords absent{12u, 0u};
    pal4::EffectHandle::UpdateContent(absent, empty);
    Check(absent.id == 12u && empty.calls.empty(),
          "null content owner leaves handle unchanged");

    empty.candidate = ReleaseRuntime::kHead;
    pal4::EffectSystemHandleWords missing{12u, ReleaseRuntime::kOwner};
    pal4::EffectHandle::UpdateContent(missing, empty);
    Check(missing.id == 0xffffffffu && missing.system == 0u &&
          empty.calls == (std::vector<std::string>{
              "register", "contains", "find"}),
          "missing Dialog resets handle without changing frame");

    ReleaseRuntime inserted;
    pal4::EffectSystemHandleWords handle{12u, ReleaseRuntime::kOwner};
    pal4::EffectHandle::UpdateContent(handle, inserted);
    Check(handle.id == 0xffffffffu && handle.system == 0u &&
          inserted.inserted_parent == 0x9000u &&
          inserted.calls == (std::vector<std::string>{
              "register", "contains", "find", "frame transform",
              "stop", "identity", "apply transform", "update objects",
              "dispatch", "erase", "insert"}),
          "content update restores frame and inserts completion record");

    ReleaseRuntime duplicate;
    duplicate.words[0x9000u + 4u] = 0x9200u;
    duplicate.words[0x9000u] = 0x9200u;
    duplicate.words[0x9200u + 12u] = 0x7000u;
    duplicate.words[0x9200u + 8u] = 0x93cdc0u;
    handle = {12u, ReleaseRuntime::kOwner};
    pal4::EffectHandle::UpdateContent(handle, duplicate);
    Check(handle.id == 0xffffffffu &&
          std::find(duplicate.calls.begin(), duplicate.calls.end(),
                    "insert") == duplicate.calls.end(),
          "unique pending tree suppresses duplicate Dialog key");

    ReleaseRuntime earlier;
    earlier.words[0x9000u + 4u] = 0x9200u;
    earlier.words[0x9000u] = 0x9100u;
    earlier.words[0x9200u + 12u] = 0x7200u;
    earlier.words[0x9200u] = 0x93cdc0u;
    earlier.words[0x9100u + 12u] = 0x6900u;
    earlier.predecessor = 0x9100u;
    handle = {12u, ReleaseRuntime::kOwner};
    pal4::EffectHandle::UpdateContent(handle, earlier);
    Check(earlier.inserted_parent == 0x9200u &&
          std::find(earlier.calls.begin(), earlier.calls.end(),
                    "predecessor") != earlier.calls.end(),
          "predecessor check keeps original insertion parent");
}

struct ContextRuntime final : pal4::IEffectHandleContextRuntime {
    static constexpr std::uint32_t kOwner = 0x5000u;
    static constexpr std::uint32_t kHead = 0x6000u;
    static constexpr std::uint32_t kNode = 0x6100u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kOwner + 232u, kHead}, {kNode + 12u, 12u},
        {kNode + 16u, 0x7000u}};
    std::vector<std::string> calls;
    std::uint32_t candidate = kNode;
    bool valid = true;
    bool registered = true;

    std::uint32_t InitializeSystem() override {
        calls.emplace_back("initialize");
        return 0x8000u;
    }
    bool Validate(std::uint32_t system, std::uint32_t owner) override {
        Check(system == 0x8000u && owner == kOwner,
              "context validation receives system and owner");
        calls.emplace_back("validate");
        return valid;
    }
    std::uint32_t RegisterSystem() override {
        calls.emplace_back("register");
        return 0x8000u;
    }
    bool ContainsOwner(std::uint32_t system,
                       std::uint32_t owner) override {
        Check(system == 0x8000u && owner == kOwner,
              "context registration checks the same owner");
        calls.emplace_back("contains");
        return registered;
    }
    std::uint32_t FindElement(
        std::uint32_t tree,
        const pal4::EffectSystemHandleWords& handle) override {
        Check(tree == kOwner + 228u && handle.system == kOwner,
              "context lookup uses the fourth embedded tree");
        calls.emplace_back("find");
        return candidate;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::string ReadNameAttribute(std::uint32_t) override { return {}; }
    void StopContextDialog(std::uint32_t dialog) override {
        Check(dialog == 0x7000u, "stop receives the selected Dialog");
        calls.emplace_back("stop");
    }
    void DestroyContextDialog(std::uint32_t dialog) override {
        Check(dialog == 0x7000u, "destroy receives the selected Dialog");
        calls.emplace_back("destroy");
    }
    float ContextDialogTime(std::uint32_t dialog) override {
        Check(dialog == 0x7000u, "time comes from the selected Dialog");
        calls.emplace_back("time");
        return 2.5f;
    }
    void PositionContextDialog(std::uint32_t owner,
                               std::uint32_t dialog, float time) override {
        Check(owner == kOwner && dialog == 0x7000u && time == 2.5f,
              "position receives the owner, Dialog and reported time");
        calls.emplace_back("position");
    }
    void EraseContext(std::uint32_t tree, std::uint32_t node) override {
        Check(tree == kOwner + 228u && node == kNode,
              "erase removes the fourth-tree entry");
        calls.emplace_back("erase");
    }
};

void TestContextOperations() {
    ContextRuntime runtime;
    pal4::EffectSystemHandleWords handle{12u, 0u};
    Check(!pal4::EffectHandle::IsReadyToShow(handle, runtime) &&
          runtime.calls.empty(), "null owner short-circuits context readiness");
    handle = {0xffffffffu, ContextRuntime::kOwner};
    Check(!pal4::EffectHandle::IsReadyToShow(handle, runtime) &&
          runtime.calls.empty(), "negative identifier short-circuits readiness");
    handle.id = 12u;
    runtime.valid = false;
    Check(!pal4::EffectHandle::IsReadyToShow(handle, runtime) &&
          runtime.calls == (std::vector<std::string>{
              "initialize", "validate"}),
          "invalid owner never searches the context tree");
    runtime.valid = true;
    runtime.calls.clear();
    Check(pal4::EffectHandle::IsReadyToShow(handle, runtime) &&
          runtime.calls == (std::vector<std::string>{
              "initialize", "validate", "find"}),
          "matching context reports readiness");
    runtime.candidate = ContextRuntime::kHead;
    Check(!pal4::EffectHandle::IsReadyToShow(handle, runtime),
          "missing context is not ready");
    runtime.candidate = ContextRuntime::kNode;
    runtime.words[ContextRuntime::kNode + 12u] = 13u;
    Check(!pal4::EffectHandle::IsReadyToShow(handle, runtime),
          "larger lower-bound key is not an exact context match");
    runtime.words[ContextRuntime::kNode + 12u] = 12u;

    runtime.calls.clear();
    runtime.registered = false;
    pal4::EffectHandle::RemoveContext(handle, runtime);
    Check(handle.id == 12u && handle.system == ContextRuntime::kOwner &&
          runtime.calls == (std::vector<std::string>{
              "register", "contains"}),
          "unregistered owner retains its context handle");
    runtime.registered = true;
    runtime.calls.clear();
    pal4::EffectHandle::RemoveContext(handle, runtime);
    Check(handle.id == 0xffffffffu && handle.system == 0u &&
          runtime.calls == (std::vector<std::string>{
              "register", "contains", "find", "stop", "destroy", "erase"}),
          "context removal stops and destroys Dialog before erasure");

    handle = {12u, ContextRuntime::kOwner};
    runtime.calls.clear();
    pal4::EffectHandle::DisplayAndCleanup(handle, runtime);
    Check(handle.id == 0xffffffffu && handle.system == 0u &&
          runtime.calls == (std::vector<std::string>{
              "initialize", "validate", "find", "time", "position", "erase"}),
          "display positions Dialog before erasing its context");
    handle = {12u, ContextRuntime::kOwner};
    runtime.candidate = ContextRuntime::kHead;
    runtime.calls.clear();
    pal4::EffectHandle::DisplayAndCleanup(handle, runtime);
    Check(handle.id == 0xffffffffu && handle.system == 0u &&
          runtime.calls == (std::vector<std::string>{
              "initialize", "validate", "find"}),
          "missing context still invalidates a registered handle");
}

}  // namespace

int main() {
    TestCopyAndReset();
    TestNeedsUpdate();
    TestNameAndFrame();
    TestMutations();
    TestConfigAndFacet();
    TestPausePlayback();
    TestRelease();
    TestContentUpdate();
    TestContextOperations();
}
