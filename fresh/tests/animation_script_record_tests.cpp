#include "pal4/animation_script_record_array.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IAnimationScriptRecordRuntime {
public:
    std::map<std::uint32_t, pal4::AnimationScriptRecordState> records;
    std::map<std::uint32_t, std::uint8_t> references;
    std::vector<std::string> calls;
    std::uint32_t free_result = 0x12345678u;

    pal4::AnimationScriptRecordState& RecordAt(
        std::uint32_t address) override {
        calls.push_back("record:" + std::to_string(address));
        return records.at(address);
    }
    std::uint8_t ReadReference(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return references.at(address);
    }
    void WriteReference(std::uint32_t address,
                        std::uint8_t value) override {
        calls.push_back("write:" + std::to_string(address));
        references[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return free_result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::AnimationScriptRecordState record{
        11u, 0x1010u, 4u, 8u, 22u, 0x2010u, 5u, 9u};
    runtime.references[0x100fu] = 2u;
    runtime.references[0x200fu] = 0u;
    Check(pal4::AnimationScriptRecord::Cleanup(record, runtime) == 0x100fu &&
              runtime.calls == std::vector<std::string>{
                  "read:8207", "free:8207", "read:4111", "write:4111"} &&
              runtime.references[0x100fu] == 1u,
          "cleanup releases second text then decrements first reference");
    Check(record.first_header == 11u && record.second_header == 22u &&
              record.first_text == 0u && record.first_length == 0u &&
              record.first_capacity == 0u && record.second_text == 0u &&
              record.second_length == 0u && record.second_capacity == 0u,
          "cleanup preserves headers and clears both string triples");

    runtime.calls.clear();
    record.first_text = 0x3010u;
    record.second_text = 0x4010u;
    runtime.references[0x300fu] = 0xffu;
    runtime.references[0x400fu] = 1u;
    Check(pal4::AnimationScriptRecord::Cleanup(record, runtime) ==
              runtime.free_result &&
              runtime.calls == std::vector<std::string>{
                  "read:16399", "write:16399", "read:12303", "free:12303"} &&
              runtime.references[0x400fu] == 0u,
          "cleanup handles saturated free and nonzero decrement");
    runtime.calls.clear();
    Check(pal4::AnimationScriptRecord::Cleanup(record, runtime) == 0u &&
              runtime.calls.empty(),
          "empty record returns zero without native calls");

    pal4::AnimationScriptRecordArrayState array{9u, 0x5000u,
                                                   0x5040u, 0x5080u};
    runtime.records[0x5000u] = {1u, 0x6010u, 1u, 2u,
                                 2u, 0x7010u, 1u, 2u};
    runtime.records[0x5020u] = {3u, 0x8010u, 1u, 2u,
                                 4u, 0x9010u, 1u, 2u};
    for (auto address : {0x600fu, 0x700fu, 0x800fu, 0x900fu})
        runtime.references[address] = 0u;
    runtime.calls.clear();
    Check(pal4::AnimationScriptRecordArray::ClearRecords(array, runtime) ==
              runtime.free_result &&
              runtime.calls.front() == "record:20480" &&
              runtime.calls[3] == "read:24591" &&
              runtime.calls[5] == "record:20512" &&
              runtime.calls.back() == "free:32783" &&
              array.begin == 0x5000u && array.end == 0x5000u &&
              array.capacity == 0x5080u && array.reserved == 9u,
          "range cleanup visits both records and retains allocation");

    runtime.calls.clear();
    Check(pal4::AnimationScriptRecordArray::ReleaseData(array, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"free:20480"} &&
              array.reserved == 9u && array.begin == 0u &&
              array.end == 0u && array.capacity == 0u,
          "release after clear frees allocation and zeros vector pointers");

    array = {12u, 0xa000u, 0xa020u, 0xa040u};
    runtime.records[0xa000u] = {1u, 0xb010u, 1u, 2u,
                                 2u, 0xc010u, 1u, 2u};
    runtime.references[0xb00fu] = 0u;
    runtime.references[0xc00fu] = 0u;
    runtime.calls.clear();
    Check(pal4::AnimationScriptRecordArray::ReleaseData(array, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "record:40960", "read:49167", "free:49167",
                  "read:45071", "free:45071", "free:40960"} &&
              array.reserved == 12u && array.begin == 0u &&
              array.end == 0u && array.capacity == 0u,
          "direct release cleans both fields before freeing record array");

    array = {5u, 0u, 0u, 0u};
    runtime.calls.clear();
    Check(pal4::AnimationScriptRecordArray::ReleaseData(array, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"free:0"},
          "empty release forwards null free");

    pal4::AnimationScriptRecordRuntimeStub stub;
    Check(pal4::AnimationScriptRecord::Cleanup(stub.RecordAt(0u), stub) == 0u,
          "native runtime stub is linkable");
    std::cout << "animation_script_record_tests passed\n";
}
