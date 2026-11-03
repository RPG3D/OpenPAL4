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
    void AssignRecordField(std::uint32_t destination,
                           std::uint32_t source) override {
        calls.push_back({'A', destination, source, 0});
        const auto header = fields[destination].header;
        fields[destination] = fields[source];
        fields[destination].header = header;
    }
    void AssignTemplateField(
        std::uint32_t destination,
        const pal4::AnimationStringFieldState& source) override {
        calls.push_back({'T', destination, source.header, 0});
        const auto header = fields[destination].header;
        fields[destination] = source;
        fields[destination].header = header;
    }
    bool AllocateText(std::uint32_t field,
                      std::uint32_t length, bool exact) override {
        calls.push_back({'a', field, length, exact ? 1u : 0u});
        if (allocation_success) {
            fields[field].text = 0x8000u;
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
    void EraseText(std::uint32_t field,
                   std::uint32_t position,
                   std::uint32_t count) override {
        calls.push_back({'x', field, position, count});
    }
};

}  // namespace

int main() {
    Runtime runtime;
    runtime.CopyRecord(0, 0x2000u);
    Check(runtime.calls.empty() && runtime.fields.empty(),
          "null destination is a no-op");

    runtime.fields[0x1000u] = {0xaabbcc00u, 0, 0, 0};
    runtime.fields[0x1010u] = {0x11223300u, 0, 0, 0};
    runtime.fields[0x2000u] = {0x55u, 0x3000u, 3u, 5u};
    runtime.fields[0x2010u] = {0x66u, 0x4000u, 4u, 6u};
    runtime.references[0x3fffu] = 0xfdu;
    runtime.CopyRecord(0x1000u, 0x2000u);
    Check(runtime.fields[0x1000u].header == 0xaabbcc55u &&
              runtime.fields[0x1000u].text == 0x3000u &&
              runtime.fields[0x1010u].header == 0x11223366u &&
              runtime.fields[0x1010u].text == 0x4000u &&
              runtime.fields[0x1010u].length == 4u &&
              runtime.fields[0x1010u].capacity == 6u &&
              runtime.references[0x3fffu] == 0xfeu &&
              runtime.calls == std::vector<Call>{
                  {'A', 0x1000u, 0x2000u, 0},
                  {'r', 0x3fffu, 0, 0}, {'r', 0x3fffu, 0, 0},
                  {'w', 0x3fffu, 0xfeu, 0}},
          "default record copy assigns first string and shares second");

    runtime.calls.clear();
    runtime.fields[0x1000u] = {0xaabbcc00u, 0, 0, 0};
    runtime.fields[0x1010u] = {0x11223300u, 0, 0, 0};
    runtime.references[0x3fffu] = 0xfeu;
    runtime.CopyRecord(0x1000u, 0x2000u);
    Check(runtime.fields[0x1010u].text == 0x8000u &&
              runtime.fields[0x1010u].length == 4u &&
              runtime.calls.end()[-3] ==
                  Call{'a', 0x1010u, 4u, 1u} &&
              runtime.calls.end()[-2] ==
                  Call{'c', 0x8000u, 0x4000u, 4u} &&
              runtime.calls.back() == Call{'z', 0x1010u, 4u, 0},
          "saturated second string is deep-copied");

    runtime.calls.clear();
    runtime.fields[0x2010u].length = 0u;
    runtime.CopyRecord(0x1000u, 0x2000u);
    Check(runtime.calls.end()[-3] ==
              Call{'a', 0x1010u, 0u, 1u} &&
              runtime.calls.end()[-2] ==
              Call{'c', 0x8000u, 0x4000u, 0u} &&
              runtime.calls.back() == Call{'z', 0x1010u, 0u, 0},
          "zero-length second string still allocates");

    runtime.calls.clear();
    runtime.allocation_success = false;
    runtime.fields[0x2010u].length = 4u;
    runtime.CopyRecord(0x1000u, 0x2000u);
    Check(runtime.fields[0x1010u].text == 0u &&
              runtime.calls.back() == Call{'a', 0x1010u, 4u, 1u},
          "failed deep-copy allocation leaves second string empty");

    runtime.calls.clear();
    runtime.allocation_success = true;
    runtime.fields[0x2010u].text = 0u;
    runtime.references[0x6fffu] = 0u;
    runtime.CopyRecord(0x1000u, 0x2000u);
    Check(runtime.fields[0x1010u].text == 0x7000u &&
              runtime.references[0x6fffu] == 1u &&
              runtime.calls.end()[-1] ==
                  Call{'w', 0x6fffu, 1u, 0},
          "null second-string data shares native empty-text fallback");

    runtime.calls.clear();
    runtime.fields[0x1010u] = {0x11223366u, 0x4000u, 4u, 6u};
    runtime.CopyRecord(0x1000u, 0x1000u);
    Check(runtime.fields[0x1010u].text == 0u &&
              runtime.calls.end()[-2] ==
                  Call{'x', 0x1010u, 0u, 0xffffffffu} &&
              runtime.calls.back() == Call{'x', 0x1010u, 0u, 0u},
          "self-copy erases second field after resetting it");

    runtime.calls.clear();
    pal4::AnimationStringRecordState temporary{};
    temporary.name = {0x77u, 0x3000u, 3u, 5u};
    temporary.value = {0x88u, 0x4000u, 4u, 6u};
    runtime.references[0x3fffu] = 1u;
    runtime.CopyTemplateRecord(0x1000u, temporary);
    Check(runtime.fields[0x1000u].header == 0xaabbcc77u &&
              runtime.fields[0x1010u].header == 0x11223388u &&
              runtime.fields[0x1010u].text == 0x4000u &&
              runtime.references[0x3fffu] == 2u &&
              runtime.calls.front() == Call{'T', 0x1000u, 0x77u, 0},
          "template record uses rebuilt copy and field assignment");

    std::cout << "animation_string_table_copy_tests passed\n";
}
