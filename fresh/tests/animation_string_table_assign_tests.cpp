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
    std::uint32_t third;
    bool operator==(const Call&) const = default;
};

class Runtime final : public pal4::AnimationStringTableRuntimeStub {
public:
    std::map<std::uint32_t, pal4::AnimationStringFieldState> fields;
    std::map<std::uint32_t, std::uint8_t> references;
    std::vector<Call> calls;
    bool allocation_success = true;

    pal4::AnimationStringFieldState& FieldAt(
        std::uint32_t address) override {
        return fields[address];
    }
    std::uint32_t EmptyText() override {
        calls.push_back({'e', 0, 0, 0});
        return 0x7000u;
    }
    std::uint8_t ReadReference(std::uint32_t address) override {
        calls.push_back({'r', address, 0, 0});
        return references[address];
    }
    void WriteReference(std::uint32_t address,
                        std::uint8_t value) override {
        calls.push_back({'w', address, value, 0});
        references[address] = value;
    }
    void SetErrorFlag(std::uint32_t field,
                      std::uint8_t flag) override {
        calls.push_back({'f', field, flag, 0});
        fields[field].text = 0;
        fields[field].length = 0;
        fields[field].capacity = 0;
    }
    bool AllocateText(std::uint32_t field,
                      std::uint32_t length, bool exact) override {
        calls.push_back({'a', field, length, exact ? 1u : 0u});
        if (allocation_success) {
            fields[field].text = field == 0x1000u ? 0x8000u : 0x9000u;
            fields[field].capacity = length;
        }
        return allocation_success;
    }
    void CopyText(std::uint32_t destination,
                  std::uint32_t source,
                  std::uint32_t length) override {
        calls.push_back({'c', destination, source, length});
    }
    void SetTextSize(std::uint32_t field,
                     std::uint32_t length) override {
        calls.push_back({'z', field, length, 0});
        fields[field].length = length;
    }
    void DetachText(std::uint32_t field) override {
        calls.push_back({'d', field, 0, 0});
    }
    void EraseText(std::uint32_t field,
                   std::uint32_t position,
                   std::uint32_t count) override {
        calls.push_back({'x', field, position, count});
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::AnimationStringTable::AssignRecord(
              0, 0x2000u, runtime) == 0u && runtime.fields.empty(),
          "null destination is a no-op");

    runtime.fields[0x1000u] = {0xaabbcc11u, 0x8000u, 2u, 3u};
    runtime.fields[0x1010u] = {0x11223322u, 0x9000u, 4u, 5u};
    runtime.fields[0x2000u] = {0x55u, 0x3000u, 3u, 6u};
    runtime.fields[0x2010u] = {0x66u, 0x4000u, 5u, 7u};
    runtime.references[0x2fffu] = 1u;
    runtime.references[0x3fffu] = 0xfdu;
    Check(pal4::AnimationStringTable::AssignRecord(
              0x1000u, 0x2000u, runtime) == 0x1000u &&
              runtime.fields[0x1000u].header == 0xaabbcc11u &&
              runtime.fields[0x1010u].header == 0x11223322u &&
              runtime.fields[0x1000u].text == 0x3000u &&
              runtime.fields[0x1010u].text == 0x4000u &&
              runtime.fields[0x1000u].length == 3u &&
              runtime.fields[0x1010u].length == 5u &&
              runtime.references[0x2fffu] == 2u &&
              runtime.references[0x3fffu] == 0xfeu &&
              runtime.calls == std::vector<Call>{
                  {'r', 0x2fffu, 0, 0}, {'f', 0x1000u, 1u, 0},
                  {'r', 0x2fffu, 0, 0}, {'w', 0x2fffu, 2u, 0},
                  {'r', 0x3fffu, 0, 0}, {'f', 0x1010u, 1u, 0},
                  {'r', 0x3fffu, 0, 0}, {'w', 0x3fffu, 0xfeu, 0}},
          "both fields share references without replacing header words");

