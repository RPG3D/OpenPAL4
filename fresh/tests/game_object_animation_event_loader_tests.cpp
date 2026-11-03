#include "pal4/game_object_animation_event_loader.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

template <typename T>
void Append(std::vector<std::byte>& bytes, const T& value) {
    const auto* first = reinterpret_cast<const std::byte*>(&value);
    bytes.insert(bytes.end(), first, first + sizeof(value));
}

void AppendHeader(std::vector<std::byte>& bytes, std::string_view header) {
    const auto length = static_cast<std::uint32_t>(header.size());
    Append(bytes, length);
    for (char ch : header) bytes.push_back(static_cast<std::byte>(ch));
}

std::uint32_t Field(const pal4::AnimationEventData& event, std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, event.data() + offset, sizeof(value));
    return value;
}

class Runtime final : public pal4::IGameObjectAnimationEventRuntime {
public:
    std::vector<std::string> calls;
    std::vector<std::byte> input;
    std::vector<pal4::AnimationEventData> events;
    std::size_t cursor = 0;
    std::uint32_t open_result = 0x1234u;
    bool sidecar_result = false;

    std::string_view RecordBasePath(std::uint32_t record) override {
        Check(record == 0x1000u, "base path record");
        calls.emplace_back("base");
        return "data/action/";
    }
    std::string_view RecordName(std::uint32_t record) override {
        Check(record == 0x1000u, "name record");
        calls.emplace_back("name");
        return "walk";
    }
    std::uint32_t OpenStream(std::uint32_t mode, std::uint32_t flags,
                             std::string_view path) override {
        Check(mode == 2 && flags == 1 && path == "data/action/walk.amf",
              "AMF path and stream flags");
        calls.emplace_back("open");
        return open_result;
    }
    void ReadStream(std::uint32_t stream, std::span<std::byte> bytes) override {
        Check(stream == open_result && cursor + bytes.size() <= input.size(),
              "stream read stays within fixture");
        calls.emplace_back("read:" + std::to_string(bytes.size()));
        if (bytes.size() == 0x14cu) {
            Check(Field(*reinterpret_cast<const pal4::AnimationEventData*>(
                            bytes.data()), 192) == 0xbf800000u &&
                      Field(*reinterpret_cast<const pal4::AnimationEventData*>(
                            bytes.data()), 200) == 0xbf800000u,
                  "event is initialized before stream read");
        }
        std::copy_n(input.data() + cursor, bytes.size(), bytes.data());
        cursor += bytes.size();
    }
    void CloseStream(std::uint32_t stream, std::uint32_t flags) override {
        Check(stream == open_result && flags == 0, "stream close arguments");
        calls.emplace_back("close");
    }
    void ClearEvents(std::uint32_t record) override {
        Check(record == 0x1000u, "old event range record");
        calls.emplace_back("clear");
        events.clear();
    }
    void InsertEvent(std::uint32_t record,
                     const pal4::AnimationEventData& event) override {
        Check(record == 0x1000u, "event insert record");
        calls.emplace_back("insert");
        events.push_back(event);
    }
    bool LoadSidecar(std::uint32_t record, std::string_view path) override {
        Check(record == 0x1000u && path == "data/action/walk.amd",
              "AMD sidecar path and record");
        calls.emplace_back("sidecar");
        return sidecar_result;
    }
};

void NullAndOpenFailure() {
    Runtime runtime;
    Check(!pal4::GameObjectAnimationEventLoader::Load(0, runtime) &&
              runtime.calls.empty(), "null record has no effects");
    runtime.open_result = 0;
    Check(!pal4::GameObjectAnimationEventLoader::Load(0x1000u, runtime) &&
              runtime.calls == std::vector<std::string>{"base", "name", "open"},
          "open failure leaves events untouched and skips sidecar");
}

void SignatureAndCount() {
    Runtime runtime;
    AppendHeader(runtime.input, "different");
    Append(runtime.input, std::int32_t{2});
    Check(pal4::GameObjectAnimationEventLoader::Load(0x1000u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "base", "name", "open", "clear", "read:4", "read:9",
                  "close", "sidecar"},
          "signature mismatch clears old range but does not read count");

    for (auto count : {0, -2}) {
        Runtime empty;
        AppendHeader(empty.input,
                     pal4::GameObjectAnimationEventLoader::kSignature);
        Append(empty.input, std::int32_t{count});
        Check(pal4::GameObjectAnimationEventLoader::Load(0x1000u, empty) &&
                  empty.events.empty() &&
                  empty.calls[empty.calls.size() - 2] == "close" &&
                  empty.calls.back() == "sidecar", 
              "zero and negative counts skip event records");
    }
}

void MultipleEventsAndFailedSidecar() {
    Runtime runtime;
    AppendHeader(runtime.input,
                 pal4::GameObjectAnimationEventLoader::kSignature);
    Append(runtime.input, std::int32_t{2});
    for (std::int32_t index = 0; index < 2; ++index) {
        pal4::AnimationEventData event{};
        const auto value = 0x3f800000 + index;
        const std::int32_t prior = 77;
        std::memcpy(event.data() + 192, &value, 4);
        std::memcpy(event.data() + 196, &prior, 4);
        std::memcpy(event.data() + 200, &prior, 4);
        event[0] = static_cast<std::byte>(index + 1);
        runtime.input.insert(runtime.input.end(), event.begin(), event.end());
    }
    Check(pal4::GameObjectAnimationEventLoader::Load(0x1000u, runtime) &&
              runtime.events.size() == 2 &&
              runtime.events[0][0] == std::byte{1} &&
              runtime.events[1][0] == std::byte{2} &&
              Field(runtime.events[0], 196) == 0 &&
              Field(runtime.events[0], 200) == 0x3f800000 &&
              Field(runtime.events[1], 200) == 0x3f800001 &&
              runtime.calls[runtime.calls.size() - 2] == "close" &&
              runtime.calls.back() == "sidecar",
          "events normalize times and sidecar failure does not change success");
}

void OversizedHeader() {
    Runtime runtime;
    AppendHeader(runtime.input, std::string(300, 'x'));
    Check(pal4::GameObjectAnimationEventLoader::Load(0x1000u, runtime) &&
              runtime.cursor == runtime.input.size() &&
              runtime.events.empty() &&
              runtime.calls == std::vector<std::string>{
                  "base", "name", "open", "clear", "read:4", "read:256",
                  "read:44", "close", "sidecar"},
          "oversized header is consumed without stack overflow");
}

}  // namespace

int main() {
    NullAndOpenFailure();
    SignatureAndCount();
    MultipleEventsAndFailedSidecar();
    OversizedHeader();
    std::cout << "game_object_animation_event_loader_tests passed\n";
}
