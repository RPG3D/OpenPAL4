#include "pal4/combat_log_stream.h"
#include "pal4/combat_log_buffer.h"

#include <cstdint>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Buffer final : std::streambuf {
    std::string data;
    int sync_count = 0;
    bool fail_overflow = false;
    bool fail_sync = false;

    int_type overflow(int_type character) override {
        if (fail_overflow) return traits_type::eof();
        if (!traits_type::eq_int_type(character, traits_type::eof()))
            data.push_back(traits_type::to_char_type(character));
        return character;
    }
    int sync() override {
        ++sync_count;
        return fail_sync ? -1 : 0;
    }
};

struct Runtime final : pal4::ICombatLogTextRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t result = 0u;
    std::uint32_t source = 0u;
    std::uint32_t length = 0u;
    pal4::CombatLogTextSource kind = pal4::CombatLogTextSource::Empty;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void AssignText(std::uint32_t incoming_result,
                    std::uint32_t incoming_source,
                    std::uint32_t incoming_length,
                    pal4::CombatLogTextSource incoming_kind) override {
        result = incoming_result;
        source = incoming_source;
        length = incoming_length;
        kind = incoming_kind;
    }
};

struct IosRuntime final : pal4::ICombatLogIosRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    void InitializeBase(std::uint32_t ios) override {
        calls.push_back("init:" + std::to_string(ios));
    }
    void Clear(std::uint32_t ios, std::uint32_t state,
               bool raise) override {
        calls.push_back("clear:" + std::to_string(ios) + ":" +
                        std::to_string(state) + ":" +
                        std::to_string(raise));
    }
    void AddStandardStream(std::uint32_t ios) override {
        calls.push_back("add-standard:" + std::to_string(ios));
    }
};

void WriteAndFlush() {
    Buffer buffer;
    std::ostream stream(&buffer);
    Check(&pal4::CombatLogStream::WriteEntry(stream, 0x41u) == &stream &&
              &pal4::CombatLogStream::WriteEntry(stream, 0x0au) == &stream &&
              buffer.data == "A\n" && buffer.sync_count == 0,
          "write returns stream and emits exact bytes");
    Check(&pal4::CombatLogStream::Flush(stream) == &stream &&
              buffer.sync_count == 1 && stream.good(),
          "flush synchronizes stream buffer");
    stream.setf(std::ios_base::unitbuf);
    static_cast<void>(pal4::CombatLogStream::WriteEntry(stream, 0xffu));
    Check(buffer.data.size() == 3u &&
              static_cast<unsigned char>(buffer.data.back()) == 0xffu &&
              buffer.sync_count == 2,
          "unitbuf flushes after writing unsigned byte");
}

void FailureStates() {
    Buffer overflow_buffer;
    overflow_buffer.fail_overflow = true;
    std::ostream overflow_stream(&overflow_buffer);
    static_cast<void>(pal4::CombatLogStream::WriteEntry(overflow_stream, 'x'));
    Check(overflow_stream.bad() && overflow_buffer.data.empty(),
          "failed sputc sets badbit");

    Buffer blocked_buffer;
    std::ostream blocked_stream(&blocked_buffer);
    blocked_stream.setstate(std::ios_base::failbit);
    static_cast<void>(pal4::CombatLogStream::WriteEntry(blocked_stream, 'x'));
    Check(blocked_buffer.data.empty() && blocked_stream.fail(),
          "failed sentry prevents write");

    Buffer sync_buffer;
    sync_buffer.fail_sync = true;
    std::ostream sync_stream(&sync_buffer);
    static_cast<void>(pal4::CombatLogStream::Flush(sync_stream));
    Check(sync_stream.bad() && sync_buffer.sync_count == 1,
          "failed flush marks stream bad");
}

void FormattedByte() {
    std::ostringstream right;
    right.fill('.');
    right.width(4);
    Check(&pal4::CombatLogStream::WriteFormatted(right, 'Q') == &right &&
              right.str() == "...Q" && right.width() == 0,
          "formatted byte inserts right padding and clears width");

    std::ostringstream left;
    left.setf(std::ios_base::left, std::ios_base::adjustfield);
    left.fill('_');
    left.width(3);
    static_cast<void>(pal4::CombatLogStream::WriteFormatted(left, 0xffu));
    Check(left.str().size() == 3u &&
              static_cast<unsigned char>(left.str()[0]) == 0xffu &&
              left.str().substr(1) == "__" && left.width() == 0,
          "formatted byte inserts left padding and preserves high-bit byte");

    std::ostringstream narrow;
    narrow.width(1);
    static_cast<void>(pal4::CombatLogStream::WriteFormatted(narrow, 'X'));
    Check(narrow.str() == "X" && narrow.width() == 0,
          "width one writes without padding");

    Buffer blocked_buffer;
    std::ostream blocked(&blocked_buffer);
    blocked.width(5);
    blocked.setstate(std::ios_base::failbit);
    static_cast<void>(pal4::CombatLogStream::WriteFormatted(blocked, 'X'));
    Check(blocked_buffer.data.empty() && blocked.fail(),
          "failed sentry prevents formatted byte output");

    Buffer failed_buffer;
    failed_buffer.fail_overflow = true;
    std::ostream failed(&failed_buffer);
    failed.width(3);
    static_cast<void>(pal4::CombatLogStream::WriteFormatted(failed, 'X'));
    Check(failed.bad(), "failed padding marks stream bad");
}

void GetStringDelegation() {
    Runtime runtime;
    runtime.words[0x1004u + 68u] = 0u;
    runtime.words[0x1004u + 32u] = 0x3000u;
    runtime.words[0x3000u] = 0x4003u;
    runtime.words[0x1004u + 60u] = 0x4005u;
    runtime.words[0x1004u + 16u] = 0x3004u;
    runtime.words[0x3004u] = 0x4000u;
    Check(pal4::CombatLogStream::GetString(0x1000u, 0x2000u,
                                           runtime) == 0x2000u &&
              runtime.result == 0x2000u &&
              runtime.source == 0x4000u &&
              runtime.length == 5u &&
              runtime.kind == pal4::CombatLogTextSource::Output,
          "wrapper extracts actual embedded output buffer");
}

void IosInitialization() {
    IosRuntime connected;
    Check(pal4::CombatLogStream::InitializeIos(
              0x1000u, 0x123400u, 0u, connected) == 0x123400u &&
              connected.words[0x1000u + 40u] == 0x123400u &&
              connected.words[0x1000u + 44u] == 0u &&
              connected.bytes[0x1000u + 48u] == 32u,
          "connected ios initializes buffer, state and fill");
    Check(connected.calls == std::vector<std::string>{
              "write:4136:1192960", "write:4140:0", "byte:4144:32",
              "init:4096", "read:4136"},
          "connected stream skips failbit and standard registration");

    IosRuntime disconnected;
    disconnected.words[0x1000u + 4u] = 0x120u;
    Check(pal4::CombatLogStream::InitializeIos(
              0x1000u, 0u, 1u, disconnected) == 1u,
          "standard flag replaces low return byte");
    Check(disconnected.calls == std::vector<std::string>{
              "write:4136:0", "write:4140:0", "byte:4144:32",
              "init:4096", "read:4136", "read:4100",
              "clear:4096:292:0", "add-standard:4096"},
          "disconnected ios clears failbit before standard registration");
}

}  // namespace

int main() {
    try {
        WriteAndFlush();
        FailureStates();
        FormattedByte();
        GetStringDelegation();
        IosInitialization();
        std::cout << "combat log stream tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
