#include "pal4/rump_shahdow_task.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
void Put(pal4::RumpShahdowTaskState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}
float Get(const pal4::RumpShahdowTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
bool Near(float a, float b) { return std::fabs(a - b) < 0.0001f; }

struct Runtime final : pal4::IRumpShahdowTaskRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> order;
    std::map<std::uint32_t, pal4::RumpShahdowRecord> records;
    std::map<std::uint32_t, pal4::RumpShahdowColor> colors;
    pal4::RumpShahdowColor owner_color{10u, 20u, 30u, 200u};
    pal4::RumpShahdowTransform transform{};
    std::uint32_t next_record = 0x7000u;
    std::uint32_t missing_facet = 0u;
    std::int32_t failed_child = -1;
    bool valid_count = true;
    bool frame_parent = true;
    std::uint32_t float_calls = 0u;
    std::uint32_t removed_frames = 0u;
    std::uint32_t released_records = 0u;

    void ConstructBase(pal4::RumpShahdowTaskState& state) override {
        calls.emplace_back("construct"); state.words[0u] = 0xdeadbeefu;
    }
    void DestroyBase(pal4::RumpShahdowTaskState& state) override {
        Check(state.words[0u] == pal4::RumpShahdowTask::kVtable &&
              state.words[25u] == 0u && state.words[26u] == 0u &&
              state.words[27u] == 0u, "base cleanup sees cleared vector");
        calls.emplace_back("destroy");
    }
    void FreeCollectionStorage(pal4::RumpShahdowTaskState&) override {
        calls.emplace_back("free-storage");
    }
    std::uint32_t ResolveFacet(std::uint32_t object,
                                std::string_view name) override {
        calls.emplace_back("resolve:" + std::string(name));
        if (object == 0x2000u && name == "RumpShahdowTask")
            return missing_facet == 1u ? 0u : 0x3000u;
        if (object == 0x2000u && name == "ChangeColorTask")
            return missing_facet == 2u ? 0u : 0x4000u;
        if (name == "ChangeColorTask") return object + 0x1000u;
        if (name == "ShowobjTask") return object + 0x2000u;
        Check(false, "unexpected facet class"); return 0u;
    }
    void PrepareOwner(std::uint32_t owner) override {
        Check(owner == 0x2000u, "owner slot 32");
        calls.emplace_back("prepare");
    }
    pal4::RumpShahdowColor ReadColor(std::uint32_t facet) override {
        calls.emplace_back("read-color");
        if (facet == 0x4000u) return owner_color;
        return colors.at(facet);
    }
    void WriteColor(std::uint32_t facet,
                    const pal4::RumpShahdowColor& value) override {
        calls.emplace_back("write-color"); colors[facet] = value;
    }
    bool ValidateCount(std::uint32_t facet, std::int32_t count) override {
        Check(facet == 0x3000u && count >= 0, "validate requested count");
        calls.emplace_back("validate"); return valid_count;
    }
    std::uint32_t CreateChild(std::uint32_t facet,
                              std::int32_t index) override {
        Check(facet == 0x3000u, "create child via task facet");
        calls.emplace_back("create");
        return index == failed_child ? 0u : 0x5000u +
               static_cast<std::uint32_t>(index);
    }
    std::uint32_t AllocateRecord(std::uint32_t bytes) override {
        Check(bytes == 0xcu, "record allocates 12 bytes");
        calls.emplace_back("alloc-record");
        const auto result = next_record; next_record += 0x10u; return result;
    }
    void FreeRecord(std::uint32_t record) override {
        calls.emplace_back("free-record"); records.erase(record);
        ++released_records;
    }
    pal4::RumpShahdowRecord ReadRecord(std::uint32_t record) override {
        return records.at(record);
    }
    void WriteRecord(std::uint32_t record,
                     const pal4::RumpShahdowRecord& value) override {
        records[record] = value;
    }
    void AppendRecord(pal4::RumpShahdowTaskState& state,
                      std::uint32_t record) override {
        calls.emplace_back("append"); order.push_back(record);
        state.words[25u] = 0x9000u;
        state.words[26u] = 0x9000u + static_cast<std::uint32_t>(order.size() * 4u);
        state.words[27u] = 0x9040u;
    }
    std::uint32_t RecordCount(const pal4::RumpShahdowTaskState&) override {
        return static_cast<std::uint32_t>(order.size());
    }
    std::uint32_t RecordAt(const pal4::RumpShahdowTaskState&,
                           std::uint32_t index) override {
        return order.at(index);
    }
    std::uint32_t MoveLastRecordToFront(
        pal4::RumpShahdowTaskState&) override {
        calls.emplace_back("rotate-record");
        const auto last = order.back(); order.pop_back();
        order.insert(order.begin(), last); return last;
    }
    void ClearRecords(pal4::RumpShahdowTaskState& state) override {
        calls.emplace_back("clear-records"); order.clear();
        state.words[26u] = state.words[25u];
    }
    void SetShowFacet(std::uint32_t, bool shown) override {
        calls.emplace_back(shown ? "show" : "hide");
    }
    void DispatchChildEvent(std::uint32_t, bool retiring) override {
        calls.emplace_back(retiring ? "retire" : "activate");
    }
    pal4::RumpShahdowTransform ReadOwnerTransform(
        std::uint32_t owner) override {
        Check(owner == 0x2000u, "transform owner");
        calls.emplace_back("owner-transform"); return transform;
    }
    std::uint32_t ChildFrame(std::uint32_t child) override {
        calls.emplace_back("child-frame"); return child + 0x3000u;
    }
    bool FrameHasParent(std::uint32_t) override {
        calls.emplace_back("frame-parent"); return frame_parent;
    }
    void RemoveFrameChild(std::uint32_t) override {
        calls.emplace_back("remove-frame"); ++removed_frames;
    }
    void FrameSetIdentity(std::uint32_t) override {
        calls.emplace_back("frame-identity");
    }
    void FrameApplyTransform(std::uint32_t,
                             const pal4::RumpShahdowTransform&) override {
        calls.emplace_back("frame-transform");
    }
    void FrameUpdateObjects(std::uint32_t) override {
        calls.emplace_back("frame-update");
    }
    std::int32_t FloatToInt(double value) override {
        ++float_calls; return static_cast<std::int32_t>(value);
    }
};

