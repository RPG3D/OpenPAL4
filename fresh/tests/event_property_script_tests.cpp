#include "pal4/event_property_script.h"

#include <cstdlib>
#include <iostream>
#include <map>
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

struct Runtime final : pal4::IEventPropertyScriptRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t found{0xa000u};
    std::uint32_t text_data{0xb001u};
    bool throw_on_find{};
    bool throw_on_set{};

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return 0xdeadu;
    }
    std::uint32_t NameRecord(const std::uint32_t source,
                             const std::uint32_t slot) override {
        calls.push_back("name:" + std::to_string(source) + ":" +
                        std::to_string(slot));
        return 0x6000u;
    }
    std::uint32_t StringLength(const std::uint32_t text) override {
        calls.push_back("length:" + std::to_string(text));
        return 5u;
    }
    void SetErrorFlag(pal4::EventScriptSearchState& search,
                      const std::uint8_t flag) override {
        calls.push_back("flag:" + std::to_string(search.tag) + ":" +
                        std::to_string(flag));
    }
    void SetSearchText(pal4::EventScriptSearchState& search,
                        const std::uint32_t text,
                        const std::uint32_t length) override {
        calls.push_back("search_text:" + std::to_string(text) + ":" +
                        std::to_string(length));
        search.first = text;
        search.second = length;
    }
    std::uint32_t FindNode(
        const std::uint32_t tree,
        const pal4::EventScriptSearchState& search) override {
        calls.push_back("find:" + std::to_string(tree) + ":" +
                        std::to_string(search.first) + ":" +
                        std::to_string(search.second));
        if (throw_on_find) throw std::runtime_error("find");
        return found;
    }
    void ConvertAttribute(const std::uint32_t attribute,
                          pal4::EventScriptTextState& output,
                          const std::uint32_t empty_text) override {
        calls.push_back("convert:" + std::to_string(attribute) + ":" +
                        std::to_string(empty_text));
        output = {0x1111u, text_data};
    }
    void SetRecord(const std::uint32_t source,
                   const std::uint32_t key,
                   const pal4::EventScriptTextState& text,
                   const std::uint32_t slot) override {
        calls.push_back("set_record:" + std::to_string(source) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(text.value) + ":" +
                        std::to_string(text.data) + ":" +
                        std::to_string(slot));
        if (throw_on_set) throw std::runtime_error("set record");
    }
    void InitializeString(pal4::StringBufferState& output,
                          const std::uint32_t text) override {
        calls.push_back("init_string:" + std::to_string(text));
        output = {text, 5u, 6u};
    }
    void CleanupString(pal4::StringBufferState& output) override {
        calls.push_back("cleanup_string:" +
                        std::to_string(output.data));
        output = {};
    }
    void SetString(const std::uint32_t source,
                   const std::uint32_t key,
                   const pal4::StringBufferState& text,
                   const std::uint32_t slot) override {
        calls.push_back("set_string:" + std::to_string(source) + ":" +
                        std::to_string(key) + ":" +
                        std::to_string(text.data) + ":" +
                        std::to_string(slot));
        if (throw_on_set) throw std::runtime_error("set string");
    }
};

Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[0x4004u] = 0x5000u;
    runtime.words[0x4008u] = 0x33u;
    runtime.words[0x6004u] = 0x7000u;
    runtime.words[0x5018u] = 0x9000u;
    runtime.bytes[0xb000u] = 2u;
    return runtime;
}

void TestDirectAndMissing() {
    auto runtime = MakeRuntime();
    Check(pal4::EventPropertyScript::SetFromScript(
              0x4000u, 0x8005u, runtime) == 0xb000u &&
              runtime.bytes.at(0xb000u) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "name:32773:4", "read:24580", "read:16388",
                  "flag:5:0", "length:28672",
                  "search_text:28672:5", "find:20500:28672:5",
                  "flag:5:1", "read:20504",
                  "convert:40988:9323100", "read:16392",
                  "set_record:32773:51:4369:45057:20",
                  "read_byte:45056", "write_byte:45056:1"},
          "script record path searches, sets and decrements text reference");

    runtime.found = 0x9000u;
    runtime.calls.clear();
    Check(pal4::EventPropertyScript::SetFromScript(
              0x4000u, 0x8005u, runtime) == 0x9000u &&
              runtime.calls.back() == "read:20504",
          "tree end sentinel skips conversion and setter");
    runtime.found = 0xffffffe4u;
    runtime.calls.clear();
    Check(pal4::EventPropertyScript::SetFromScript(
              0x4000u, 0x8005u, runtime) == 0xffffffe4u &&
              runtime.calls.back() == "read:20504",
          "minus-twenty-eight sentinel also skips conversion");
}

