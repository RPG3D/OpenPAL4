#include "pal4/world_map_frame.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IWorldMapFrameRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    static constexpr std::uint32_t kName = 0x7000u;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::uint32_t> allocated;
    std::string throw_on;
    std::uint32_t reject_address = 0u;

    void Record(const std::string& action) {
        calls.push_back(action);
        if (action == throw_on) throw std::runtime_error(action);
    }
    void LoadLayout(const std::uint32_t frame,
                    const std::uint32_t name) override {
        Check(frame == kFrame && name == kName, "layout arguments");
        Record("base");
    }
    void DestroyBase(const std::uint32_t frame) override {
        Check(frame == kFrame, "destroy base address");
        Record("destroy base");
    }
    void ConstructMouse(const std::uint32_t address) override {
        Check(address == kFrame + 0x28u, "mouse address");
        Record("mouse");
    }
    void DestroyMouse(const std::uint32_t address) override {
        Check(address == kFrame + 0x28u, "destroy mouse address");
        Record("destroy mouse");
    }
    void ConstructNames(const std::uint32_t address,
                        const std::uint32_t count) override {
        Check(address == kFrame + 0x94u && count == 21u,
              "name-array layout");
        Record("names");
    }
    void DestroyNames(const std::uint32_t address,
                      const std::uint32_t count) override {
        Check(address == kFrame + 0x94u && count == 21u,
              "destroy name-array layout");
        Record("destroy names");
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(address == kName, "name's first byte address");
        Record("read byte");
        return 0x57u;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        Record("write byte");
        bytes[address] = value;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Record("write word");
        words[address] = value;
    }
    void ClearNameFlag(const std::uint32_t address) override {
        Check(address == kFrame + 0x258u, "fly-name flag address");
        Record("clear flag");
    }
    void AssignText(const std::uint32_t address,
                    const std::string_view value) override {
        Record(address == kFrame + 0x258u ? "fly text" : "assign text");
        names[address] = value;
    }
    void DestroyText(const std::uint32_t address) override {
        Check(address == kFrame + 0x258u, "destroy fly-name address");
        Record("destroy fly");
    }
    void InitializeEventHandlers(const std::uint32_t frame) override {
        Check(frame == kFrame, "event-handler frame");
        Record("events");
    }
    bool AllocateText(const std::uint32_t address,
                      const std::uint32_t size) override {
        Check(size == 3u, "location text length");
        Record("allocate text");
        allocated.push_back(address);
        return address != reject_address;
    }
    void CopyText(const std::uint32_t address,
                  const std::string_view value) override {
        Record("copy text");
        names[address] = value;
    }
    void SetTextSize(const std::uint32_t address,
                     const std::uint32_t size) override {
        Check(size == 3u && names.at(address).size() == size,
              "set text size after copy");
        Record("set size");
    }
};

struct DestructorRuntime final : pal4::IWorldMapFrameDestructorRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {Runtime::kFrame + 0x25cu, 0x8101u},
        {Runtime::kFrame + 0x30u, 0x9000u}};
    std::uint8_t count = 0u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read word");
        return words.at(address);
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(address == 0x8100u, "reference byte address");
        calls.emplace_back("read byte");
        return count;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write word");
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        Check(address == 0x8100u, "decrement address");
        calls.emplace_back("write byte");
        count = value;
    }
    void Free(const std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    void DestroyNames(const std::uint32_t address,
                      const std::uint32_t name_count) override {
        Check(address == Runtime::kFrame + 0x94u && name_count == 21u,
              "destroy location-name array");
        calls.emplace_back("destroy names");
    }
    std::uint32_t DestroyBase(const std::uint32_t frame) override {
        Check(frame == Runtime::kFrame, "destructor base address");
        calls.emplace_back("destroy base");
        return 0x1234u;
    }
};

