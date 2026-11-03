#include "pal4/game_arena_object_list.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using List = pal4::GameArenaObjectList;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string Event(char action, std::uint32_t address) {
    return std::string(1, action) + std::to_string(address);
}

bool SameIgnoreCase(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) return false;
    }
    return true;
}

struct Runtime final : pal4::IGameArenaObjectListRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5108u, 0x8000u}, {0x8034u, 0u}, {0x5194u, 0xa000u},
        {0x5008u, 0x9000u}, {0x500cu, 0x9010u}};
    std::map<std::uint32_t, std::string> cstrings{
        {0x9000u, "World"}, {0x9010u, "Area"}};
    std::map<std::uint32_t, std::string> slotNames;
    std::vector<std::string> events;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    bool changeOnRelease = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back(Event('R', address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back(Event('W', address));
        writes.emplace_back(address, value);
        words[address] = value;
    }
    std::string ReadCString(std::uint32_t address) override {
        events.push_back(Event('T', address));
        return cstrings[address];
    }
    bool TextEqualsIgnoreCase(std::uint32_t address,
                              std::string_view text) override {
        events.push_back(Event('C', address));
        return SameIgnoreCase(slotNames[address], text);
    }
    void InitializeText(std::uint32_t address,
                        std::string_view text) override {
        events.push_back(Event('I', address));
        slotNames[address] = text;
    }
    void ReleaseObjectChildren(std::uint32_t object) override {
        events.push_back(Event('L', object));
        if (changeOnRelease) words[0x521cu] = 0xb010u;
    }
    void DestroyObject(std::uint32_t object) override {
        events.push_back(Event('D', object));
    }
    std::uint32_t FreeObject(std::uint32_t object) override {
        events.push_back(Event('F', object));
        return object + 1u;
    }
};

void Find() {
    Runtime runtime;
    runtime.words[0x5108u] = 0u;
    Check(List::FindByName(0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{Event('R', 0x5108u)},
          "find stops when world is absent");
    runtime = Runtime{};
    runtime.words[0x8034u] = 1u;
    Check(List::FindByName(0x5000u, runtime) == 0u &&
              runtime.events == (std::vector<std::string>{
                  Event('R', 0x5108u), Event('R', 0x8034u)}),
          "find stops in world mode one");

    runtime = Runtime{};
    runtime.words[0x5238u] = 1u;
    runtime.words[0x525cu] = 1u;
    runtime.words[0x5264u] = 0xb000u;
    runtime.slotNames[0x5220u] = "other";
    runtime.slotNames[0x5244u] = "wOrLd";
    runtime.slotNames[0x5250u] = "aReA";
    Check(List::FindByName(0x5000u, runtime) == 0xb000u &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x5260u, 1u}}),
          "find matches both names ignoring case and marks only selected slot");
    Check(runtime.events == (std::vector<std::string>{
              Event('R', 0x5108u), Event('R', 0x8034u),
              Event('R', 0x5214u), Event('R', 0x5238u),
              Event('R', 0x5008u), Event('T', 0x9000u),
              Event('C', 0x5220u), Event('R', 0x525cu),
              Event('R', 0x5008u), Event('T', 0x9000u),
              Event('C', 0x5244u), Event('R', 0x500cu),
              Event('T', 0x9010u), Event('C', 0x5250u),
              Event('W', 0x5260u), Event('R', 0x5264u)}),
          "find reads two names only for active candidates and returns after flag");

    runtime = Runtime{};
    runtime.words[0x5214u] = 1u;
    runtime.slotNames[0x51fcu] = "WORLD";
    runtime.slotNames[0x5208u] = "wrong";
    Check(List::FindByName(0x5000u, runtime) == 0u &&
              runtime.writes.empty(),
          "second-name mismatch does not mark a slot");
}

