#include "pal4/thunder_object.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderObjectRuntime {
    std::vector<std::string> events;
    std::uint32_t count = 7u;
    bool fail_write = false;

    std::uint32_t SystemInstance() override {
        events.emplace_back("system");
        return 0x5000u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x50a4u, "count field at system +164");
        events.emplace_back("read");
        return count;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == 0x50a4u, "count write address");
        events.emplace_back("write");
        if (fail_write) throw std::runtime_error("write");
        count = value;
    }
    std::uint8_t ConstructorStackByte() override {
        events.emplace_back("stack");
        return 0x5au;
    }
    void ResetErrorField(pal4::ThunderObjectState& state,
                         const std::uint32_t offset) override {
        Check(offset == 216u && state.words[0u] == 0x84ab78u,
              "error cleanup sees derived object");
        events.emplace_back("error");
    }
    void ResetProperties(pal4::ThunderObjectState&,
                         const std::uint32_t offset) override {
        Check(offset == 60u, "property cleanup offset");
        events.emplace_back("properties");
    }
    void CleanupMemoryBlock(pal4::ThunderObjectState&,
                            const std::uint32_t offset) override {
        Check(offset == 200u || offset == 184u,
              "memory block cleanup offset");
        events.emplace_back(offset == 200u ? "block200" : "block184");
    }
};

void TestLifecycle() {
    pal4::ThunderObjectState state{};
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    Check(&pal4::ThunderObject::Construct(state, runtime) == &state,
          "constructor returns object");
    Check(runtime.count == 8u && runtime.events ==
              std::vector<std::string>{"stack", "system", "read", "write"},
          "constructor increments system count after initialization");
    Check(state.words[0u] == 0x84ab78u &&
              state.words[5u] == 0x84ab6cu &&
              state.words[13u] == 0x84ab0cu,
          "derived vtables and nine facets");
    for (std::uint32_t i = 1u; i <= 4u; ++i)
        Check(state.words[i] == 0u, "base header cleared");
    Check(state.words[15u] == 0x01000100u,
          "bytes +60..63 change without clearing adjacent storage");
    Check(state.words[16u] == 0xa5000000u && state.words[17u] == 4u,
          "flags and mode at +64..+68");
    for (std::uint32_t i = 18u; i <= 20u; ++i)
        Check(state.words[i] == 0x3f800000u, "unit scale");
    for (std::uint32_t i = 21u; i <= 26u; ++i)
        Check(state.words[i] == 0u, "initial rotation and position");
    Check(state.words[27u] == 0xffffffffu &&
              state.words[28u] == 0u && state.words[29u] == 0u,
          "RGBA and initial resource pointers");
    for (std::uint32_t i = 30u; i <= 33u; ++i)
        Check(state.words[i] == 0xa5a5a5a5u,
              "constructor preserves unspecified fields");
    for (std::uint32_t i = 34u; i <= 39u; ++i)
        Check(state.words[i] == 0u, "resource and direction fields");
    Check(state.words[40u] == 0x3e4ccccdu &&
              state.words[41u] == 0x3f000000u &&
              state.words[42u] == 0x3f000000u &&
              state.words[43u] == 0x3f800000u &&
              state.words[44u] == 0x3dcccccdu &&
              state.words[45u] == 0x3ecccccdu,
          "six exact float defaults");
    for (const auto index : {46u, 50u, 54u})
        Check(state.words[index] == 0xa5a5a55au,
              "stack byte preserves upper bytes in nested blocks");
    for (const auto index : {47u, 48u, 49u, 51u, 52u, 53u,
                             55u, 56u, 57u})
        Check(state.words[index] == 0u, "nested block pointer fields");
    for (std::uint32_t i = 58u; i < state.words.size(); ++i)
        Check(state.words[i] == 0xa5a5a5a5u,
              "constructor preserves configuration cache tail");

    runtime.events.clear();
    pal4::ThunderObject::Destroy(state, runtime);
    Check(runtime.count == 7u && runtime.events ==
              std::vector<std::string>{"system", "read", "write",
                                       "error", "properties",
                                       "block200", "block184"},
          "destructor decrements and cleans in IDA order");
    Check(state.words[0u] == 0x84b240u &&
              state.words[5u] == 0x84a1a0u &&
              state.words[13u] == 0x84a178u,
          "destructor restores base vtables");
}

void TestCountFailure() {
    pal4::ThunderObjectState state{};
    Runtime runtime;
    runtime.fail_write = true;
    bool caught = false;
    try {
        (void)pal4::ThunderObject::Construct(state, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.count == 7u &&
              state.words[0u] == 0x84b240u &&
              state.words[13u] == 0x84a178u,
          "count failure unwinds derived facet vtables");
}

struct PlaybackRuntime final : pal4::IThunderObjectPlaybackRuntime {
    std::vector<std::string> events;
    void StartFacet(pal4::ThunderObjectState& state,
                    std::array<std::uint32_t, 4u>& outputs) override {
        Check((state.words[2u] >> 24u) == 1u &&
                  outputs == std::array<std::uint32_t, 4u>{},
              "active playback dispatch receives four zero outputs");
        events.emplace_back("start");
        outputs = {2u, 3u, 4u, 30u};
    }
    double Duration(pal4::ThunderObjectState&) override {
        events.emplace_back("duration");
        return 12.0;
    }
};

void TestPropertiesAndPlayback() {
    pal4::ThunderObjectState state{};
    state.words[28u] = 0x7788u;
    Check(pal4::ThunderObject::ResourceHandle(state) == 0u &&
              pal4::ThunderObject::ClassName() == "ThunderObject",
          "inactive resource gate and class name");
    PlaybackRuntime runtime;
    pal4::ThunderObjectPlayback::SetEnabled(state, 1u, runtime);
    Check(runtime.events.empty() && state.words[59u] == 0u,
          "inactive playback does not start");

    state.words[1u] = 1u;
    state.words[2u] = 0x00abcdefu;
    Check(pal4::ThunderObject::ResourceHandle(state) == 0x7788u,
          "active resource handle from +112");
    pal4::ThunderObjectPlayback::SetEnabled(state, 1u, runtime);
    Check(runtime.events == std::vector<std::string>{"start", "duration"} &&
              state.words[2u] == 0x01abcdefu &&
              state.words[59u] == std::bit_cast<std::uint32_t>(2.5f),
          "start output at fourth word divided by virtual duration");
    runtime.events.clear();
    pal4::ThunderObjectPlayback::SetEnabled(state, 1u, runtime);
    Check(runtime.events.empty(), "unchanged playback byte skips work");
    pal4::ThunderObjectPlayback::SetEnabled(state, 0u, runtime);
    Check(runtime.events.empty() && state.words[2u] == 0x00abcdefu &&
              state.words[59u] == std::bit_cast<std::uint32_t>(2.5f),
          "disable clears playback byte and retains cached ratio");
}

}  // namespace

int main() {
    TestLifecycle();
    TestCountFailure();
    TestPropertiesAndPlayback();
}