pal4::RumpShahdowTaskState ConfiguredState(std::int32_t count = 2) {
    pal4::RumpShahdowTaskState state{};
    state.words[16u] = 0x2000u;
    Put(state, 20u, 0.5f);
    Put(state, 21u, 1.0f / 24.0f);
    state.words[23u] = static_cast<std::uint32_t>(count);
    return state;
}

void ExpectThrow(auto&& operation, const char* message) {
    bool caught = false;
    try { operation(); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught, message);
}

}  // namespace

int main() {
    Runtime runtime;
    pal4::RumpShahdowTaskState constructed{};
    constructed.words.fill(0xabcdef01u);
    Check(&pal4::RumpShahdowTask::Construct(
              constructed, 0x5au, runtime) == &constructed &&
          constructed.words[0u] == pal4::RumpShahdowTask::kVtable &&
          constructed.words[18u] == 0u && constructed.words[20u] == 0u &&
          constructed.words[21u] == 0u && constructed.words[23u] == 0u &&
          (constructed.words[24u] & 0xffu) == 0x5au &&
          constructed.words[25u] == 0u && constructed.words[26u] == 0u &&
          constructed.words[27u] == 0u &&
          constructed.words[19u] == 0xabcdef01u &&
          runtime.calls == std::vector<std::string>{"construct"},
          "constructor preserves fields and injects unwritten stack byte");
    Check(pal4::RumpShahdowTask::ClassName() == "RumpShahdowTask",
          "native class spelling");

    auto state = ConfiguredState();
    runtime.calls.clear();
    Check(pal4::RumpShahdowTask::InitializeShadows(state, runtime) == 2 &&
          state.words[18u] == 0x3000u && runtime.order.size() == 2u &&
          runtime.calls == std::vector<std::string>{
              "resolve:RumpShahdowTask", "prepare", "resolve:ChangeColorTask",
              "read-color", "validate", "alloc-record", "create",
              "resolve:ChangeColorTask", "write-color", "append",
              "alloc-record", "create", "resolve:ChangeColorTask",
              "write-color", "append"},
          "initialization resolves facets and creates records in native order");
    for (std::size_t i = 0; i < 2u; ++i) {
        const auto record = runtime.records.at(runtime.order[i]);
        Check(Near(record.age, 0.5f) && record.original_alpha == 200u &&
              record.child == 0x5000u + i &&
              runtime.colors.at(record.child + 0x1000u) ==
                  pal4::RumpShahdowColor{10u, 20u, 30u, 0u},
              "new shadow starts transparent and fully aged");
    }
    runtime.calls.clear();
    Check(pal4::RumpShahdowTask::ActivateShadows(state, runtime) == 1u &&
          ((state.words[28u] >> 8u) & 0xffu) == 1u &&
          state.words[22u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "resolve:ShowobjTask", "show", "activate",
              "resolve:ShowobjTask", "show", "activate"},
          "activate shows each child and primes first-frame capture");

    state.words[28u] |= 1u;
    runtime.transform[0u] = 1.0f;
    runtime.transform[5u] = 1.0f;
    runtime.transform[10u] = 1.0f;
    runtime.calls.clear();
    Check(pal4::RumpShahdowTask::Update(state, 0.02f, runtime) == 1u &&
          Near(Get(state, 22u), 0.02f) && runtime.float_calls == 2u &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "rotate-record") == runtime.calls.end(),
          "interval gate fades records without spawning");
    runtime.calls.clear();
    Check(pal4::RumpShahdowTask::Update(state, 0.03f, runtime) == 1u &&
          state.words[22u] == 0u && runtime.order.front() == 0x7010u &&
          ((state.words[28u] >> 8u) & 0xffu) == 0u &&
          runtime.colors.at(0x6001u)[3u] == 188u &&
          runtime.colors.at(0x6000u)[3u] == 0u,
          "spawn reuses last record, captures transform and fades by age");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "frame-identity") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "frame-transform") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "frame-update") != runtime.calls.end(),
          "RenderWare frame stubs called in spawn path");
    const auto last_order = runtime.order;
    runtime.calls.clear();
    (void)pal4::RumpShahdowTask::Update(state, 0.05f, runtime);
    Check(runtime.order == last_order &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "rotate-record") == runtime.calls.end(),
          "unchanged RGB and transform skip respawn");
    runtime.transform[0u] += 0.02f;
    runtime.calls.clear();
    (void)pal4::RumpShahdowTask::Update(state, 0.05f, runtime);
    Check(runtime.order.front() == 0x7000u &&
          runtime.colors.at(0x6001u)[3u] == 74u &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "rotate-record") != runtime.calls.end(),
          "transform delta respawns and second record gets index fade");
    runtime.owner_color[0u] = 11u;
    runtime.calls.clear();
    (void)pal4::RumpShahdowTask::Update(state, 0.05f, runtime);
    Check(runtime.order.front() == 0x7010u,
          "RGB change respawns even with stable transform");
    runtime.transform[0u] += 0.005f;
    const auto near_order = runtime.order;
    runtime.calls.clear();
    (void)pal4::RumpShahdowTask::Update(state, 0.05f, runtime);
    Check(runtime.order == near_order &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "rotate-record") == runtime.calls.end(),
          "transform delta inside 0.01 threshold skips respawn");

    runtime.calls.clear();
    const auto prior_end = state.words[26u];
    Check(pal4::RumpShahdowTask::ClearShadows(state, runtime) == prior_end &&
          runtime.order.empty() && runtime.records.empty() &&
          state.words[18u] == 0u && state.words[26u] == state.words[25u] &&
          runtime.removed_frames == 0u && runtime.released_records == 2u,
          "clear hides, retires and frees records without frame removal");
    Check(runtime.calls == std::vector<std::string>{
              "resolve:ShowobjTask", "hide", "retire", "free-record",
              "resolve:ShowobjTask", "hide", "retire", "free-record",
              "clear-records"}, "clear call order");
    runtime.calls.clear();
    Check(&pal4::RumpShahdowTask::Destroy(state, runtime) == &state &&
          runtime.calls == std::vector<std::string>{
              "clear-records", "free-storage", "destroy"} &&
          state.words[25u] == 0u && state.words[26u] == 0u &&
          state.words[27u] == 0u, "destructor frees collection after clear");

    Runtime stopped;
    auto stop_state = ConfiguredState();
    (void)pal4::RumpShahdowTask::InitializeShadows(stop_state, stopped);
    stop_state.words[28u] |= 0x100u;
    stopped.calls.clear();
    Check(pal4::RumpShahdowTask::Stop(stop_state, stopped) == 1u &&
          stopped.order.empty() && stopped.removed_frames == 2u &&
          stopped.released_records == 2u &&
          ((stop_state.words[28u] >> 8u) & 0xffu) == 0u &&
          stop_state.words[18u] == 0u,
          "stop removes child frames before retirement");
    const auto hide = std::find(stopped.calls.begin(), stopped.calls.end(), "hide");
    const auto remove = std::find(stopped.calls.begin(), stopped.calls.end(),
                                  "remove-frame");
    const auto retire = std::find(stopped.calls.begin(), stopped.calls.end(),
                                  "retire");
    Check(hide < remove && remove < retire, "stop frame cleanup order");

    Runtime detached;
    detached.frame_parent = false;
    auto detached_state = ConfiguredState(1);
    (void)pal4::RumpShahdowTask::InitializeShadows(detached_state, detached);
    (void)pal4::RumpShahdowTask::Stop(detached_state, detached);
    Check(detached.removed_frames == 0u && detached.released_records == 1u,
          "detached child skips RenderWare remove-child call");

    Runtime failures;
    auto bad = ConfiguredState();
    failures.missing_facet = 1u;
    ExpectThrow([&] { (void)pal4::RumpShahdowTask::InitializeShadows(bad, failures); },
                "missing task facet throws");
    Check(failures.calls == std::vector<std::string>{
              "resolve:RumpShahdowTask"}, "first lookup failure stops early");
    failures = Runtime{};
    failures.missing_facet = 2u;
    bad = ConfiguredState();
    ExpectThrow([&] { (void)pal4::RumpShahdowTask::InitializeShadows(bad, failures); },
                "missing color facet throws");
    failures = Runtime{};
    failures.valid_count = false;
    bad = ConfiguredState();
    ExpectThrow([&] { (void)pal4::RumpShahdowTask::InitializeShadows(bad, failures); },
                "rejected count throws");
    failures = Runtime{};
    failures.failed_child = 1;
    bad = ConfiguredState();
    ExpectThrow([&] { (void)pal4::RumpShahdowTask::InitializeShadows(bad, failures); },
                "failed child creation throws");
    Check(failures.order.empty() && failures.records.size() == 1u &&
          failures.released_records == 1u &&
          bad.words[26u] == bad.words[25u],
          "failed child frees current record and resets native collection end");
    failures = Runtime{};
    bad = ConfiguredState(0);
    Check(pal4::RumpShahdowTask::InitializeShadows(bad, failures) == 0 &&
          failures.order.empty(), "zero configured child count");
    ExpectThrow([&] { (void)pal4::RumpShahdowTask::Update(bad, 0.05f, failures); },
                "no reusable child is reported at spawn boundary");
}