void Add() {
    Runtime runtime;
    runtime.words[0x5194u] = 0u;
    List::AddPending(0x5000u, runtime);
    Check(runtime.events == std::vector<std::string>{Event('R', 0x5194u)},
          "add stops before world read with no pending object");

    runtime = Runtime{};
    runtime.words[0x5214u] = 1u;
    runtime.words[0x521cu] = 0xb000u;
    runtime.words[0x5264u] = 0xa000u;
    List::AddPending(0x5000u, runtime);
    Check(runtime.words[0x5238u] == 1u &&
              runtime.words[0x5240u] == 0xa000u &&
              runtime.words[0x5194u] == 0u &&
              runtime.slotNames[0x5220u] == "World" &&
              runtime.slotNames[0x522cu] == "Area" &&
              runtime.words[0x523cu] == 0u,
          "first empty slot takes pending object before later duplicate");
    Check(runtime.events == (std::vector<std::string>{
              Event('R', 0x5194u), Event('R', 0x5108u),
              Event('R', 0x8034u), Event('R', 0x5214u),
              Event('R', 0x521cu), Event('R', 0x5238u),
              Event('R', 0x523cu), Event('R', 0x5008u),
              Event('T', 0x9000u), Event('I', 0x5220u),
              Event('R', 0x500cu), Event('T', 0x9010u),
              Event('I', 0x522cu), Event('W', 0x5238u),
              Event('R', 0x5194u), Event('W', 0x5240u),
              Event('W', 0x5194u)}),
          "empty-slot insertion keeps exact read and write order");

    runtime = Runtime{};
    runtime.words[0x5214u] = 1u;
    runtime.words[0x521cu] = 0xa000u;
    List::AddPending(0x5000u, runtime);
    Check(runtime.words[0x5194u] == 0u &&
              runtime.events.back() == Event('W', 0x5194u) &&
              runtime.slotNames.empty(),
          "duplicate pointer clears pending without insertion");

    runtime = Runtime{};
    runtime.words[0x5214u] = runtime.words[0x5238u] =
        runtime.words[0x525cu] = 1u;
    runtime.words[0x521cu] = 0xb000u;
    runtime.words[0x5240u] = 0xb020u;
    runtime.words[0x5264u] = 0xb030u;
    runtime.words[0x5218u] = 1u;
    runtime.changeOnRelease = true;
    List::AddPending(0x5000u, runtime);
    const auto release = std::find(runtime.events.begin(), runtime.events.end(),
                                   Event('L', 0xb000u));
    Check(release != runtime.events.end() &&
              std::vector<std::string>(release, release + 4) ==
                  (std::vector<std::string>{
                      Event('L', 0xb000u), Event('R', 0x521cu),
                      Event('D', 0xb010u), Event('F', 0xb010u)}) &&
              runtime.words[0x521cu] == 0xa000u &&
              runtime.words[0x5218u] == 0u &&
              runtime.words[0x5194u] == 0u,
          "full list releases old children, rereads pointer, then replaces first slot");
}

void InitializeAndCleanup() {
    Runtime runtime;
    for (std::uint32_t offset = 0x214u; offset <= 0x264u; offset += 36u) {
        runtime.words[0x5000u + offset] = 7u;
        runtime.words[0x5000u + offset + 4u] = 8u;
        runtime.words[0x5000u + offset + 8u] = 9u;
    }
    Check(List::Initialize(0x5000u, runtime) == 0x5284u &&
              runtime.writes.size() == 9u,
          "initialize clears three 36-byte slots and returns one-past cursor");
    for (const auto& [address, value] : runtime.writes)
        Check(value == 0u && address >= 0x5214u && address <= 0x5264u,
              "initialize writes only slot status, flag and pointer");

    runtime = Runtime{};
    runtime.words[0x5214u] = 1u;
    runtime.words[0x521cu] = 0xb000u;
    runtime.words[0x5238u] = 1u;
    runtime.words[0x5240u] = 0u;
    runtime.words[0x5264u] = 0xb020u;
    Check(List::Cleanup(0x5000u, runtime) == 0xb001u &&
              runtime.events == (std::vector<std::string>{
                  Event('R', 0x5214u), Event('R', 0x521cu),
                  Event('D', 0xb000u), Event('F', 0xb000u),
                  Event('W', 0x521cu), Event('W', 0x5214u),
                  Event('W', 0x5218u), Event('R', 0x5238u),
                  Event('R', 0x5240u), Event('R', 0x525cu)}),
          "cleanup destroys only active non-null records in slot order");
    Check(runtime.words[0x5238u] == 1u &&
              runtime.words[0x5264u] == 0xb020u,
          "null active and inactive populated slots remain unchanged");

    pal4::GameArenaObjectListRuntimeStub stub;
    Check(List::FindByName(0u, stub) == 0u &&
              List::Initialize(0u, stub) == 0x284u &&
              List::Cleanup(0u, stub) == 0u,
          "native object-list stub links");
    List::AddPending(0u, stub);
}

}  // namespace

int main() {
    try {
        Find();
        Add();
        InitializeAndCleanup();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
