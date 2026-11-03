#include "pal4/sequence_anim_properties.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISequenceAnimPropertiesRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t color = 0u;
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x3030u) {
            events.emplace_back("geometry");
            return 0x4000u;
        }
        if (address == 0x401cu) {
            events.emplace_back("color-read");
            return color;
        }
        Check(address >= 0x3018u && address <= 0x302cu,
              "sequence resource read address");
        events.push_back("read:" + std::to_string(address - 0x3000u));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address >= 0x3018u && address <= 0x302cu,
              "sequence resource write address");
        events.push_back("write:" + std::to_string(address - 0x3000u));
        words[address] = value;
    }
    std::uint32_t InitializeTransform(std::uint32_t resource) override {
        Check(resource == 0x3000u, "position setter transform resource");
        events.emplace_back("transform");
        return 0x89abcdefu;
    }
    std::uint8_t SetGeometryColor(std::uint32_t geometry,
                                  std::uint32_t rgba) override {
        Check(geometry == 0x4000u, "color geometry handle");
        color = rgba;
        events.emplace_back("color-write");
        return 0x6du;
    }
};

void TestGetters() {
    pal4::SequenceAnimObjectState state{};
    state.words[30u] = 0x3000u;
    Runtime runtime;
    runtime.words[0x3018u] = std::bit_cast<std::uint32_t>(10.0f);
    runtime.words[0x3028u] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[0x302cu] = 0x4040007bu;
    runtime.words[0x301cu] = 0x1111u;
    runtime.words[0x3020u] = 0x2222u;
    runtime.words[0x3024u] = 0x33335au;
    std::array<std::uint32_t, 3> output{7u, 8u, 9u};
    Check(pal4::SequenceAnimProperties::GetRotation(
              state, output, 0x205du, runtime) == 0u &&
              output == std::array<std::uint32_t, 3>{7u, 8u, 9u} &&
              runtime.events.empty(),
          "inactive getter gate and untouched outputs");
    state.words[1u] = 0x100u;
    Check(pal4::SequenceAnimProperties::GetScale(
              state, output, runtime) == 0u && runtime.events.empty(),
          "getter activity uses low byte");
    state.words[1u] = 1u;

    Check(pal4::SequenceAnimProperties::GetRotation(
              state, output, 0x205du, runtime) == 0x5du &&
              output == std::array<std::uint32_t, 3>{
                  std::bit_cast<std::uint32_t>(10.0f), 0u, 0u},
          "rotation getter returns third pointer low byte");
    Check(pal4::SequenceAnimProperties::GetScale(
              state, output, runtime) == 0x7bu &&
              output == std::array<std::uint32_t, 3>{
                  std::bit_cast<std::uint32_t>(2.0f), 0x4040007bu, 0u},
          "scale getter returns second word low byte");
    Check(pal4::SequenceAnimProperties::GetPosition(
              state, output, runtime) == 0x5au &&
              output == std::array<std::uint32_t, 3>{
                  0x1111u, 0x2222u, 0x33335au},
          "position getter returns third word low byte");
    Check(runtime.events == std::vector<std::string>{
              "read:24", "read:40", "read:44", "read:28",
              "read:32", "read:36"},
          "getter resource offsets and order");
}

