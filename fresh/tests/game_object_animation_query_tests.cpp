#include "pal4/game_object_animation_query.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectAnimationQueryRuntime {
public:
    std::vector<std::string> calls;
    std::int32_t count = 2;
    std::uint32_t record_name = 0;
    std::uint32_t event_begin = 0x2000u;
    std::uint32_t event_end = 0x2000u + 2u * 0x14cu;
    std::uint32_t match_name_address = 0;

    std::uint32_t SelectedObjectName(std::uint32_t object) override {
        Check(object == 0x77u, "gets selected object's virtual name");
        calls.emplace_back("selected-name");
        return 0xabcu;
    }
    std::uint32_t EmptyErrorString() override {
        calls.emplace_back("empty-error");
        return 0xe1u;
    }
    std::uint32_t EmptyLogMessage() override {
        calls.emplace_back("empty-log");
        return 0xe2u;
    }
    std::uint32_t RecordName(std::uint32_t record) override {
        Check(record == 0x1000u, "reads current record name at +20");
        calls.emplace_back("record-name");
        return record_name;
    }
    std::int32_t AnimationCount(
        pal4::GameObjectEmbeddedAnimationState&) override {
        calls.emplace_back("count");
        return count;
    }
    std::uint32_t AnimationEntry(
        pal4::GameObjectEmbeddedAnimationState&,
        std::int32_t index) override {
        calls.emplace_back("entry:" + std::to_string(index));
        return 0x3000u + static_cast<std::uint32_t>(index) * 0x20u;
    }
    std::uint32_t EventBegin(std::uint32_t record) override {
        Check(record == 0x1000u, "reads event begin from record +40");
        calls.emplace_back("begin");
        return event_begin;
    }
    std::uint32_t EventEnd(std::uint32_t record) override {
        Check(record == 0x1000u, "reads event end from record +44");
        calls.emplace_back("end");
        return event_end;
    }
    int CompareEventName(std::uint32_t stored,
                         std::uint32_t requested) override {
        Check(requested == 0x999u, "forwards requested event name");
        calls.emplace_back("compare:" + std::to_string(stored));
        return stored == match_name_address ? 0 : 1;
    }
};

void NamesAndCurrentData() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    Check(pal4::GameObjectAnimationQuery::CurrentAnimationData(state) == 0 &&
              pal4::GameObjectAnimationQuery::SelectedAnimationName(
                  state, runtime) == 0xe1u &&
              pal4::GameObjectAnimationQuery::CurrentRecordName(
                  state, runtime) == 0xe1u &&
              runtime.calls == std::vector<std::string>{
                  "empty-error", "empty-error"},
          "missing selected object and record use error-string fallback");
    runtime.calls.clear();
    state.words[10] = 0x77u;
    state.words[3] = 0x1000u;
    Check(pal4::GameObjectAnimationQuery::CurrentAnimationData(state) ==
                  0x1034u &&
              pal4::GameObjectAnimationQuery::SelectedAnimationName(
                  state, runtime) == 0xabcu &&
              pal4::GameObjectAnimationQuery::CurrentRecordName(
                  state, runtime) == 0xe2u &&
              runtime.calls == std::vector<std::string>{
                  "selected-name", "record-name", "empty-log"},
          "present record adds 52 and null record name uses log fallback");
    runtime.calls.clear();
    runtime.record_name = 0x123u;
    Check(pal4::GameObjectAnimationQuery::CurrentRecordName(
              state, runtime) == 0x123u &&
              runtime.calls == std::vector<std::string>{"record-name"},
          "present record name is forwarded");
}

void TraverseAndFind() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    int visited = 0;
    auto callback = [&](std::uint32_t entry, std::int32_t index,
                        std::uint32_t context) {
        Check(entry == 0x3000u + static_cast<std::uint32_t>(index) * 0x20u &&
                  context == 0x55u,
              "visitor receives entry, index and context");
        ++visited;
        return index != 0;
    };
    runtime.count = 0;
    Check(!pal4::GameObjectAnimationQuery::ProcessAnimations(
              state, callback, 0x55u, runtime) && visited == 0,
          "empty manager rejects traversal");
    runtime.count = 2;
    runtime.calls.clear();
    Check(!pal4::GameObjectAnimationQuery::ProcessAnimations(
              state, {}, 0x55u, runtime) &&
              runtime.calls == std::vector<std::string>{"count"},
          "missing callback rejects traversal after count check");
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationQuery::ProcessAnimations(
              state, callback, 0x55u, runtime) && visited == 1 &&
              runtime.calls == std::vector<std::string>{
                  "count", "count", "entry:0"},
          "callback false stops traversal but returns success");
    visited = 0;
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationQuery::ProcessAnimations(
              state,
              [&](std::uint32_t, std::int32_t, std::uint32_t) {
                  ++visited;
                  return true;
              }, 0x55u, runtime) && visited == 2,
          "visitor walks all manager entries when callback continues");

    runtime.calls.clear();
    Check(pal4::GameObjectAnimationQuery::FindEvent(
              state, 0x999u, runtime) == 0 && runtime.calls.empty(),
          "event lookup requires current record");
    state.words[3] = 0x1000u;
    runtime.event_end = runtime.event_begin;
    Check(pal4::GameObjectAnimationQuery::FindEvent(
              state, 0x999u, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"begin", "end"},
          "empty event range stops before comparison");
    runtime.event_end = runtime.event_begin + 2u * 0x14cu;
    runtime.calls.clear();
    runtime.match_name_address = runtime.event_begin + 0x80u;
    Check(pal4::GameObjectAnimationQuery::FindEvent(
              state, 0x999u, runtime) == 0x2000u &&
              runtime.calls.back() == "compare:8320",
          "first event matches name at offset 128");
    runtime.calls.clear();
    runtime.match_name_address = runtime.event_begin + 0x14cu + 0x80u;
    Check(pal4::GameObjectAnimationQuery::FindEvent(
              state, 0x999u, runtime) == 0x214cu &&
              runtime.calls.back() == "compare:8652",
          "second event follows 332-byte stride");
    runtime.calls.clear();
    runtime.match_name_address = 0;
    Check(pal4::GameObjectAnimationQuery::FindEvent(
              state, 0x999u, runtime) == 0 &&
              runtime.calls.back() == "end",
          "unmatched event range returns zero");
}

}  // namespace

int main() {
    NamesAndCurrentData();
    TraverseAndFind();
    std::cout << "game_object_animation_query_tests passed\n";
}
