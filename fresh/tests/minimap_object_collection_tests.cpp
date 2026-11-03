#include "pal4/minimap_object_collection.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapObjectCollectionRuntime {
public:
    pal4::MinimapObjectCollectionState state;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;

    pal4::MinimapObjectCollectionState& Access(
        std::uint32_t address) override {
        Check(address == 0x5000u, "collection address");
        return state;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void FreeRecordBuffer(std::uint32_t address) override {
        events.emplace_back("buffer:" + std::to_string(address));
    }
    void FreeRecordArray(std::uint32_t address) override {
        events.emplace_back("array:" + std::to_string(address));
    }
};

void TestConstruct() {
    Runtime runtime;
    runtime.state.Bytes().fill(0xa5u);
    Check(pal4::MinimapObjectCollection::Construct(
              0x5000u, 0x42u, runtime) == 0x5000u &&
              runtime.state.Bytes()[0] == 0x42u &&
              runtime.state.Bytes()[1] == 0xa5u &&
              runtime.state.Bytes()[2] == 0xa5u &&
              runtime.state.Bytes()[3] == 0xa5u &&
              runtime.state.ReadDword(4u) == 0u &&
              runtime.state.ReadDword(8u) == 0u &&
              runtime.state.ReadDword(12u) == 0u &&
              runtime.state.ReadDword(16u) == 0u,
          "constructor writes one incoming byte and four zero DWORDs");
}

void TestCleanupRecords() {
    Runtime runtime;
    runtime.state.WriteDword(0u, 0x12345678u);
    runtime.state.WriteDword(4u, 0x6000u);
    runtime.state.WriteDword(8u, 0x6028u);
    runtime.state.WriteDword(12u, 0x6040u);
    runtime.state.WriteDword(16u, 0xabcdef01u);
    runtime.words[0x6008u] = 0x7000u;
    runtime.words[0x600cu] = 0x7008u;
    runtime.words[0x6010u] = 0x7010u;
    runtime.words[0x601cu] = 0u;
    runtime.words[0x6020u] = 0u;
    runtime.words[0x6024u] = 0u;
    Check(pal4::MinimapObjectCollection::Cleanup(
              0x5000u, runtime) == 0 &&
              runtime.words[0x6008u] == 0u &&
              runtime.words[0x600cu] == 0u &&
              runtime.words[0x6010u] == 0u &&
              runtime.words[0x601cu] == 0u &&
              runtime.state.ReadDword(0u) == 0x12345678u &&
              runtime.state.ReadDword(4u) == 0u &&
              runtime.state.ReadDword(8u) == 0u &&
              runtime.state.ReadDword(12u) == 0u &&
              runtime.state.ReadDword(16u) == 0xabcdef01u,
          "cleanup clears each nested buffer and outer collection pointers");
    Check(runtime.events == std::vector<std::string>{
              "read:24584", "buffer:28672", "write:24584",
              "write:24588", "write:24592", "read:24604",
              "buffer:0", "write:24604", "write:24608",
              "write:24612", "array:24576"},
          "nested buffers are released in record order before array");
}

void TestEmptyCleanup() {
    Runtime runtime;
    runtime.state.WriteDword(16u, 0xfeedu);
    Check(pal4::MinimapObjectCollection::Cleanup(
              0x5000u, runtime) == 0 &&
              runtime.events == std::vector<std::string>{"array:0"} &&
              runtime.state.ReadDword(16u) == 0xfeedu,
          "empty collection still passes null to array free");
}

}  // namespace

int main() {
    TestConstruct();
    TestCleanupRecords();
    TestEmptyCleanup();
    return EXIT_SUCCESS;
}
