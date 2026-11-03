#include "pal4/minimap_object_collection.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapObjectActiveRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words{
        {0x6000u, 0x4000u}, {0x6014u, 0x4004u},
        {0x6008u, 0x7000u}, {0x600cu, 0x7008u},
        {0x601cu, 0u}, {0x6020u, 0u},
        {0x7000u, 0x8000u}, {0x7004u, 0x8100u},
        {0x8008u, 0xa000u}, {0x800cu, 4u},
        {0x8108u, 0xa100u}, {0x810cu, 3u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {0xa000u, 'H'}, {0xa001u, 'o'}, {0xa002u, 'm'},
        {0xa003u, 'e'}, {0xa004u, 0u},
        {0xa100u, 'I'}, {0xa101u, 'n'}, {0xa102u, 'n'},
        {0xa103u, 0u}, {0x8044u, 1u}, {0x8144u, 0u}};
    std::vector<std::string> events;

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.emplace_back("write:" + std::to_string(address) +
                            ":" + std::to_string(value));
        bytes[address] = value;
    }
};

pal4::MinimapObjectCollectionState State() {
    pal4::MinimapObjectCollectionState state;
    state.WriteDword(4u, 0x6000u);
    state.WriteDword(8u, 0x6028u);
    state.WriteDword(16u, 0x8000u);
    return state;
}

void TestFindGroup() {
    Runtime runtime;
    pal4::MinimapObjectCollectionState empty;
    Check(pal4::MinimapObjectCollection::FindGroupById(
              empty, 0x4000u, runtime) == 0u &&
              runtime.events.empty(),
          "null group range returns before memory access");
    auto state = State();
    Check(pal4::MinimapObjectCollection::FindGroupById(
              state, 0x4004u, runtime) == 0x6014u &&
              runtime.events == std::vector<std::string>{
                  "read:24576", "read:24596"},
          "ID lookup compares first word of each 20-byte group");
    runtime.events.clear();
    Check(pal4::MinimapObjectCollection::FindGroupById(
              state, 0x9999u, runtime) == 0u &&
              runtime.events.size() == 2u,
          "missing ID returns zero after scanning all groups");
}

void TestFindIconByName() {
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::FindIconByName(
              0x6004u, "Inn", runtime) == 0x8100u &&
              runtime.events == std::vector<std::string>{
                  "read:24584", "read:24588", "read:28672",
                  "read:32776", "read:32780", "read:28676",
                  "read:33032", "read:33036", "byte:41216",
                  "byte:41217", "byte:41218", "byte:41219"},
          "name search compares stored length plus terminating NUL");
    runtime.events.clear();
    Check(pal4::MinimapObjectCollection::FindIconByName(
              0x6004u, "In", runtime) == 0u,
          "short name does not match stored icon name");
    runtime.events.clear();
    Check(pal4::MinimapObjectCollection::FindIconByName(
              0x6018u, "Inn", runtime) == 0u &&
              runtime.events.size() == 1u,
          "null nested pointer array returns immediately");
    runtime.bytes[0xa103u] = 'x';
    Check(pal4::MinimapObjectCollection::FindIconByName(
              0x6004u, "Inn", runtime) == 0u,
          "nonzero terminator is rejected");
}

void TestActiveSelection() {
    Runtime runtime;
    auto state = State();
    Check(pal4::MinimapObjectCollection::SetActiveIcon(
              state, "Inn", 0x4000u, runtime) &&
              state.ReadDword(16u) == 0x8100u &&
              runtime.bytes[0x8044u] == 0u &&
              runtime.bytes[0x8144u] == 1u,
          "selection clears old marker and sets new marker");
    Runtime missing_name;
    auto state2 = State();
    Check(!pal4::MinimapObjectCollection::SetActiveIcon(
              state2, "Castle", 0x4000u, missing_name) &&
              state2.ReadDword(16u) == 0u &&
              missing_name.bytes[0x8044u] == 0u,
          "missing name clears old active icon and stored pointer");
    Runtime missing_group;
    auto state3 = State();
    Check(!pal4::MinimapObjectCollection::SetActiveIcon(
              state3, "Inn", 0x9999u, missing_group) &&
              state3.ReadDword(16u) == 0x8000u &&
              missing_group.bytes[0x8044u] == 1u,
          "missing group leaves previous selection unchanged");
}

}  // namespace

int main() {
    TestFindGroup();
    TestFindIconByName();
    TestActiveSelection();
    return EXIT_SUCCESS;
}