void TestSetters() {
    pal4::SequenceAnimObjectState state{};
    state.words[30u] = 0x3000u;
    Runtime runtime;
    runtime.words[0x3018u] = std::bit_cast<std::uint32_t>(10.0f);
    runtime.words[0x3028u] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[0x302cu] = std::bit_cast<std::uint32_t>(3.0f);
    Check(pal4::SequenceAnimProperties::SetRotation(
              state, 20.0f, 1u, 0x12345678u, runtime) == 0x12345600u &&
              pal4::SequenceAnimProperties::SetScale(
                  state, 4.0f, 5.0f, 0u, 0x12345678u, runtime) ==
                  0x12345600u && runtime.events.empty(),
          "inactive setters clear only entry EAX low byte");
    state.words[1u] = 1u;
    Check(pal4::SequenceAnimProperties::SetRotation(
              state, 20.0f, 0u, 0x12345678u, runtime) == 0x12345600u &&
              runtime.words[0x3018u] == std::bit_cast<std::uint32_t>(20.0f),
          "absolute rotation writes first axis only");
    Check(pal4::SequenceAnimProperties::SetRotation(
              state, 5.0f, 1u, 0x12345678u, runtime) == 0x3000u &&
              runtime.words[0x3018u] == std::bit_cast<std::uint32_t>(25.0f),
          "relative rotation returns resource pointer");
    Check(pal4::SequenceAnimProperties::SetScale(
              state, 4.0f, 5.0f, 0u, 0x12345678u, runtime) ==
              std::bit_cast<std::uint32_t>(5.0f) &&
              runtime.words[0x3028u] == std::bit_cast<std::uint32_t>(4.0f) &&
              runtime.words[0x302cu] == std::bit_cast<std::uint32_t>(5.0f),
          "absolute scale writes two floats");
    Check(pal4::SequenceAnimProperties::SetScale(
              state, 2.0f, 3.0f, 1u, 0x12345678u, runtime) ==
              std::bit_cast<std::uint32_t>(8.0f) &&
              runtime.words[0x3028u] == std::bit_cast<std::uint32_t>(6.0f) &&
              runtime.words[0x302cu] == std::bit_cast<std::uint32_t>(8.0f),
          "relative scale adds components");
    Check(runtime.events == std::vector<std::string>{
              "write:24", "read:24", "write:24",
              "write:40", "write:44", "read:40", "read:44",
              "write:40", "write:44"},
          "setter native read/write order");

    runtime.events.clear();
    state.words[1u] = 0u;
    Check(pal4::SequenceAnimProperties::SetPosition(
              state, {1.0f, 2.0f, 3.0f}, 0u,
              0x12345678u, runtime) == 0x12345600u &&
              runtime.events.empty(), "inactive position setter gate");
    state.words[1u] = 1u;
    Check(pal4::SequenceAnimProperties::SetPosition(
              state, {1.0f, 2.0f, 3.0f}, 0u,
              0x12345678u, runtime) == 0x89abcdefu &&
              runtime.words[0x301cu] == std::bit_cast<std::uint32_t>(1.0f) &&
              runtime.words[0x3020u] == std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words[0x3024u] == std::bit_cast<std::uint32_t>(3.0f),
          "absolute position setter and transform result");
    Check(pal4::SequenceAnimProperties::SetPosition(
              state, {4.0f, 5.0f, 6.0f}, 1u,
              0x12345678u, runtime) == 0x89abcdefu &&
              runtime.words[0x301cu] == std::bit_cast<std::uint32_t>(5.0f) &&
              runtime.words[0x3020u] == std::bit_cast<std::uint32_t>(7.0f) &&
              runtime.words[0x3024u] == std::bit_cast<std::uint32_t>(9.0f),
          "relative position setter adds all components");
    Check(runtime.events == std::vector<std::string>{
              "write:28", "write:32", "write:36", "transform",
              "read:28", "read:32", "read:36",
              "write:28", "write:32", "write:36", "transform"},
          "position setter read, write and transform ordering");
}

void TestTaskFacets() {
    pal4::SequenceAnimObjectState state{};
    state.words[30u] = 0x3000u;
    Runtime runtime;
    std::array<std::uint8_t, 4> channels{1u, 2u, 3u, 4u};
    Check(pal4::SequenceAnimProperties::SetPositionWords(
              state, {11u, 12u, 13u}, runtime) == 0u &&
              pal4::SequenceAnimProperties::GetColor(
                  state, channels, runtime) == 0u &&
              pal4::SequenceAnimProperties::SetColor(
                  state, {5u, 6u, 7u, 8u}, runtime) == 0u &&
              channels == std::array<std::uint8_t, 4>{1u, 2u, 3u, 4u} &&
              runtime.events.empty(),
          "inactive raw position and color facets");
    state.words[1u] = 1u;
    Check(pal4::SequenceAnimProperties::SetPositionWords(
              state, {0xaaaa0001u, 0xbbbb0002u, 0xcccc0003u}, runtime) ==
              0xefu && runtime.words[0x301cu] == 0xaaaa0001u &&
              runtime.words[0x3020u] == 0xbbbb0002u &&
              runtime.words[0x3024u] == 0xcccc0003u,
          "raw position words and transform low-byte result");
    runtime.color = 0x78563412u;
    Check(pal4::SequenceAnimProperties::GetColor(
              state, channels, runtime) == 0x78u &&
              channels == std::array<std::uint8_t, 4>{
                  0x12u, 0x34u, 0x56u, 0x78u},
          "RGBA getter extracts native byte order");
    Check(pal4::SequenceAnimProperties::SetColor(
              state, {0x11u, 0x22u, 0x33u, 0x44u}, runtime) == 0x6du &&
              runtime.color == 0x44332211u,
          "RGBA setter packs little-endian channels");
    Check(runtime.events == std::vector<std::string>{
              "write:28", "write:32", "write:36", "transform",
              "geometry", "color-read", "geometry", "color-write"},
          "raw position and color native call order");
}

}  // namespace

int main() {
    TestGetters();
    TestSetters();
    TestTaskFacets();
}
