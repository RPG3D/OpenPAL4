#include "pal4/animation_script_record_array.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Call final {
    char kind;
    std::uint32_t first;
    std::uint32_t second;
    bool operator==(const Call&) const = default;
};

class Runtime final : public pal4::IAnimationScriptRecordInsertRuntime {
public:
    std::uint32_t allocation = 0x4000u;
    std::map<std::uint32_t, pal4::AnimationScriptRecordState> records;
    std::vector<Call> calls;

    pal4::AnimationScriptRecordState& RecordAt(
        std::uint32_t address) override {
        calls.push_back({'R', address, 0u});
        return records[address];
    }
    std::uint8_t ReadReference(std::uint32_t address) override {
        calls.push_back({'r', address, 0u});
        return 0;
    }
    void WriteReference(std::uint32_t address,
                        std::uint8_t value) override {
        calls.push_back({'w', address, value});
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back({'F', address, 0u});
        return 0;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back({'A', bytes, 0u});
        return allocation;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({'C', destination, source});
        records[destination] = records[source];
    }
    void AssignString(std::uint32_t destination,
                      std::uint32_t source) override {
        calls.push_back({'S', destination, source});
        auto& target = records[destination & ~31u];
        const auto& original = records[source & ~31u];
        if ((destination & 16u) == 0u) {
            target.first_text = (source & 16u) == 0u
                                    ? original.first_text : original.second_text;
            target.first_length = (source & 16u) == 0u
                                      ? original.first_length : original.second_length;
            target.first_capacity = (source & 16u) == 0u
                                        ? original.first_capacity : original.second_capacity;
        } else {
            target.second_text = (source & 16u) == 0u
                                     ? original.first_text : original.second_text;
            target.second_length = (source & 16u) == 0u
                                       ? original.first_length : original.second_length;
            target.second_capacity = (source & 16u) == 0u
                                         ? original.first_capacity : original.second_capacity;
        }
    }
    void EraseString(std::uint32_t, std::uint32_t,
                     std::uint32_t) override {}
    std::uint32_t EmptyText() override { return 0; }
    bool AllocateText(std::uint32_t, std::uint32_t,
                      bool) override { return false; }
    void CopyText(std::uint32_t, std::uint32_t,
                  std::uint32_t) override {}
    void SetTextSize(std::uint32_t, std::uint32_t) override {}
};

pal4::AnimationScriptRecordState Record(std::uint32_t first,
                                         std::uint32_t second) {
    return {0u, 0u, first, first, 0u, 0u, second, second};
}

}  // namespace

int main() {
    Runtime runtime;
    runtime.records[0x9000u] = Record(7u, 8u);
    pal4::AnimationScriptRecordArrayState state{5u, 0x1000u,
                                                  0x1040u, 0x10a0u};
    pal4::AnimationScriptRecordArray::Insert(
        state, 0x1020u, 0u, 0x9000u, runtime);
    Check(runtime.calls.empty() && state.end == 0x1040u,
          "zero count leaves array unchanged");

    runtime.records[0x1000u] = Record(1u, 2u);
    runtime.records[0x1020u] = Record(3u, 4u);
    pal4::AnimationScriptRecordArray::Insert(
        state, 0x1020u, 2u, 0x9000u, runtime);
    Check(runtime.calls == std::vector<Call>{
              {'C', 0x1060u, 0x1020u}, {'C', 0x1040u, 0x9000u},
              {'S', 0x1020u, 0x9000u}, {'S', 0x1030u, 0x9010u}} &&
              state.end == 0x1080u && state.capacity == 0x10a0u &&
              runtime.records[0x1020u].first_length == 7u &&
              runtime.records[0x1040u].second_length == 8u &&
              runtime.records[0x1060u].first_length == 3u,
          "short tail copies tail out, constructs excess and assigns old slot");

    runtime.calls.clear();
    state = {6u, 0x2000u, 0x2080u, 0x20a0u};
    runtime.records[0x2000u] = Record(1u, 2u);
    runtime.records[0x2020u] = Record(3u, 4u);
    runtime.records[0x2040u] = Record(5u, 6u);
    runtime.records[0x2060u] = Record(9u, 10u);
    pal4::AnimationScriptRecordArray::Insert(
        state, 0x2020u, 1u, 0x9000u, runtime);
    Check(runtime.calls == std::vector<Call>{
              {'C', 0x2080u, 0x2060u},
              {'S', 0x2060u, 0x2040u}, {'S', 0x2070u, 0x2050u},
              {'S', 0x2040u, 0x2020u}, {'S', 0x2050u, 0x2030u},
              {'S', 0x2020u, 0x9000u}, {'S', 0x2030u, 0x9010u}} &&
              state.end == 0x20a0u &&
              runtime.records[0x2040u].first_length == 3u &&
              runtime.records[0x2060u].first_length == 5u &&
              runtime.records[0x2080u].first_length == 9u,
          "long tail shifts backward before filling insertion position");

    runtime.calls.clear();
    state = {7u, 0x3000u, 0x3040u, 0x3040u};
    runtime.records[0x3000u] = Record(1u, 2u);
    runtime.records[0x3020u] = Record(3u, 4u);
    pal4::AnimationScriptRecordArray::Insert(
        state, 0x3020u, 1u, 0x9000u, runtime);
    Check(runtime.calls == std::vector<Call>{
              {'A', 128u, 0u}, {'C', 0x4000u, 0x3000u},
              {'C', 0x4020u, 0x9000u}, {'C', 0x4040u, 0x3020u},
              {'R', 0x3000u, 0u}, {'R', 0x3020u, 0u},
              {'F', 0x3000u, 0u}} &&
              state.reserved == 7u && state.begin == 0x4000u &&
              state.end == 0x4060u && state.capacity == 0x4080u &&
              runtime.records[0x4040u].first_length == 3u,
          "growth copies prefix, insertion and suffix before old cleanup/free");

    runtime.calls.clear();
    state = {8u, 0u, 0u, 0u};
    runtime.allocation = 0x5000u;
    pal4::AnimationScriptRecordArray::Insert(
        state, 0u, 2u, 0x9000u, runtime);
    Check(runtime.calls == std::vector<Call>{
              {'A', 64u, 0u}, {'C', 0x5000u, 0x9000u},
              {'C', 0x5020u, 0x9000u}, {'F', 0u, 0u}} &&
              state.begin == 0x5000u && state.end == 0x5040u &&
              state.capacity == 0x5040u,
          "empty array grows and forwards null free");

    runtime.calls.clear();
    state = {9u, 0u, 0u, 0u};
    runtime.allocation = 0u;
    pal4::AnimationScriptRecordArray::Insert(
        state, 0u, 1u, 0x9000u, runtime);
    Check(runtime.calls == std::vector<Call>{{'A', 32u, 0u}} &&
              state.begin == 0u && state.end == 0u,
          "allocation failure preserves old state");

    runtime.calls.clear();
    state = {10u, 0x1000u, 0x1041u, 0x1080u};
    pal4::AnimationScriptRecordArray::Insert(
        state, 0x1000u, 1u, 0x9000u, runtime);
    Check(runtime.calls.empty() && state.end == 0x1041u,
          "malformed vector bounds terminate safely");

    pal4::AnimationScriptRecordInsertRuntimeStub stub;
    state = {0u, 0u, 0u, 0u};
    pal4::AnimationScriptRecordArray::Insert(state, 0u, 1u, 0u, stub);
    std::cout << "animation_script_record_insert_tests passed\n";
}
