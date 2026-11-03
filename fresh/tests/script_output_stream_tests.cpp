#include "pal4/script_output_stream.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct SyncBuffer final : std::stringbuf {
    int sync_count = 0;
    int sync() override {
        ++sync_count;
        return std::stringbuf::sync();
    }
};

struct Event {
    std::string kind;
    std::uint32_t address;
    std::uint32_t value = 0u;
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::IScriptOutputTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Event> events;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back({"read_word", address});
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back({"read_byte", address});
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back({"write_word", address, value});
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.push_back({"write_byte", address, value});
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        events.push_back({"free", address});
    }
};

void StandardStreamSubstitution() {
    std::stringbuf buffer;
    auto stream = pal4::ScriptOutputStream::Construct(&buffer);
    Check(stream != nullptr && pal4::ScriptOutputStream::CheckReady(*stream),
          "constructed ostream uses supplied streambuf");
    Check(&pal4::ScriptOutputStream::InsertInteger(*stream, -17) == stream.get(),
          "integer insertion returns the stream");
    (void)pal4::ScriptOutputStream::InsertCString(*stream, ":");
    *stream << std::hex << std::setfill('0') << std::setw(4);
    (void)pal4::ScriptOutputStream::InsertInteger(*stream, 42);
    Check(buffer.str() == "-17:002a", "integer insertion respects stream flags");
    *stream << std::dec << std::setfill(' ') << std::setw(5);
    (void)pal4::ScriptOutputStream::InsertCString(*stream, "xy");
    Check(buffer.str() == "-17:002a   xy",
          "C string insertion respects width and resets it");

    SyncBuffer tied_buffer;
    std::ostream tied(&tied_buffer);
    stream->tie(&tied);
    Check(pal4::ScriptOutputStream::CheckReady(*stream) &&
              tied_buffer.sync_count == 1,
          "ostream sentry flushes tied stream");
    stream->setstate(std::ios_base::badbit);
    Check(!pal4::ScriptOutputStream::CheckReady(*stream),
          "failed stream is not ready");
    pal4::ScriptOutputStream::Destroy(stream);
    Check(stream == nullptr, "ostream lifetime uses RAII");

    auto file = std::make_unique<std::ofstream>();
    pal4::ScriptOutputStream::DestroyFile(file);
    Check(file == nullptr, "owned file stream lifetime uses RAII");

    auto invalid = pal4::ScriptOutputStream::Construct(nullptr);
    Check(!pal4::ScriptOutputStream::CheckReady(*invalid),
          "missing stream buffer starts in failed state");
}

void TextRecordDestructor() {
    constexpr std::uint32_t object = 0x1000u;
    constexpr std::uint32_t data = 0x2001u;
    for (const auto count : {0u, 1u, 0xffu}) {
        for (std::uint8_t flags = 0u; flags < 4u; ++flags) {
            Runtime r;
            r.words[object + 4u] = data;
            r.bytes[data - 1u] = static_cast<std::uint8_t>(count);
            Check(pal4::ScriptOutputTextRecord::Destroy(object, flags, r) ==
                      object,
                  "text destructor returns original object");
            std::vector<Event> expected{{"read_word", object + 4u},
                                        {"read_byte", data - 1u}};
            if (count == 0u || count == 0xffu)
                expected.push_back({"free", data - 1u});
            else
                expected.push_back({"write_byte", data - 1u, 0u});
            expected.push_back({"write_word", object + 4u, 0u});
            expected.push_back({"write_word", object + 8u, 0u});
            expected.push_back({"write_word", object + 12u, 0u});
            if ((flags & 1u) != 0u) expected.push_back({"free", object});
            Check(r.events == expected, "text reference and deleting flags order");
        }
    }
    Runtime r;
    r.words[object + 4u] = 0u;
    Check(pal4::ScriptOutputTextRecord::Destroy(object, 0u, r) == object &&
              r.events == std::vector<Event>{{"read_word", object + 4u},
                                             {"write_word", object + 4u, 0u},
                                             {"write_word", object + 8u, 0u},
                                             {"write_word", object + 12u, 0u}},
          "null text pointer skips reference cleanup");
}

}  // namespace

int main() {
    try {
        StandardStreamSubstitution();
        TextRecordDestructor();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
