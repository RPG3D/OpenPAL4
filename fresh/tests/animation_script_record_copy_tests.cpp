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
    std::uint32_t third;
    bool operator==(const Call&) const = default;
};

class Runtime final : public pal4::IAnimationScriptRecordInsertRuntime {
public:
    std::map<std::uint32_t, pal4::AnimationScriptRecordState> records;
    std::map<std::uint32_t, std::uint8_t> references;
    std::vector<Call> calls;
    bool allocation_success = true;
    std::uint32_t allocation = 0x8000u;
    std::uint32_t array_allocation = 0xa000u;

    pal4::AnimationScriptRecordState& RecordAt(
        std::uint32_t address) override {
        calls.push_back({'R', address, 0u, 0u});
        return records[address];
    }
    std::uint8_t ReadReference(std::uint32_t address) override {
        calls.push_back({'r', address, 0u, 0u});
        return references[address];
    }
    void WriteReference(std::uint32_t address,
                        std::uint8_t value) override {
        calls.push_back({'w', address, value, 0u});
        references[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back({'F', address, 0u, 0u});
        return 0;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back({'V', bytes, 0u, 0u});
        return array_allocation;
    }
    void AssignString(std::uint32_t destination,
                      std::uint32_t source) override {
        calls.push_back({'S', destination, source, 0u});
        if (destination == source) return;
        auto& target = records[destination];
        const auto& original = records[source];
        target.first_text = original.first_text;
        target.first_length = original.first_length;
        target.first_capacity = original.first_capacity;
    }
    void EraseString(std::uint32_t field,
                     std::uint32_t position,
                     std::uint32_t count) override {
        calls.push_back({'E', field, position, count});
    }
    std::uint32_t EmptyText() override {
        calls.push_back({'e', 0u, 0u, 0u});
        return 0x7000u;
    }
    bool AllocateText(std::uint32_t field, std::uint32_t length,
                      bool exact) override {
        calls.push_back({'A', field, length, exact ? 1u : 0u});
        if (allocation_success) {
            auto& target = records[field - 16u];
            target.second_text = allocation;
            target.second_capacity = length;
        }
        return allocation_success;
    }
    void CopyText(std::uint32_t destination,
                  std::uint32_t source,
                  std::uint32_t length) override {
        calls.push_back({'C', destination, source, length});
    }
    void SetTextSize(std::uint32_t field,
                     std::uint32_t length) override {
        calls.push_back({'Z', field, length, 0u});
        records[field - 16u].second_length = length;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::AnimationScriptRecord::CopyConstruct(0u, 0x2000u, runtime);
    Check(runtime.calls.empty(), "null destination is a no-op");

    runtime.records[0x1000u] = {0xaabbcc00u, 0u, 0u, 0u,
                                0x11223300u, 0u, 0u, 0u};
    runtime.records[0x2000u] = {0x00000055u, 0x3000u, 3u, 4u,
                                0x00000066u, 0x5000u, 5u, 7u};
    runtime.references[0x4fffu] = 1u;
    pal4::AnimationScriptRecord::CopyConstruct(0x1000u, 0x2000u, runtime);
    const auto& shared = runtime.records[0x1000u];
    Check(shared.first_header == 0xaabbcc55u &&
              shared.second_header == 0x11223366u &&
              shared.first_text == 0x3000u && shared.first_length == 3u &&
              shared.second_text == 0x5000u && shared.second_length == 5u &&
              shared.second_capacity == 7u &&
              runtime.references[0x4fffu] == 2u &&
              runtime.calls.back() == Call{'w', 0x4fffu, 2u, 0u},
          "copy preserves upper header bytes and shares reference-counted text");

    runtime.calls.clear();
    runtime.records[0x1000u] = {};
    runtime.records[0x2000u].second_text = 0x6000u;
    runtime.references[0x5fffu] = 0xfeu;
    pal4::AnimationScriptRecord::CopyConstruct(0x1000u, 0x2000u, runtime);
    Check(runtime.records[0x1000u].second_text == 0x8000u &&
              runtime.records[0x1000u].second_length == 5u &&
              runtime.calls.end()[-3] == Call{'A', 0x1010u, 5u, 1u} &&
              runtime.calls.end()[-2] == Call{'C', 0x8000u, 0x6000u, 5u} &&
              runtime.calls.back() == Call{'Z', 0x1010u, 5u, 0u},
          "saturated reference uses allocated byte copy and sets size");

    runtime.calls.clear();
    runtime.records[0x1000u] = {};
    runtime.records[0x2000u].second_text = 0u;
    runtime.records[0x2000u].second_length = 0u;
    pal4::AnimationScriptRecord::CopyConstruct(0x1000u, 0x2000u, runtime);
    Check(runtime.calls.end()[-4] == Call{'A', 0x1010u, 0u, 1u} &&
              runtime.calls.end()[-3] == Call{'e', 0u, 0u, 0u} &&
              runtime.calls.end()[-2] == Call{'C', 0x8000u, 0x7000u, 0u} &&
              runtime.calls.back() == Call{'Z', 0x1010u, 0u, 0u},
          "zero-length second string takes allocation path");

    runtime.calls.clear();
    runtime.allocation_success = false;
    runtime.records[0x1000u] = {};
    runtime.records[0x2000u].second_length = 5u;
    runtime.references[0x6fffu] = 0xfeu;
    pal4::AnimationScriptRecord::CopyConstruct(0x1000u, 0x2000u, runtime);
    Check(runtime.records[0x1000u].second_text == 0u &&
              runtime.calls.back() == Call{'A', 0x1010u, 5u, 1u},
          "failed allocation leaves second string empty");

    runtime.calls.clear();
    runtime.allocation_success = true;
    runtime.records[0x1000u] = {0x12345678u, 0x3000u, 3u, 4u,
                                0x87654321u, 0x5000u, 5u, 7u};
    pal4::AnimationScriptRecord::CopyConstruct(0x1000u, 0x1000u, runtime);
    Check(runtime.calls.end()[-2] ==
              Call{'E', 0x1010u, 0u, 0xffffffffu} &&
              runtime.calls.back() == Call{'E', 0x1010u, 0u, 0u} &&
              runtime.records[0x1000u].second_text == 0u,
          "self-copy uses two native erase calls after resetting destination");

    runtime.calls.clear();
    runtime.records[0x2000u].second_text = 0x6000u;
    runtime.records[0x2000u].second_length = 5u;
    runtime.references[0x5fffu] = 1u;
    pal4::AnimationScriptRecordArrayState array{};
    pal4::AnimationScriptRecordArray::Insert(
        array, 0u, 1u, 0x2000u, runtime);
    Check(array.begin == 0xa000u && array.end == 0xa020u &&
              array.capacity == 0xa020u &&
              runtime.records[0xa000u].first_text == 0x3000u &&
              runtime.records[0xa000u].second_text == 0x6000u &&
              runtime.references[0x5fffu] == 2u &&
              runtime.calls.front() == Call{'V', 32u, 0u, 0u} &&
              runtime.calls.back() == Call{'F', 0u, 0u, 0u},
          "array insertion default copy calls rebuilt record constructor");

    std::cout << "animation_script_record_copy_tests passed\n";
}
