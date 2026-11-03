#include "pal4/animation_string_file_reader.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IAnimationStringFileRuntime {
public:
    std::uint32_t raw_length = 0;
    std::string payload;
    bool read_success = true;
    std::uint32_t temporary = 0xa000u;
    std::map<std::uint32_t, std::uint8_t> references;
    std::map<std::uint32_t, std::string> output;
    std::vector<std::string> calls;

    void ReadLength(std::uint32_t file,
                    std::uint32_t& length) override {
        Check(file == 0x100u, "length read uses source file");
        calls.emplace_back("length");
        length = raw_length;
    }
    bool ReadPayload(std::uint32_t file, char* destination,
                     std::uint32_t length) override {
        Check(file == 0x100u && length == raw_length,
              "payload read uses announced byte count");
        calls.emplace_back("payload");
        if (!read_success) return false;
        Check(payload.size() >= length, "payload fixture contains enough bytes");
        for (std::uint32_t index = 0; index < length; ++index)
            destination[index] = payload[index];
        return true;
    }
    std::uint32_t AllocateTemporary(std::uint32_t bytes) override {
        calls.emplace_back("allocate:" + std::to_string(bytes));
        return temporary;
    }
    void FreeTemporary(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    std::uint8_t ReadReference(std::uint32_t address) override {
        calls.emplace_back("reference:" + std::to_string(address));
        return references[address];
    }
    void WriteReference(std::uint32_t address,
                        std::uint8_t value) override {
        calls.emplace_back("decrement:" + std::to_string(address));
        references[address] = value;
    }
    void SetErrorFlag(pal4::StringBufferState& state,
                      std::uint8_t flag) override {
        calls.emplace_back("flag:" + std::to_string(flag));
        state = {};
    }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        calls.emplace_back("reallocate:" + std::to_string(length));
        state.data = 0x9000u;
        state.capacity = length;
    }
    void WriteData(std::uint32_t destination,
                   std::string_view data) override {
        calls.emplace_back("write:" + std::to_string(destination));
        output[destination] = std::string(data);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("zero:" + std::to_string(address));
        Check(value == 0u, "string terminator is zero");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::StringBufferState state{0x1000u, 9u, 10u};
    runtime.raw_length = 3u;
    runtime.payload = "abc";
    runtime.read_success = false;
    Check(!pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "length", "payload"} &&
              state.data == 0x1000u && state.length == 9u,
          "short read failure leaves destination unchanged");

    runtime = {};
    state = {0x1000u, 9u, 10u};
    runtime.raw_length = 3u;
    runtime.payload = "abc";
    runtime.references[0xfffu] = 2u;
    Check(pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "length", "payload", "reference:4095", "reallocate:3",
                  "write:36864", "zero:36867"} &&
              runtime.output[0x9000u] == "abc" && state.length == 3u,
          "shared short text detaches before copying bytes and terminator");

    runtime = {};
    state = {0x1000u, 9u, 10u};
    runtime.raw_length = 0u;
    runtime.references[0xfffu] = 3u;
    Check(pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.references[0xfffu] == 2u &&
              runtime.calls == std::vector<std::string>{
                  "length", "payload", "reference:4095",
                  "decrement:4095", "flag:0"},
          "empty text decrements shared reference before clearing");

    runtime = {};
    state = {0x1000u, 9u, 10u};
    runtime.references[0xfffu] = 0xffu;
    Check(pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.calls.back() == "flag:1" &&
              runtime.references[0xfffu] == 0xffu,
          "empty unshared text takes release flag path");

    runtime = {};
    state = {0x1000u, 0u, 20u};
    runtime.raw_length = 3u;
    runtime.payload = "xyz";
    runtime.references[0xfffu] = 0u;
    Check(pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "length", "payload", "reference:4095",
                  "write:4096", "zero:4099"} &&
              state.data == 0x1000u && state.length == 3u,
          "unshared small-capacity buffer is reused");

    runtime = {};
    state = {0x1000u, 0u, 64u};
    runtime.raw_length = 3u;
    runtime.payload = "xyz";
    Check(pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.calls[3] == "flag:1" &&
              runtime.calls[4] == "reallocate:3" &&
              state.data == 0x9000u,
          "large-capacity destination resets and reallocates");

    runtime = {};
    state = {0x1000u, 0u, 20u};
    runtime.raw_length = 64u;
    runtime.payload = std::string("hello\0", 6) + std::string(58, 'x');
    Check(pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.calls[1] == "allocate:65" &&
              runtime.calls[2] == "payload" &&
              runtime.calls.back() == "free:40960" &&
              state.length == 5u && runtime.output[0x1000u] == "hello",
          "long payload uses temporary allocation and stops at embedded NUL");

    runtime = {};
    state = {0x1000u, 9u, 10u};
    runtime.raw_length = 64u;
    runtime.read_success = false;
    Check(!pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "length", "allocate:65", "payload", "free:40960"} &&
              state.length == 9u,
          "failed long read releases temporary allocation");

    runtime = {};
    state = {0x1000u, 9u, 10u};
    runtime.raw_length = 64u;
    runtime.temporary = 0u;
    Check(!pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "length", "allocate:65"},
          "temporary allocation failure leaves state unchanged");

    runtime = {};
    runtime.raw_length = 0xffffffffu;
    Check(!pal4::AnimationStringFileReader::Load(state, 0x100u, runtime) &&
              runtime.calls == std::vector<std::string>{"length"},
          "invalid announced length is rejected before allocation");

    pal4::AnimationStringFileRuntimeStub stub;
    Check(!pal4::AnimationStringFileReader::Load(state, 0u, stub),
          "native runtime stub is linkable");
    std::cout << "animation_string_file_reader_tests passed\n";
}
