#include "pal4/game_data_equipment_attribute_codec.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Event {
    char operation;
    std::uint32_t stream;
    std::uint32_t address;
    std::uint32_t bytes;

    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::IGameDataEquipmentAttributeCodecRuntime {
    std::vector<Event> calls;
    std::uint32_t fail_at = 0;

    void Add(char operation, std::uint32_t stream,
             std::uint32_t address, std::uint32_t bytes) {
        Check(stream == 0x8000, "stream forwarded to every operation");
        calls.push_back({operation, stream, address, bytes});
        if (fail_at == calls.size()) throw std::runtime_error("injected I/O");
    }
    std::uint32_t ReadRaw(std::uint32_t stream, std::uint32_t destination,
                          std::uint32_t bytes) override {
        Add('r', stream, destination, bytes);
        return bytes == 0x90 ? 0x1234 : 0xaaaa;
    }
    std::uint32_t WriteRaw(std::uint32_t stream, std::uint32_t source,
                           std::uint32_t bytes) override {
        Add('w', stream, source, bytes);
        return bytes == 0x90 ? 0x5678 : 0xbbbb;
    }
    void ReadText(std::uint32_t stream,
                  std::uint32_t destination) override {
        Add('t', stream, destination, 16);
    }
    void WriteText(std::uint32_t stream,
                   std::uint32_t source) override {
        Add('T', stream, source, 16);
    }
};

void CheckSequence(const Runtime& runtime, char raw, char text) {
    Check(runtime.calls.size() == 26, "exactly 26 stream operations");
    Check(runtime.calls[0] == Event{raw, 0x8000, 0x1090, 4} &&
              runtime.calls[1] == Event{raw, 0x8000, 0x1094, 4},
          "two header DWORDs lead the stream");
    for (std::uint32_t index = 0; index < 13; ++index)
        Check(runtime.calls[2 + index] ==
                  Event{text, 0x8000, 0x1098 + index * 16, 16},
              "13 text records use 16-byte stride");
    for (std::uint32_t index = 0; index < 10; ++index)
        Check(runtime.calls[15 + index] ==
                  Event{raw, 0x8000, 0x1168 + index * 4, 4},
              "four then six raw words use four-byte stride");
    Check(runtime.calls[25] == Event{raw, 0x8000, 0x1000, 0x90},
          "base block is transferred last");
}

void CheckReadAndWrite() {
    Runtime runtime;
    Check(pal4::GameDataEquipmentAttributeCodec::Deserialize(
              0x1000, 0x8000, runtime) == 0x1234,
          "deserialize forwards the final raw-read result");
    CheckSequence(runtime, 'r', 't');
    runtime.calls.clear();
    Check(pal4::GameDataEquipmentAttributeCodec::Serialize(
              0x1000, 0x8000, runtime) == 0x5678,
          "serialize forwards the final raw-write result");
    CheckSequence(runtime, 'w', 'T');
}

void CheckFailureStopsSequence() {
    Runtime runtime;
    runtime.fail_at = 4;
    bool failed = false;
    try {
        (void)pal4::GameDataEquipmentAttributeCodec::Deserialize(
            0x1000, 0x8000, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.size() == 4 &&
              runtime.calls.back() == Event{'t', 0x8000, 0x10a8, 16},
          "I/O failure stops at the failing text record");
}

}  // namespace

int main() {
    try {
        CheckReadAndWrite();
        CheckFailureStopsSequence();
        std::cout << "game_data_equipment_attribute_codec_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
