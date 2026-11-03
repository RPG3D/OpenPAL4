#include "pal4/collection_world_initializer.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::CollectionWorldRuntimeStub {
    std::vector<char> events;
    int comparison{};
    bool allocated{};
    std::uint32_t clump{};
    std::string copied;

    void PrepareGeometry(pal4::CollectionState&,
                         std::size_t offset) override {
        Check(offset == 0xdcu,
              "plugin preparation uses world +0xc4 / collection +0xdc");
        events.push_back('G');
    }
    int CompareName(pal4::CollectionWorldState& world,
                    std::string_view name) override {
        Check(name == "scene01" && world.field_dc == 3u,
              "comparison receives input name and stored length");
        events.push_back('C');
        return comparison;
    }
    bool AllocateName(pal4::CollectionWorldState&,
                       std::size_t length, std::uint8_t mode) override {
        Check(length == 7u && mode == 1u,
              "name buffer receives length and allocation mode one");
        events.push_back('A');
        return allocated;
    }
    void CopyName(pal4::CollectionWorldState& world,
                  std::string_view name) override {
        Check(world.field_dc == 3u && name == "scene01",
              "copy precedes stored-length update");
        copied.assign(name);
        events.push_back('N');
    }
    void ProcessScripts() override { events.push_back('P'); }
    std::uint32_t GameInstance() override {
        events.push_back('I');
        return 0x1000u;
    }
    void InitializeWorld(pal4::CollectionState&,
                         std::uint32_t object, std::string_view label,
                         std::string_view name,
                         std::uint32_t owner) override {
        Check(object == 0x77u && label == "UIWorld" &&
                  name == "scene01" && owner == 0x11acu,
              "world init receives object, UIWorld label, name and owner");
        events.push_back('W');
    }
    void SetCamera(pal4::CollectionState&,
                   std::uint32_t camera) override {
        Check(camera == 0x8800u,
              "world camera receives caller's camera word");
        events.push_back('K');
    }
    void AddLight(pal4::CollectionState&,
                  const std::array<std::uint32_t, 4>& color) override {
        Check(color == std::array<std::uint32_t, 4>{
                  0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u},
              "default light receives four 1.0f words");
        events.push_back('L');
    }
    std::uint32_t WorldClump(pal4::CollectionState&) override {
        events.push_back('Q');
        return clump;
    }
    void AddClump(pal4::CollectionState&,
                  std::uint32_t value) override {
        Check(value == clump && value != 0u,
              "world add receives stored nonzero clump");
        events.push_back('D');
    }
    void ProcessClumpAtomics(pal4::CollectionState&,
                             std::uint32_t value,
                             std::size_t offset) override {
        Check(value == clump && offset == 0xdcu,
              "clump plugin receives same clump and geometry member");
        events.push_back('T');
    }
    int CleanupScripts() override {
        events.push_back('X');
        return -19;
    }
};

void CheckEqualName() {
    pal4::CollectionState state{};
    state.world.field_dc = 3u;
    Runtime runtime;
    Check(pal4::CollectionWorldInitializer::AddElementWithDetails(
              state, 0x77u, "scene01", 0x8800u, runtime) == 0 &&
              runtime.events == std::vector<char>{'G', 'C'} &&
              state.world.field_dc == 3u,
          "equal name stops after unconditional geometry preparation");
}

void CheckChangedName() {
    pal4::CollectionState state{};
    state.world.field_dc = 3u;
    Runtime runtime;
    runtime.comparison = -1;
    runtime.allocated = true;
    runtime.clump = 0x9900u;
    Check(pal4::CollectionWorldInitializer::AddElementWithDetails(
              state, 0x77u, "scene01", 0x8800u, runtime) == -19 &&
              runtime.events == std::vector<char>{
                  'G', 'C', 'A', 'N', 'P', 'I', 'W', 'K',
                  'L', 'Q', 'D', 'T', 'X'} &&
              runtime.copied == "scene01" && state.world.field_dc == 7u,
          "changed name copies bytes then initializes world and clump");
}

void CheckAllocationFailureAndNoClump() {
    pal4::CollectionState state{};
    state.world.field_dc = 3u;
    Runtime runtime;
    runtime.comparison = 1;
    runtime.allocated = false;
    Check(pal4::CollectionWorldInitializer::AddElementWithDetails(
              state, 0x77u, "scene01", 0x8800u, runtime) == -19 &&
              runtime.events == std::vector<char>{
                  'G', 'C', 'A', 'P', 'I', 'W', 'K', 'L', 'Q', 'X'} &&
              state.world.field_dc == 3u && runtime.copied.empty(),
          "allocation failure retains old name but still initializes world");
}

}  // namespace

int main() {
    CheckEqualName();
    CheckChangedName();
    CheckAllocationFailureAndNoClump();
    std::cout << "collection world initializer tests passed\n";
}
