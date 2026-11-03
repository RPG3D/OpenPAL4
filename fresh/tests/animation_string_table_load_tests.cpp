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
    std::map<std::uint32_t, std::uint8_t> references;
    std::vector<Call> calls;
    std::vector<pal4::AnimationStringRecordState> inserted;
    std::uint32_t record_count = 0;
    std::uint32_t load_index = 0;

    pal4::AnimationStringTableState& TableAt(std::uint32_t address) override {
        calls.push_back({'T', address, 0});
        return state;
    }
    pal4::AnimationStringFieldState& FieldAt(
        std::uint32_t address) override {
        calls.push_back({'S', address, 0});
        return fields[address];
    }
    void SetErrorFlag(std::uint32_t field,
                      std::uint8_t flag) override {
        calls.push_back({'E', field, flag});
    }
    void LoadTableText(std::uint32_t table,
                       std::uint32_t file) override {
        calls.push_back({'H', table, file});
    }
    void ReadRecordCount(std::uint32_t file,
                         std::uint32_t& count) override {
        calls.push_back({'N', file, 0});
        count = record_count;
    }
    void ResetTemporaryText(pal4::AnimationStringFieldState& field,
                            std::uint8_t flag) override {
        calls.push_back({'R', field.header & 0xffu, flag});
        Check(field.text == 0u && field.length == 0u &&
                  field.capacity == 0u,
              "temporary field starts empty");
    }
    void LoadTemporaryText(pal4::AnimationStringFieldState& field,
                           std::uint32_t file) override {
        calls.push_back({'L', field.header & 0xffu, file});
        field.text = 0x3000u + load_index * 0x100u;
        field.length = load_index + 1u;
        field.capacity = load_index + 2u;
        ++load_index;
    }
    std::uint32_t AllocateRecordArray(std::uint32_t bytes) override {
        calls.push_back({'A', bytes, 0});
        return 0xa000u;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({'C', destination, source});
    }
    void CopyTemplateRecord(
        std::uint32_t destination,
        const pal4::AnimationStringRecordState& record) override {
        calls.push_back({'I', destination, 0});
        inserted.push_back(record);
    }
    void AssignRecordField(std::uint32_t destination,
                           std::uint32_t source) override {
        calls.push_back({'S', destination, source});
    }
    void AssignTemplateField(
        std::uint32_t destination,
        const pal4::AnimationStringFieldState&) override {
        calls.push_back({'s', destination, 0});
    }
    std::uint8_t ReadReference(std::uint32_t address) override {
        calls.push_back({'r', address, 0});
        return references[address];
    }
    void WriteReference(std::uint32_t address,
                        std::uint8_t value) override {
        calls.push_back({'w', address, value});
        references[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back({'F', address, 0});
        return address;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(!pal4::AnimationStringTable::LoadFromFile(0u, 7u, runtime) &&
              runtime.calls.empty(),
          "null table is rejected");

    runtime.state = {0, 0, 0, 0, 0, 0x9000u, 0x9040u, 0x9080u};
    runtime.record_count = 2u;
    runtime.fields[0x9000u] = {0x11u, 0x5000u, 2u, 3u};
    runtime.fields[0x9010u] = {0x22u, 0x6000u, 4u, 5u};
    runtime.fields[0x9020u] = {0x33u, 0u, 0u, 0u};
    runtime.fields[0x9030u] = {0x44u, 0x7000u, 6u, 7u};
    runtime.references[0x4fffu] = 2u;
    runtime.references[0x5fffu] = 0u;
    runtime.references[0x6fffu] = 0xffu;
    runtime.references[0x2fffu] = 0u;
    runtime.references[0x30ffu] = 2u;
    runtime.references[0x31ffu] = 0xffu;
    runtime.references[0x32ffu] = 1u;
    Check(pal4::AnimationStringTable::LoadFromFile(
              0x1000u, 7u, runtime),
          "loader succeeds after two records");
    Check(runtime.state.begin == 0x9000u &&
              runtime.state.end == 0x9040u &&
              runtime.state.limit == 0x9080u &&
              runtime.fields[0x9000u].text == 0u &&
              runtime.fields[0x9010u].text == 0u &&
              runtime.fields[0x9030u].text == 0u &&
              runtime.references[0x4fffu] == 1u &&
              runtime.inserted.size() == 2u &&
              runtime.inserted[0].name.header == 0u &&
              runtime.inserted[0].name.text == 0x3000u &&
              runtime.inserted[0].value.text == 0x3100u &&
              runtime.inserted[1].name.header == 1u &&
              runtime.inserted[1].value.header == 1u &&
              runtime.inserted[1].name.text == 0x3200u &&
              runtime.inserted[1].value.text == 0x3300u,
          "loader preserves array allocation and inserts paired fields");
    Check(runtime.calls == std::vector<Call>{
              {'T', 0x1000u, 0},
              {'S', 0x9000u, 0}, {'S', 0x9010u, 0},
              {'r', 0x5fffu, 0}, {'F', 0x5fffu, 0},
              {'r', 0x4fffu, 0}, {'w', 0x4fffu, 1},
              {'S', 0x9020u, 0}, {'S', 0x9030u, 0},
              {'r', 0x6fffu, 0}, {'F', 0x6fffu, 0},
              {'H', 0x1000u, 7}, {'N', 7, 0},
              {'R', 0, 0}, {'R', 0, 0}, {'L', 0, 7}, {'L', 0, 7},
              {'T', 0x1000u, 0}, {'I', 0x9000u, 0},
              {'r', 0x30ffu, 0}, {'w', 0x30ffu, 1},
              {'r', 0x2fffu, 0}, {'F', 0x2fffu, 0},
              {'R', 1, 0}, {'R', 1, 0}, {'L', 1, 7}, {'L', 1, 7},
              {'T', 0x1000u, 0}, {'I', 0x9020u, 0},
              {'r', 0x32ffu, 0}, {'w', 0x32ffu, 0},
              {'r', 0x31ffu, 0}, {'F', 0x31ffu, 0}},
          "loader follows cleanup, read, insertion and reverse temporary release order");

    runtime.calls.clear();
    runtime.record_count = 0u;
    Check(pal4::AnimationStringTable::LoadFromFile(
              0x1000u, 8u, runtime) &&
              runtime.state.end == runtime.state.begin &&
              runtime.calls == std::vector<Call>{
                  {'T', 0x1000u, 0},
                  {'S', 0x9000u, 0}, {'S', 0x9010u, 0},
                  {'S', 0x9020u, 0}, {'S', 0x9030u, 0},
                  {'H', 0x1000u, 8}, {'N', 8, 0}},
          "zero count clears old elements without reallocating");

    runtime.calls.clear();
    runtime.state.end = 0x9021u;
    Check(!pal4::AnimationStringTable::LoadFromFile(
              0x1000u, 8u, runtime) &&
              runtime.calls == std::vector<Call>{{'T', 0x1000u, 0}} &&
              runtime.state.end == 0x9021u,
          "malformed range is rejected before cleanup");

    std::cout << "animation_string_table_load_tests passed\n";
}
