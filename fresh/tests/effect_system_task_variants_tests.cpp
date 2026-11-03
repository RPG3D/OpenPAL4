#include "pal4/effect_system_task_variants.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemTaskVariantRuntime {
    std::uint32_t text = 0u;
    std::uint8_t count = 0u;
    std::vector<std::string> calls;

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read_word:" + std::to_string(address));
        return text;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return count;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void ResetMessageString(std::uint32_t object) override {
        calls.push_back("reset_message:" + std::to_string(object));
    }
    void ResetSlowMotion(std::uint32_t object) override {
        calls.push_back("reset_slow:" + std::to_string(object));
    }
    std::uint32_t CleanupCommon(std::uint32_t object) override {
        calls.push_back("cleanup:" + std::to_string(object));
        return 0x7777u;
    }
};

void TestMessageCleanup(std::uint32_t text, std::uint8_t count,
                        const char* release_call) {
    Runtime runtime;
    runtime.text = text;
    runtime.count = count;
    Check(pal4::EffectSystemTaskVariants::CleanupMessage(0x5000u, runtime) ==
              0x7777u,
          "MessageTask returns common cleanup result");
    std::vector<std::string> expected{
        "word:20480:8696536", "reset_message:20480", "read_word:20556"};
    if (text != 0u) {
        expected.push_back("read_byte:" + std::to_string(text - 1u));
        expected.emplace_back(release_call);
    }
    expected.insert(expected.end(), {"word:20556:0", "word:20560:0",
                                     "word:20564:0", "cleanup:20480"});
    Check(runtime.calls == expected,
          "MessageTask releases text and clears fields in IDA order");
}

void TestSlowMotionCleanup() {
    Runtime runtime;
    Check(pal4::EffectSystemTaskVariants::CleanupSlowMotion(0x5000u, runtime) ==
              0x7777u &&
              runtime.calls == std::vector<std::string>{
                  "word:20480:8696600", "reset_slow:20480",
                  "cleanup:20480"},
          "SlowMotionTask reset precedes common cleanup");
}

}  // namespace

int main() {
    Check(pal4::EffectSystemTaskVariants::MessageTypeName() == "MessageTask" &&
              pal4::EffectSystemTaskVariants::SlowMotionTypeName() ==
                  "SlowMotionTask",
          "virtual type names match IDA strings");
    TestMessageCleanup(0u, 0u, "");
    TestMessageCleanup(0x6000u, 0u, "free:24575");
    TestMessageCleanup(0x6000u, 0xffu, "free:24575");
    TestMessageCleanup(0x6000u, 3u, "byte:24575:2");
    TestSlowMotionCleanup();
}