void TestTextLifetimeAndExceptions() {
    auto runtime = MakeRuntime();
    runtime.bytes[0xb000u] = 0u;
    Check(pal4::EventPropertyScript::SetFromScript(
              0x4000u, 0x8005u, runtime) == 0xdeadu &&
              runtime.calls.back() == "free:45056",
          "zero reference header frees the text block");
    runtime.bytes[0xb000u] = 0xffu;
    runtime.calls.clear();
    Check(pal4::EventPropertyScript::SetFromScript(
              0x4000u, 0x8005u, runtime) == 0xdeadu &&
              runtime.calls.back() == "free:45056",
          "255 reference header also frees the text block");
    runtime.text_data = 0u;
    runtime.calls.clear();
    Check(pal4::EventPropertyScript::SetFromScript(
              0x4000u, 0x8005u, runtime) == 0u &&
              runtime.calls.back() == "set_record:32773:51:4369:0:20",
          "null converted text has no reference header to release");

    runtime.text_data = 0xb001u;
    runtime.bytes[0xb000u] = 2u;
    runtime.throw_on_set = true;
    runtime.calls.clear();
    bool caught = false;
    try {
        (void)pal4::EventPropertyScript::SetFromScript(
            0x4000u, 0x8005u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.bytes.at(0xb000u) == 1u &&
              runtime.calls.back() == "write_byte:45056:1",
          "record setter exception releases converted text");

    runtime.throw_on_set = false;
    runtime.throw_on_find = true;
    runtime.calls.clear();
    caught = false;
    try {
        (void)pal4::EventPropertyScript::SetFromScript(
            0x4000u, 0x8005u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "flag:5:1",
          "tree lookup exception resets search error flag");
}

void TestStringTemporary() {
    auto runtime = MakeRuntime();
    Check(pal4::EventPropertyScript::SetStringFromScript(
              0x4000u, 0x8005u, runtime) == 0xb000u &&
              runtime.calls[runtime.calls.size() - 6u] ==
                  "init_string:45057" &&
              runtime.calls[runtime.calls.size() - 3u] ==
                  "cleanup_string:45057" &&
              runtime.calls.back() == "write_byte:45056:1",
          "string script path cleans temporary before reference header");
    runtime.text_data = 0u;
    runtime.calls.clear();
    Check(pal4::EventPropertyScript::SetStringFromScript(
              0x4000u, 0x8005u, runtime) == 0u &&
              runtime.calls[runtime.calls.size() - 4u] ==
                  "init_string:" + std::to_string(
                      pal4::StringBuffer::EmptyLogMessageAddress()),
          "null converted text initializes temporary from shared empty text");
    runtime.text_data = 0xb001u;
    runtime.bytes[0xb000u] = 2u;
    runtime.throw_on_set = true;
    runtime.calls.clear();
    bool caught = false;
    try {
        (void)pal4::EventPropertyScript::SetStringFromScript(
            0x4000u, 0x8005u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls[runtime.calls.size() - 3u] ==
              "cleanup_string:45057" &&
              runtime.calls.back() == "write_byte:45056:1",
          "string setter exception cleans both temporary layers");

    pal4::EventPropertyScriptRuntimeStub stub;
    Check(stub.ReadWord(0u) == 0u && stub.ReadByte(0u) == 0u &&
              stub.Free(0u) == 0u && stub.FindNode(0u, {}) == 0u,
          "native script property boundary has a linkable stub");
}

}  // namespace

int main() {
    TestDirectAndMissing();
    TestTextLifetimeAndExceptions();
    TestStringTemporary();
}