void TestConstruction() {
    constexpr std::array<std::string_view, 21> expected{
        "M05", "Q05", "M15", "M03", "Q13", "M17", "M13",
        "Q09", "Q08", "Q01", "Q06", "M14", "Q03", "Q02",
        "M09", "Q07", "M11", "M02", "Q04", "M19", "Q10"};
    Runtime runtime;
    Check(pal4::WorldMapFrame::Construct(Runtime::kFrame, Runtime::kName,
                                         runtime) == Runtime::kFrame,
          "constructor return");
    Check(runtime.calls[0] == "base" && runtime.calls[1] == "mouse" &&
              runtime.calls[3] == "names" &&
              runtime.calls[4] == "read byte" &&
              runtime.calls[6] == "clear flag" &&
              runtime.calls[7] == "fly text" &&
              runtime.calls[11] == "events" &&
              runtime.calls[12] == "assign text",
          "constructor setup order");
    Check(runtime.bytes.at(Runtime::kFrame + 0x258u) == 0x57u &&
              runtime.words.at(Runtime::kFrame) == 0x843ca8u &&
              runtime.words.at(Runtime::kFrame + 0x28u) == 0x843c80u &&
              runtime.words.at(Runtime::kFrame + 0x268u) == 0x3c23d70au &&
              runtime.names.at(Runtime::kFrame + 0x258u) == "WorldMapFly",
          "constructor fields and fly name");
    for (std::uint32_t i = 0u; i < expected.size(); ++i) {
        Check(runtime.names.at(Runtime::kFrame + 0x94u + 0x10u * i) ==
                  expected[i], "location order and stride");
    }
    Check(runtime.allocated.size() == 16u &&
              runtime.allocated.front() == Runtime::kFrame + 0xe4u &&
              runtime.allocated.back() == Runtime::kFrame + 0x1d4u,
          "only later names use buffer allocation");
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.reject_address = Runtime::kFrame + 0x104u;
    Check(pal4::WorldMapFrame::Construct(Runtime::kFrame, Runtime::kName,
                                         runtime) == Runtime::kFrame &&
              !runtime.names.contains(runtime.reject_address) &&
              runtime.names.size() == 21u,
          "failed location allocation skips copy and size update");
}

void TestUnwind(const std::string& fail_at,
                const std::vector<std::string>& expected_tail) {
    Runtime runtime;
    runtime.throw_on = fail_at;
    bool caught = false;
    try {
        (void)pal4::WorldMapFrame::Construct(Runtime::kFrame,
                                              Runtime::kName, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.size() >= expected_tail.size(),
          "constructor exception propagated");
    Check(std::vector<std::string>(runtime.calls.end() -
              static_cast<std::ptrdiff_t>(expected_tail.size()),
              runtime.calls.end()) == expected_tail,
          "constructor staged cleanup");
}

void TestDestroy(const std::uint8_t initial_count,
                 const bool release_text) {
    DestructorRuntime runtime;
    runtime.count = initial_count;
    Check(pal4::WorldMapFrame::Destroy(Runtime::kFrame, runtime) == 0x1234u,
          "destructor forwards base result");
    Check((std::find(runtime.calls.begin(), runtime.calls.end(),
                     "free:33024") != runtime.calls.end()) == release_text,
          "fly-name reference branch");
    Check(runtime.count == (release_text ? initial_count :
                            static_cast<std::uint8_t>(initial_count - 1u)),
          "fly-name reference decrement");
    for (const auto offset : {0x25cu, 0x260u, 0x264u,
                              0x30u, 0x34u, 0x38u}) {
        Check(runtime.words.at(Runtime::kFrame + offset) == 0u,
              "destructor clears text and mouse fields");
    }
    Check(runtime.calls.back() == "destroy base" &&
              runtime.calls[runtime.calls.size() - 5u] == "free:36864" &&
              runtime.calls[runtime.calls.size() - 6u] == "read word" &&
              runtime.calls[runtime.calls.size() - 7u] == "destroy names",
          "destructor cleanup order");
}

void TestDestroyNullText() {
    DestructorRuntime runtime;
    runtime.words[Runtime::kFrame + 0x25cu] = 0u;
    (void)pal4::WorldMapFrame::Destroy(Runtime::kFrame, runtime);
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "read byte") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "free:33024") == runtime.calls.end(),
          "null fly-name skips reference access");
}

}  // namespace

int main() {
    TestConstruction();
    TestAllocationFailure();
    TestUnwind("mouse", {"mouse", "destroy base"});
    TestUnwind("names", {"names", "destroy mouse", "destroy base"});
    TestUnwind("fly text", {"fly text", "destroy names",
                             "destroy mouse", "destroy base"});
    TestUnwind("events", {"events", "destroy fly", "destroy names",
                           "destroy mouse", "destroy base"});
    TestUnwind("copy text", {"copy text", "destroy fly", "destroy names",
                              "destroy mouse", "destroy base"});
    TestDestroy(0u, true);
    TestDestroy(0xffu, true);
    TestDestroy(2u, false);
    TestDestroyNullText();
    return EXIT_SUCCESS;
}
