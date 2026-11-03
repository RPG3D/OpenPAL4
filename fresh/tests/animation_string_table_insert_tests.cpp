#include "pal4/animation_string_table.h"

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

class Runtime final : public pal4::AnimationStringTableRuntimeStub {
public:
    pal4::AnimationStringTableState state{};
    std::map<std::uint32_t, pal4::AnimationStringFieldState> fields;
    std::vector<Call> calls;
    std::uint32_t allocation = 0x2000u;

    pal4::AnimationStringTableState& TableAt(std::uint32_t) override {
        return state;
    }
    pal4::AnimationStringFieldState& FieldAt(
        std::uint32_t address) override {
        return fields[address];
    }
    std::uint32_t AllocateRecordArray(std::uint32_t bytes) override {
        calls.push_back({'A', bytes, 0});
        return allocation;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({'C', destination, source});
        fields[destination] = fields[source];
        fields[destination + 16u] = fields[source + 16u];
    }
    void CopyTemplateRecord(
        std::uint32_t destination,
        const pal4::AnimationStringRecordState& record) override {
        calls.push_back({'T', destination, 0});
        fields[destination] = record.name;
        fields[destination + 16u] = record.value;
    }
    void AssignRecordField(std::uint32_t destination,
                           std::uint32_t source) override {
        calls.push_back({'S', destination, source});
        fields[destination] = fields[source];
    }
    void AssignTemplateField(
        std::uint32_t destination,
        const pal4::AnimationStringFieldState& field) override {
        calls.push_back({'s', destination, field.header});
        fields[destination] = field;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back({'F', address, 0});
        return 0;
    }

    void Fill(std::uint32_t begin, std::uint32_t count) {
        fields.clear();
        for (std::uint32_t index = 0; index < count; ++index) {
            fields[begin + index * 32u].header = index + 1u;
            fields[begin + index * 32u + 16u].header = index + 101u;
        }
    }
    std::vector<std::uint32_t> Ids() {
        std::vector<std::uint32_t> result;
        for (auto address = state.begin; address != state.end;
             address += 32u)
            result.push_back(fields[address].header);
        return result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::AnimationStringRecordState record{};
    record.name.header = 99u;
    record.value.header = 199u;

    runtime.state = {0, 0, 0, 0, 0, 0x1000u, 0x1080u, 0x10c0u};
    runtime.Fill(0x1000u, 4u);
    pal4::AnimationStringTable::InsertRecords(
        0x5000u, 0x1020u, 1u, record, runtime);
    Check(runtime.Ids() == std::vector<std::uint32_t>{1, 99, 2, 3, 4} &&
              runtime.state.end == 0x10a0u &&
              runtime.calls == std::vector<Call>{
                  {'C', 0x1080u, 0x1060u},
                  {'S', 0x1060u, 0x1040u},
                  {'S', 0x1070u, 0x1050u},
                  {'S', 0x1040u, 0x1020u},
                  {'S', 0x1050u, 0x1030u},
                  {'s', 0x1020u, 99u},
                  {'s', 0x1030u, 199u}},
          "long tail moves backward before assigning insertion slot");

    runtime.calls.clear();
    runtime.state = {0, 0, 0, 0, 0, 0x1000u, 0x1060u, 0x10e0u};
    runtime.Fill(0x1000u, 3u);
    pal4::AnimationStringTable::InsertRecords(
        0x5000u, 0x1040u, 2u, record, runtime);
    Check(runtime.Ids() == std::vector<std::uint32_t>{1, 2, 99, 99, 3} &&
              runtime.state.end == 0x10a0u &&
              runtime.calls == std::vector<Call>{
                  {'C', 0x1080u, 0x1040u},
                  {'T', 0x1060u, 0},
                  {'s', 0x1040u, 99u}, {'s', 0x1050u, 199u}},
          "short tail copies old tail and constructs extra new records");

    runtime.calls.clear();
    runtime.state = {0, 0, 0, 0, 0, 0x1000u, 0x1060u, 0x1060u};
    runtime.Fill(0x1000u, 3u);
    pal4::AnimationStringTable::InsertRecords(
        0x5000u, 0x1020u, 2u, record, runtime);
    Check(runtime.Ids() == std::vector<std::uint32_t>{1, 99, 99, 2, 3} &&
              runtime.state.begin == 0x2000u &&
              runtime.state.end == 0x20a0u &&
              runtime.state.limit == 0x20c0u &&
              runtime.calls == std::vector<Call>{
                  {'A', 192u, 0},
                  {'C', 0x2000u, 0x1000u},
                  {'T', 0x2020u, 0}, {'T', 0x2040u, 0},
                  {'C', 0x2060u, 0x1020u},
                  {'C', 0x2080u, 0x1040u},
                  {'F', 0x1000u, 0}},
          "growth copies prefix, inserted records and suffix in order");

    runtime.calls.clear();
    runtime.state = {};
    runtime.Fill(0, 0);
    pal4::AnimationStringTable::InsertRecords(
        0x5000u, 0u, 1u, record, runtime);
    Check(runtime.state.begin == 0x2000u &&
              runtime.state.end == 0x2020u &&
              runtime.state.limit == 0x2020u &&
              runtime.Ids() == std::vector<std::uint32_t>{99u} &&
              runtime.calls == std::vector<Call>{
                  {'A', 32u, 0}, {'T', 0x2000u, 0}, {'F', 0u, 0}},
          "empty array allocates one slot and frees null old begin");

    runtime.calls.clear();
    runtime.allocation = 0u;
    runtime.state = {};
    pal4::AnimationStringTable::InsertRecords(
        0x5000u, 0u, 1u, record, runtime);
    Check(runtime.state.begin == 0u && runtime.state.end == 0u &&
              runtime.calls == std::vector<Call>{{'A', 32u, 0}},
          "allocation failure leaves array unchanged");

    runtime.calls.clear();
    runtime.state = {0, 0, 0, 0, 0, 0x1000u, 0x1021u, 0x1040u};
    pal4::AnimationStringTable::InsertRecords(
        0x5000u, 0x1000u, 1u, record, runtime);
    Check(runtime.state.end == 0x1021u && runtime.calls.empty(),
          "malformed range is rejected");
    runtime.state.end = 0x1020u;
    pal4::AnimationStringTable::InsertRecords(
        0x5000u, 0x1000u, 0u, record, runtime);
    Check(runtime.state.end == 0x1020u && runtime.calls.empty(),
          "zero insertion count is a no-op");

    std::cout << "animation_string_table_insert_tests passed\n";
}
