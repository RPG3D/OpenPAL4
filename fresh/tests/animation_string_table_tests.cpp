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

class Runtime final : public pal4::IAnimationStringTableRuntime {
public:
    std::map<std::uint32_t, pal4::AnimationStringTableState> tables;
    std::map<std::uint32_t, pal4::AnimationStringFieldState> fields;
    std::map<std::uint32_t, std::uint8_t> references;
    std::vector<Call> calls;
    bool allocation_success = true;

    pal4::AnimationStringTableState& TableAt(
        std::uint32_t address) override {
        return tables[address];
    }
    pal4::AnimationStringFieldState& FieldAt(
        std::uint32_t address) override {
        calls.push_back({'s', address, 0, 0});
        return fields[address];
    }
    std::uint32_t EmptyText() override {
        calls.push_back({'e', 0, 0, 0});
        return 0x7000u;
    }
    std::int32_t CompareText(std::uint32_t left,
                             std::uint32_t right) override {
        calls.push_back({'=', left, right, 0});
        return left == right ? 0 : 1;
    }
    std::uint32_t CStringLength(std::uint32_t address) override {
        calls.push_back({'l', address, 0, 0});
        return 6u;
    }
    std::int32_t ParseInteger(std::uint32_t text) override {
        calls.push_back({'i', text, 0, 0});
        return -42;
    }
    float ParseFloat(std::uint32_t text) override {
        calls.push_back({'d', text, 0, 0});
        return 1.25f;
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
    void SetErrorFlag(std::uint32_t table,
                      std::uint8_t flag) override {
        calls.push_back({'f', table, flag, 0});
    }
    bool AllocateText(std::uint32_t table, std::uint32_t length,
                      bool exact) override {
        calls.push_back({'a', table, length, exact ? 1u : 0u});
        if (allocation_success) {
            tables[table].text = 0x8000u;
            tables[table].capacity = length;
            fields[table].text = 0x8000u;
            fields[table].capacity = length;
        }
        return allocation_success;
    }
    void CopyText(std::uint32_t destination, std::uint32_t source,
                  std::uint32_t length) override {
        calls.push_back({'c', destination, source, length});
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back({'b', address, value, 0});
    }
    void SetTextSize(std::uint32_t table, std::uint32_t length) override {
        calls.push_back({'z', table, length, 0});
        tables[table].length = length;
        fields[table].length = length;
    }
    void EraseText(std::uint32_t table, std::uint32_t position,
                   std::uint32_t count) override {
        calls.push_back({'x', table, position, count});
    }
    void DetachText(std::uint32_t) override {}
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back({'F', address, 0, 0});
        return address + 0x10u;
    }
    void LoadTableText(std::uint32_t, std::uint32_t) override {}
    void ReadRecordCount(std::uint32_t, std::uint32_t&) override {}
    void ResetTemporaryText(pal4::AnimationStringFieldState&,
                            std::uint8_t) override {}
    void LoadTemporaryText(pal4::AnimationStringFieldState&,
                           std::uint32_t) override {}
    std::uint32_t AllocateRecordArray(std::uint32_t) override {
        return 0;
    }
    void CopyRecord(std::uint32_t, std::uint32_t) override {}
    void CopyTemplateRecord(
        std::uint32_t,
        const pal4::AnimationStringRecordState&) override {}
    void AssignRecordField(std::uint32_t, std::uint32_t) override {}
    void AssignTemplateField(
        std::uint32_t,
        const pal4::AnimationStringFieldState&) override {}
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::AnimationStringTable::CopyConstruct(0, 0x2055u,
                                                     runtime) == 0 &&
              runtime.tables.empty(),
          "null destination leaves runtime untouched");

    runtime.tables[0x1055u] = {0xaabbcc00u, 0, 0, 0,
                               0x12345600u, 0x9000u, 0x9020u, 0x9040u};
    runtime.tables[0x2066u] = {0x0000005au, 0x4000u, 4u, 7u,
                               0u, 0u, 0u, 0u};
    runtime.references[0x3fffu] = 0xfdu;
    Check(pal4::AnimationStringTable::CopyConstruct(0x1055u, 0x2066u,
                                                     runtime) == 0x1055u,
          "constructor returns destination");
    const auto& shared = runtime.tables[0x1055u];
    Check(shared.header == 0xaabbcc5au && shared.text == 0x4000u &&
              shared.length == 4u && shared.capacity == 7u &&
              shared.tag == 0x12345666u && shared.begin == 0 &&
              shared.end == 0 && shared.limit == 0 &&
              runtime.references[0x3fffu] == 0xfeu &&
              runtime.calls == std::vector<Call>{
                  {'r', 0x3fffu, 0, 0}, {'f', 0x1055u, 1, 0},
                  {'r', 0x3fffu, 0, 0}, {'w', 0x3fffu, 0xfeu, 0}},
          "non-saturated text shares reference and resets array");

    runtime.calls.clear();
    runtime.tables[0x1055u] = {0, 0, 0, 0, 0, 1, 2, 3};
    runtime.references[0x3fffu] = 0xfeu;
    Check(pal4::AnimationStringTable::CopyConstruct(0x1055u, 0x2066u,
                                                     runtime) == 0x1055u,
          "deep copy returns destination");
    Check(runtime.tables[0x1055u].text == 0x8000u &&
              runtime.tables[0x1055u].length == 4u &&
              runtime.calls == std::vector<Call>{
                  {'r', 0x3fffu, 0, 0}, {'a', 0x1055u, 4u, 1u},
                  {'c', 0x8000u, 0x4000u, 4u}, {'z', 0x1055u, 4u, 0}},
          "saturated reference gets owned copy");

    runtime.calls.clear();
    runtime.tables[0x2066u].length = 0;
    Check(pal4::AnimationStringTable::CopyConstruct(0x1055u, 0x2066u,
                                                     runtime) == 0x1055u,
          "empty copy returns destination");
    Check(runtime.calls == std::vector<Call>{
              {'a', 0x1055u, 0u, 1u}, {'c', 0x8000u, 0x4000u, 0u},
              {'z', 0x1055u, 0u, 0u}},
          "empty string still takes allocation path");

    runtime.calls.clear();
    runtime.allocation_success = false;
    runtime.tables[0x2066u].length = 4;
    Check(pal4::AnimationStringTable::CopyConstruct(0x1055u, 0x2066u,
                                                     runtime) == 0x1055u,
          "failed allocation returns destination");
    Check(runtime.tables[0x1055u].text == 0u &&
              runtime.tables[0x1055u].begin == 0u &&
              runtime.calls == std::vector<Call>{
                  {'r', 0x3fffu, 0, 0}, {'a', 0x1055u, 4u, 1u}},
          "allocation failure still initializes array");

    runtime.calls.clear();
    runtime.allocation_success = true;
    runtime.tables[0x2066u].text = 0;
    runtime.references[0x6fffu] = 0;
    Check(pal4::AnimationStringTable::CopyConstruct(0x1055u, 0x2066u,
                                                     runtime) == 0x1055u,
          "empty-text fallback returns destination");
    Check(runtime.tables[0x1055u].text == 0x7000u &&
              runtime.references[0x6fffu] == 1u &&
              runtime.calls.front() == Call{'e', 0, 0, 0},
          "null source text uses native empty-text fallback");

    runtime.calls.clear();
    runtime.tables[0x1055u] = {0x12345678u, 0x4000u, 7u, 8u,
                               0xaabbcc00u, 1u, 2u, 3u};
    Check(pal4::AnimationStringTable::CopyConstruct(0x1055u, 0x1055u,
                                                     runtime) == 0x1055u,
          "self-copy returns destination");
    Check(runtime.tables[0x1055u].text == 0u &&
              runtime.tables[0x1055u].tag == 0xaabbcc55u &&
              runtime.calls == std::vector<Call>{
                  {'x', 0x1055u, 0u, 0xffffffffu},
                  {'x', 0x1055u, 0u, 0u}},
          "self-copy erases twice after its string triple is cleared");

    runtime.calls.clear();
    runtime.tables[0x1055u] = {0x12345678u, 0x4000u, 7u, 8u,
                               0xaabbcc55u, 0x9000u, 0x9040u, 0x9060u};
    runtime.references[0x3fffu] = 2u;
    Check(pal4::AnimationStringTable::Destroy(0x1055u, runtime) == 0x3fffu &&
              runtime.calls == std::vector<Call>{
                  {'f', 0x9010u, 1u, 0}, {'f', 0x9000u, 1u, 0},
                  {'f', 0x9030u, 1u, 0}, {'f', 0x9020u, 1u, 0},
                  {'F', 0x9000u, 0, 0}, {'r', 0x3fffu, 0, 0},
                  {'w', 0x3fffu, 1u, 0}} &&
              runtime.tables[0x1055u].begin == 0u &&
              runtime.tables[0x1055u].end == 0u &&
              runtime.tables[0x1055u].limit == 0u &&
              runtime.tables[0x1055u].text == 0u &&
              runtime.tables[0x1055u].length == 0u &&
              runtime.tables[0x1055u].capacity == 0u &&
              runtime.tables[0x1055u].tag == 0xaabbcc55u,
          "destructor cleans elements and decrements shared first text");

    for (const auto reference : {0u, 0xffu}) {
        runtime.calls.clear();
        runtime.tables[0x1055u].text = 0x5000u;
        runtime.tables[0x1055u].length = 3u;
        runtime.references[0x4fffu] = static_cast<std::uint8_t>(reference);
        Check(pal4::AnimationStringTable::Destroy(0x1055u, runtime) ==
                  0x500fu &&
                  runtime.calls == std::vector<Call>{
                      {'F', 0u, 0, 0}, {'r', 0x4fffu, 0, 0},
                      {'F', 0x4fffu, 0, 0}},
              "unshared first text frees its header after empty array");
    }
    runtime.calls.clear();
    Check(pal4::AnimationStringTable::Destroy(0x1055u, runtime) == 0u &&
              runtime.calls == std::vector<Call>{{'F', 0u, 0, 0}},
          "repeated destructor frees null array but no text");

    runtime.calls.clear();
    runtime.tables[0x1055u].begin = 0x9000u;
    runtime.tables[0x1055u].end = 0x9040u;
    runtime.fields[0x9000u] = {0, 0x5000u, 4u, 5u};
    runtime.fields[0x9020u] = {0, 0u, 0u, 0u};
    Check(pal4::AnimationStringTable::FindString(
              0x1055u, 0x5000u, runtime) == 0x9000u &&
              runtime.calls == std::vector<Call>{
                  {'s', 0x9000u, 0, 0}, {'=', 0x5000u, 0x5000u, 0}},
          "find returns the first matching 32-byte element");
    runtime.calls.clear();
    Check(pal4::AnimationStringTable::FindString(
              0x1055u, 0x7000u, runtime) == 0x9020u &&
              runtime.calls == std::vector<Call>{
                  {'s', 0x9000u, 0, 0}, {'=', 0x5000u, 0x7000u, 0},
                  {'s', 0x9020u, 0, 0}, {'e', 0, 0, 0},
                  {'=', 0x7000u, 0x7000u, 0}},
          "find uses the empty-text fallback for a null name");
    runtime.calls.clear();
    Check(pal4::AnimationStringTable::FindString(
              0x1055u, 0x6000u, runtime) == 0u &&
              runtime.calls.size() == 5u,
          "find returns null after all names differ");
    runtime.calls.clear();
    runtime.tables[0x1055u].end = 0x9000u;
    Check(pal4::AnimationStringTable::FindString(
              0x1055u, 0x5000u, runtime) == 0u &&
              runtime.calls.empty(),
          "empty table avoids comparison callback");

    runtime.tables[0x1055u].end = 0x9020u;
    runtime.fields[0x9010u] = {0x0000005au, 0x6000u, 3u, 8u};
    runtime.fields[0xa055u] = {0xaabbcc00u, 0u, 0u, 0u};
    runtime.references[0x5fffu] = 1u;
    runtime.calls.clear();
    Check(pal4::AnimationStringTable::CopyString(
              0x1055u, 0xa055u, 0x5000u, 0xb000u,
              runtime) == 0xa055u &&
              runtime.fields[0xa055u].header == 0xaabbcc5au &&
              runtime.fields[0xa055u].text == 0x6000u &&
              runtime.fields[0xa055u].length == 3u &&
              runtime.fields[0xa055u].capacity == 8u &&
              runtime.references[0x5fffu] == 2u,
          "copy of found value shares an unsaturated text reference");

    runtime.calls.clear();
    runtime.fields[0xa055u] = {};
    runtime.references[0x5fffu] = 0xfeu;
    Check(pal4::AnimationStringTable::CopyString(
              0x1055u, 0xa055u, 0x5000u, 0xb000u,
              runtime) == 0xa055u &&
              runtime.fields[0xa055u].text == 0x8000u &&
              runtime.fields[0xa055u].length == 3u &&
              runtime.calls.end()[-2] ==
                  Call{'c', 0x8000u, 0x6000u, 3u} &&
              runtime.calls.back() == Call{'z', 0xa055u, 3u, 0},
          "copy of saturated value allocates and sets size");

    runtime.calls.clear();
    runtime.fields[0xa055u] = {};
    Check(pal4::AnimationStringTable::CopyString(
              0x1055u, 0xa055u, 0x7777u, 0xb000u,
              runtime) == 0xa055u &&
              runtime.fields[0xa055u].header == 0x55u &&
              runtime.fields[0xa055u].length == 6u &&
              runtime.calls.end()[-3] ==
                  Call{'a', 0xa055u, 6u, 1u} &&
              runtime.calls.end()[-2] ==
                  Call{'c', 0x8000u, 0xb000u, 6u} &&
              runtime.calls.back() == Call{'b', 0x8006u, 0u, 0},
          "missing name copies fallback C text and writes terminator");

    runtime.calls.clear();
    runtime.fields[0x9010u] = {0x12345678u, 0x6000u, 3u, 8u};
    Check(pal4::AnimationStringTable::CopyString(
              0x1055u, 0x9010u, 0x5000u, 0xb000u,
              runtime) == 0x9010u &&
              runtime.fields[0x9010u].text == 0u &&
              runtime.calls.end()[-2] ==
                  Call{'x', 0x9010u, 0u, 0xffffffffu} &&
              runtime.calls.back() == Call{'x', 0x9010u, 0u, 0u},
          "self-copy erases after resetting source value field");

    runtime.calls.clear();
    runtime.fields[0x9010u] = {0x5au, 0x6000u, 0u, 8u};
    runtime.fields[0xa055u] = {};
    Check(pal4::AnimationStringTable::CopyString(
              0x1055u, 0xa055u, 0x5000u, 0xb000u,
              runtime) == 0xa055u &&
              runtime.calls.end()[-3] ==
                  Call{'a', 0xa055u, 0u, 1u} &&
              runtime.calls.end()[-2] ==
                  Call{'c', 0x8000u, 0x6000u, 0u} &&
              runtime.calls.back() == Call{'z', 0xa055u, 0u, 0},
          "zero-length found value still allocates and sets size");

    runtime.calls.clear();
    runtime.allocation_success = false;
    runtime.fields[0x9010u].length = 3u;
    runtime.fields[0xa055u] = {};
    Check(pal4::AnimationStringTable::CopyString(
              0x1055u, 0xa055u, 0x5000u, 0xb000u,
              runtime) == 0xa055u &&
              runtime.fields[0xa055u].text == 0u &&
              runtime.calls.back() == Call{'a', 0xa055u, 3u, 1u},
          "found value allocation failure leaves destination empty");
    runtime.calls.clear();
    Check(pal4::AnimationStringTable::CopyString(
              0x1055u, 0xa055u, 0x7777u, 0xb000u,
              runtime) == 0xa055u &&
              runtime.fields[0xa055u].text == 0u &&
              runtime.calls.back() == Call{'a', 0xa055u, 6u, 1u},
          "fallback allocation failure skips copy and terminator");

    runtime.calls.clear();
    Check(pal4::AnimationStringTable::ConvertStringToInt(
              0x1055u, 0x7777u, 17, runtime) == 17 &&
              runtime.calls == std::vector<Call>{
                  {'s', 0x9000u, 0, 0}, {'=', 0x5000u, 0x7777u, 0}},
          "integer conversion returns default without parsing on miss");
    runtime.calls.clear();
    runtime.fields[0x9010u].text = 0x6000u;
    Check(pal4::AnimationStringTable::ConvertStringToInt(
              0x1055u, 0x5000u, 17, runtime) == -42 &&
              runtime.calls.back() == Call{'i', 0x6000u, 0, 0},
          "integer conversion parses matched value");
    runtime.calls.clear();
    runtime.fields[0x9010u].text = 0;
    Check(pal4::AnimationStringTable::ConvertStringToInt(
              0x1055u, 0x5000u, 17, runtime) == -42 &&
              runtime.calls.end()[-2] == Call{'e', 0, 0, 0} &&
              runtime.calls.back() == Call{'i', 0x7000u, 0, 0},
          "integer conversion parses empty-text fallback");
    runtime.calls.clear();
    Check(pal4::AnimationStringTable::ConvertStringToFloat(
              0x1055u, 0x7777u, -3.5f, runtime) == -3.5f &&
              runtime.calls.size() == 2u,
          "float conversion returns default on miss");
    runtime.calls.clear();
    runtime.fields[0x9010u].text = 0x6000u;
    Check(pal4::AnimationStringTable::ConvertStringToFloat(
              0x1055u, 0x5000u, -3.5f, runtime) == 1.25f &&
              runtime.calls.back() == Call{'d', 0x6000u, 0, 0},
          "float conversion uses parsed result");

    std::cout << "animation_string_table_tests passed\n";
}