    runtime.calls.clear();
    runtime.fields[0x1000u] = {0xaabbcc11u, 0, 0, 0};
    runtime.fields[0x1010u] = {0x11223322u, 0, 0, 0};
    runtime.references[0x2fffu] = 0xfeu;
    runtime.references[0x3fffu] = 0xffu;
    Check(pal4::AnimationStringTable::AssignRecord(
              0x1000u, 0x2000u, runtime) == 0x1000u,
          "deep-copy assignment returns destination");
    Check(runtime.fields[0x1000u].text == 0x8000u &&
              runtime.fields[0x1010u].text == 0x9000u &&
              runtime.fields[0x1000u].length == 3u &&
              runtime.fields[0x1010u].length == 5u &&
              runtime.calls == std::vector<Call>{
                  {'r', 0x2fffu, 0, 0},
                  {'a', 0x1000u, 3u, 1u},
                  {'c', 0x8000u, 0x3000u, 3u},
                  {'z', 0x1000u, 3u, 0},
                  {'r', 0x3fffu, 0, 0},
                  {'a', 0x1010u, 5u, 1u},
                  {'c', 0x9000u, 0x4000u, 5u},
                  {'z', 0x1010u, 5u, 0}},
          "saturated references deep-copy both fields in order");

    runtime.calls.clear();
    runtime.fields[0x2000u].length = 0u;
    runtime.fields[0x2010u].length = 0u;
    Check(pal4::AnimationStringTable::AssignRecord(
              0x1000u, 0x2000u, runtime) == 0x1000u,
          "zero-length assignment returns destination");
    Check(runtime.calls == std::vector<Call>{
              {'a', 0x1000u, 0u, 1u},
              {'c', 0x8000u, 0x3000u, 0u},
              {'z', 0x1000u, 0u, 0},
              {'a', 0x1010u, 0u, 1u},
              {'c', 0x9000u, 0x4000u, 0u},
              {'z', 0x1010u, 0u, 0}},
          "zero-length fields still allocate and set size");

    runtime.calls.clear();
    runtime.allocation_success = false;
    runtime.fields[0x2000u].length = 3u;
    runtime.fields[0x2010u].length = 5u;
    Check(pal4::AnimationStringTable::AssignRecord(
              0x1000u, 0x2000u, runtime) == 0x1000u,
          "failed allocation returns destination");
    Check(runtime.calls == std::vector<Call>{
              {'r', 0x2fffu, 0, 0}, {'a', 0x1000u, 3u, 1u},
              {'r', 0x3fffu, 0, 0}, {'a', 0x1010u, 5u, 1u}},
          "allocation failure skips copy while processing both fields");

    runtime.calls.clear();
    runtime.allocation_success = true;
    runtime.fields[0x2000u].text = 0u;
    runtime.fields[0x2010u].text = 0u;
    runtime.references[0x6fffu] = 0u;
    runtime.fields[0x1000u] = {};
    runtime.fields[0x1010u] = {};
    Check(pal4::AnimationStringTable::AssignRecord(
              0x1000u, 0x2000u, runtime) == 0x1000u,
          "empty-text assignment returns destination");
    Check(runtime.fields[0x1000u].text == 0x7000u &&
              runtime.fields[0x1010u].text == 0x7000u &&
              runtime.references[0x6fffu] == 2u,
          "null source text uses shared native empty-text fallback");

    runtime.calls.clear();
    runtime.fields[0x1000u] = {0x11u, 0x3000u, 3u, 6u};
    runtime.fields[0x1010u] = {0x22u, 0x4000u, 5u, 7u};
    Check(pal4::AnimationStringTable::AssignRecord(
              0x1000u, 0x1000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<Call>{
                  {'d', 0x1000u, 0, 0}, {'x', 0x1000u, 0, 0},
                  {'d', 0x1010u, 0, 0}, {'x', 0x1010u, 0, 0}},
          "self-assignment detaches and erases each field in order");

    std::cout << "animation_string_table_assign_tests passed\n";
}
