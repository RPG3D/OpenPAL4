#include "pal4/ui_placement_rule.h"

#include "pal4/ui_property_grid.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiPropertyGridRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t) override { return 0x7000; }
    void Free(std::uint32_t) override {}
    void InvokeCallback(std::uint32_t, pal4::UiPropertyGrid&) override {}
    std::uint32_t LookupFont(const char*) override { return 0; }
    std::string LookupGameTextUtf8(std::uint32_t) override { return {}; }
    std::uint32_t SetCodepointText(std::uint32_t) override { return 0; }
    std::uint32_t LookupImage(const char*, const char*) override { return 0; }
    std::array<float, 2> ImageDimensions(std::uint32_t) override {
        return {0.0f, 0.0f};
    }
    std::uint32_t TemporaryElementAddress() override { return 0; }
    std::uint8_t UnwrittenSequenceMarker() override { return 0; }
    bool CheckPlacementViaAnchor(
        std::uint32_t, pal4::UiPropertyGrid&, std::uint32_t,
        const std::array<std::int32_t, 2>&) override {
        return false;
    }
    std::uint32_t GetAnchorProperty(
        std::uint32_t, pal4::UiPropertyGrid&) override {
        return 0;
    }
    void DestroyElementString(std::uint32_t) override {}
    void DestroyTemporaryElement(std::uint32_t) override {}
};

void TestConstructionAndPropertyMatch() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    grid.Grid().Resize(4, 4);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x1000, 7, {1, 1}, {1, 1}, {11, 12, 13}, runtime));
    grid.ConfigureRender(0x1000, 0x10c8, {0.0f, 0.0f}, {1.0f, 1.0f},
                         0, 7, false);
    pal4::UiPlacementRule rule;
    Check(rule.Property() == 0 &&
              rule.First() == std::array<std::int32_t, 2>{0, 0} &&
              rule.Second() == std::array<std::int32_t, 2>{0, 0},
          "constructor zeros rule fields");
    rule.Configure(7, {1, 1}, {2, 2});
    Check(pal4::UiPlacementRule::BasicCanOccupy(
              {1, 1}, 0x1000, grid.Grid(), runtime),
          "basic vtable thunk forwards grid occupancy");
    Check(rule.Property() == 7 &&
              rule.First() == std::array<std::int32_t, 2>{1, 1} &&
              rule.Second() == std::array<std::int32_t, 2>{2, 2},
          "rule configuration");
    Check(!rule.MatchesGridProperties(grid, runtime),
          "neither grid cell holds selected property");
    static_cast<void>(grid.Grid().Set(1, 1, 7));
    Check(rule.MatchesGridProperties(grid, runtime),
          "first grid cell holds selected property");
    static_cast<void>(grid.Grid().Set(1, 1, 0));
    static_cast<void>(grid.Grid().Set(2, 2, 7));
    Check(rule.MatchesGridProperties(grid, runtime),
          "second grid cell holds selected property");
    rule.Configure(8, {1, 1}, {2, 2});
    Check(!rule.MatchesGridProperties(grid, runtime),
          "missing selected element rejects rule");
}

void TestOccupancyFallback() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    grid.Grid().Resize(4, 4);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x1000, 7, {1, 1}, {1, 1}, {11, 12, 13}, runtime));
    pal4::UiPlacementRule rule;
    rule.Configure(7, {0, 0}, {0, 0});
    Check(rule.CanOccupyOrOverlap({1, 1}, 0x1000, grid, runtime),
          "ordinary empty cell succeeds before fallback");
    static_cast<void>(grid.Grid().Set(1, 1, -1));
    Check(rule.CanOccupyOrOverlap({1, 1}, 0x1000, grid, runtime),
          "fallback permits special minus-one cell");
    static_cast<void>(grid.Grid().Set(1, 1, 9));
    Check(!rule.CanOccupyOrOverlap({1, 1}, 0x1000, grid, runtime),
          "fallback rejects foreign property");
    static_cast<void>(grid.Grid().Set(1, 1, -1));
    rule.Configure(8, {0, 0}, {0, 0});
    Check(!rule.CanOccupyOrOverlap({1, 1}, 0x1000, grid, runtime),
          "fallback requires matching rule property");
    rule.Configure(7, {0, 0}, {0, 0});
    Check(!rule.CanOccupyOrOverlap({3, 3}, 0x1000, grid, runtime),
          "fallback requires distance at most one");
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x1000, 7, {0, 1}, {1, 1}, {11, 12, 13}, runtime));
    Check(!rule.CanOccupyOrOverlap({-1, 1}, 0x1000, grid, runtime),
          "out-of-range cell does not pass fallback");
}

void TestImageLinks() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    grid.Grid().Resize(4, 4);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x1000, 7, {0, 0}, {1, 1}, {101, 0, 0}, runtime));
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x10c8, 8, {1, 1}, {1, 1}, {101, 0, 0}, runtime));
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x1190, 9, {2, 2}, {1, 1}, {202, 0, 0}, runtime));
    grid.ConfigureRender(0x1000, 0x1258, {0.0f, 0.0f}, {1.0f, 1.0f},
                         0, 7, false);
    pal4::UiPlacementSequenceRule rule;
    Check(rule.MatchesImageLinks(grid, runtime), "empty link list matches");
    rule.AddLink({7, 2, 2});
    Check(rule.Links().size() == 1 &&
              !rule.MatchesImageLinks(grid, runtime),
          "empty grid cell has no linked element");
    static_cast<void>(grid.Grid().Set(2, 2, 8));
    Check(rule.MatchesImageLinks(grid, runtime),
          "linked property shares first image");
    rule.AddLink({8, 1, 1});
    static_cast<void>(grid.Grid().Set(1, 1, 7));
    Check(rule.MatchesImageLinks(grid, runtime),
          "all triples must match");
    runtime.words[0x10c8 + 168u] = 303;
    Check(!rule.MatchesImageLinks(grid, runtime),
          "different first image rejects sequence");
    pal4::UiPlacementSequenceRule missing;
    missing.AddLink({99, 1, 1});
    Check(!missing.MatchesImageLinks(grid, runtime),
          "missing source property rejects sequence");
}

}  // namespace

int main() {
    try {
        TestConstructionAndPropertyMatch();
        TestOccupancyFallback();
        TestImageLinks();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
