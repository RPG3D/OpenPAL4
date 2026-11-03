#include "pal4/string_stream_destructor.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IStringStreamDestructorRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool throw_on_free = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(words.contains(address), "destructor reads mapped native word");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
        words[address] = value;
    }
    void Free(const std::uint32_t address) override {
        Check(address == 0x4000u, "owned string buffer address");
        calls.emplace_back("free");
        if (throw_on_free) throw std::runtime_error("free");
    }
    void DestroyLocale(const std::uint32_t function,
                       const std::uint32_t locale,
                       const std::uint32_t flags) override {
        Check(function == 0x7000u && locale == 0x5000u && flags == 1u,
              "locale virtual destructor target and flag");
        calls.emplace_back("destroy-locale");
    }
    void DestroyFileStream(const std::uint32_t address) override {
        Check(address == 0x100cu, "unwind uses streambuf -0x4c");
        calls.emplace_back("destroy-file-stream");
    }
};

Runtime Fixture(const std::uint32_t flags, const std::uint32_t locale_count) {
    Runtime runtime;
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x2004u] = 0u;
    runtime.words[0x1008u] = 0x2100u;
    runtime.words[0x2104u] = 0u;
    runtime.words[0x84a054u] = 0u;
    runtime.words[0x849fb0u] = 0u;
    runtime.words[0x1050u] = flags;
    runtime.words[0x1018u] = 0x3000u;
    runtime.words[0x3000u] = 0x4000u;
    runtime.words[0x1040u] = 0x5000u;
    runtime.words[0x5004u] = locale_count;
    runtime.words[0x5000u] = 0x6000u;
    runtime.words[0x6000u] = 0x7000u;
    return runtime;
}

void TestOwnedBufferAndLocale() {
    Runtime runtime = Fixture(5u, 1u);
    Check(pal4::StringStreamDestructor::Destroy(0x1058u, runtime) ==
              0x849facu &&
              runtime.words[0x1000u] == 0x8682c0u &&
              runtime.words[0x1008u] == 0x8682c8u &&
              runtime.words[0x100cu] == 0x868310u &&
              runtime.words[0x1048u] == 0u &&
              runtime.words[0x1050u] == 4u &&
              runtime.words[0x5004u] == 0u,
          "destructor installs staged vtables and releases final locale");
    Check(runtime.calls == std::vector<std::string>{
              "write:4096:8691792", "write:4108:8683852", "free",
              "write:4168:0", "write:4108:8815376", "write:4176:4",
              "write:20484:0", "destroy-locale", "write:4096:8691628",
              "write:4104:8815304", "write:4096:8815296"},
          "native staged writes, free and virtual destructor order");
}

void TestReferenceClasses() {
    for (const auto count : {0u, 2u, 0xffffffffu}) {
        Runtime runtime = Fixture(0u, count);
        (void)pal4::StringStreamDestructor::Destroy(0x1058u, runtime);
        Check(runtime.words[0x5004u] == (count == 2u ? 1u : count),
              "locale count only decrements finite nonzero values");
        bool destroyed = false;
        bool freed_buffer = false;
        for (const auto& call : runtime.calls) {
            destroyed |= call == "destroy-locale";
            freed_buffer |= call == "free";
        }
        Check(destroyed == (count == 0u) && !freed_buffer,
              "zero count destroys locale without freeing unowned buffer");
    }
    Runtime no_locale = Fixture(0u, 2u);
    no_locale.words[0x1040u] = 0u;
    (void)pal4::StringStreamDestructor::Destroy(0x1058u, no_locale);
    Check(no_locale.words[0x5004u] == 2u,
          "null locale bypasses reference-count access");
}

void TestVirtualBaseOffsetAndUnwind() {
    Runtime offset = Fixture(0u, 2u);
    offset.words[0x2004u] = 4u;
    offset.words[0x2104u] = 0x20u;
    Check(pal4::StringStreamDestructor::Destroy(0x1058u, offset) ==
              0x2000u &&
              offset.words[0x1004u] == 0x8682c0u &&
              offset.words[0x1028u] == 0x8682c8u,
          "virtual-base offsets determine each vtable write address");

    Runtime throwing = Fixture(1u, 1u);
    throwing.throw_on_free = true;
    bool caught = false;
    try {
        (void)pal4::StringStreamDestructor::Destroy(0x1058u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.calls.back() == "destroy-file-stream" &&
              throwing.words[0x100cu] == 0x84814cu,
          "throwing free invokes attached FileStream unwind tail");
}

}  // namespace

int main() {
    TestOwnedBufferAndLocale();
    TestReferenceClasses();
    TestVirtualBaseOffsetAndUnwind();
}
