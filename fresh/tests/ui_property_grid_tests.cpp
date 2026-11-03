#include "pal4/ui_property_grid.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiPropertyGridRuntime {
    std::uint32_t allocation = 0x7000;
    std::uint32_t next_element_allocation = 0xb000;
    std::uint32_t requested_size = 0;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::uint32_t> calls;
    std::vector<std::uint32_t> freed;
    pal4::UiPropertyGrid* called_grid = nullptr;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> image_names;
    std::vector<std::string> events;
    std::vector<std::uint32_t> codepoints;
    std::vector<std::uint32_t> game_text_ids;
    std::vector<std::string> font_names;
    std::array<std::uint32_t, 2> checked_origin{};
    std::uint32_t checked_element = 0;
    std::vector<std::uint32_t> destroyed_strings;
    std::uint32_t checked_anchor = 0;
    std::uint32_t anchor_property = 0x12345678;
    std::uint32_t destroyed_element = 0;
    std::uint32_t text_source = 0;
    bool can_place = true;
    bool fail_sequence_anchor = false;
    bool fail_record_allocation = false;
    int destroy_count = 0;
    std::vector<std::uint32_t> drawn_images;
    std::vector<pal4::UiFloatRect> drawn_rects;
    std::array<float, 2> image_dimensions{10.0f, 20.0f};

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        requested_size = bytes;
        allocation_sizes.push_back(bytes);
        if (bytes == 4) return allocation;
        if ((bytes == 20 && fail_sequence_anchor) ||
            (bytes == 12 && fail_record_allocation)) return 0;
        const auto address = next_element_allocation;
        next_element_allocation += 0x1000;
        return address;
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t AssignCeguiString(const std::uint32_t destination,
                                    const std::uint32_t source,
                                    const std::uint32_t start,
                                    const std::uint32_t length) override {
        Check(start == 0 && length == 0xffffffffu,
              "CEGUI text assignment arguments");
        if (destination == 0x8010) {
            text_source = source;
            if (source == 0x900000) words[destination] = words.at(source);
            events.emplace_back("text");
        } else {
            words[destination] = words.at(source);
            events.emplace_back("assign");
        }
        return destination;
    }
    void CopyConstructCeguiString(const std::uint32_t destination,
                                  const std::uint32_t source) override {
        words[destination] = words.at(source);
        events.emplace_back("copy");
    }
    void InvokeCallback(const std::uint32_t address,
                        pal4::UiPropertyGrid& grid) override {
        calls.push_back(address);
        called_grid = &grid;
    }
    std::uint32_t LookupFont(const char* name) override {
        font_names.emplace_back(name);
        return 0x7777;
    }
    std::string LookupGameTextUtf8(std::uint32_t id) override {
        game_text_ids.push_back(id);
        return "Puzzle title";
    }
    std::uint32_t SetCodepointText(std::uint32_t codepoint) override {
        codepoints.push_back(codepoint);
        words[0x900000] = codepoint;
        return 0x900000;
    }
    std::uint32_t LookupImage(const char* imageset,
                              const char* image) override {
        Check(std::string(imageset) == "youshang0", "imageset name");
        image_names.emplace_back(image);
        events.emplace_back("image");
        return 0x9000 + static_cast<std::uint32_t>(image_names.size());
    }
    std::array<float, 2> ImageDimensions(std::uint32_t) override {
        return image_dimensions;
    }
    std::uint32_t TemporaryElementAddress() override { return 0x8000; }
    std::uint8_t UnwrittenSequenceMarker() override { return 0xa5; }
    bool CheckPlacementViaAnchor(
        const std::uint32_t anchor, pal4::UiPropertyGrid& grid,
        const std::uint32_t element,
        const std::array<std::int32_t, 2>& origin) override {
        Check(&grid == called_grid || called_grid == nullptr,
              "placement object");
        checked_anchor = anchor;
        checked_element = element;
        checked_origin = {static_cast<std::uint32_t>(origin[0]),
                          static_cast<std::uint32_t>(origin[1])};
        events.emplace_back("check");
        return can_place;
    }
    std::uint32_t GetAnchorProperty(
        const std::uint32_t anchor,
        pal4::UiPropertyGrid&) override {
        checked_anchor = anchor;
        return anchor_property;
    }
    void DestroyElementString(const std::uint32_t element) override {
        destroyed_strings.push_back(element);
        events.emplace_back("destroy string");
    }
    void DestroyTemporaryElement(const std::uint32_t element) override {
        destroyed_element = element;
        ++destroy_count;
        events.emplace_back("destroy");
    }
    void DrawImage(const std::uint32_t image,
                   const pal4::UiFloatRect& rect,
                   float) override {
        drawn_images.push_back(image);
        drawn_rects.push_back(rect);
    }
};

constexpr std::array<std::uint32_t, pal4::UiPropertyGrid::kCallbackCount>
    kCallbacks = {
        0x5ad160, 0x5ad660, 0x5adb60, 0x5ae060, 0x5ae600, 0x5aeae0,
        0x5af090, 0x5af520, 0x5afad0, 0x5affc0, 0x5b0670, 0x5b0cc0,
        0x5b1390, 0x5b1a60, 0x5b2240, 0x5b2bc0, 0x5b3360, 0x5b3b50,
};

void TestConstruction() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0xa5, 0x5a, runtime);
    Check(grid.Grid().InitialMarker() == 0xa5 &&
              grid.Grid().RowCount() == 0 &&
              grid.Grid().ColumnCount() == 0,
          "base grid construction");
    Check(grid.Byte(0x18) == 0x5a && grid.Word(0x3c) == 0x7000,
          "derived marker and owned anchor");
    Check(runtime.requested_size == 4 && runtime.writes.size() == 1 &&
              runtime.writes[0] == std::pair(0x7000u, 0x8493f0u),
          "four-byte vtable anchor allocation");
    for (std::size_t offset = 0x1c; offset <= 0x38; offset += 4) {
        Check(grid.Word(offset) == 0, "element vector and layout fields");
    }
    for (std::size_t offset = 0x40; offset <= 0x48; offset += 4) {
        Check(grid.Word(offset) == 0, "selection and state words");
    }
    Check(grid.Byte(0x4c) == 0 && grid.Word(0x98) == 0 &&
              grid.Word(0x9c) == 32, "flag and string capacity");
    for (std::size_t offset = 0xa0; offset <= 0xac; offset += 4) {
        Check(grid.Word(offset) == 0, "string trailing words");
    }
    Check(grid.Word(0x138) == 0 && grid.Word(0x13c) == 0 &&
              grid.Byte(0x140) == 0 && grid.Word(0x14c) == 0 &&
              grid.Word(0x150) == 0 && grid.Byte(0x154) == 0,
          "trailing UI state");
    for (std::size_t index = 0; index < kCallbacks.size(); ++index) {
        Check(grid.Word(0x50 + index * 4) == kCallbacks[index],
              "callback address and order");
    }
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.allocation = 0;
    pal4::UiPropertyGrid grid(1, 2, runtime);
    Check(runtime.requested_size == 4 && runtime.writes.empty() &&
              grid.Word(0x3c) == 0, "failed anchor allocation");
}

void TestDispatch() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    for (std::size_t i = 0; i < kCallbacks.size(); ++i) {
        grid.SetCallback(i, 0x123456u + static_cast<std::uint32_t>(i));
    }
    for (std::size_t index = 0; index < kCallbacks.size(); ++index) {
        grid.SetDispatchState(123);
        Check(grid.Dispatch(static_cast<std::int32_t>(index), runtime),
              "valid callback index");
        Check(grid.Word(0x40) == 0 && runtime.calls.back() ==
                  0x123456u + static_cast<std::uint32_t>(index) &&
                  runtime.called_grid == &grid,
              "state cleared before dispatch and object forwarded");
    }
    Check(runtime.calls.size() == kCallbacks.size(), "all callbacks invoked");
    grid.SetCallback(5, 0);
    grid.SetDispatchState(44);
    Check(grid.Dispatch(5, runtime) && grid.Word(0x40) == 0 &&
              runtime.calls.size() == kCallbacks.size(),
          "null callback succeeds without invocation");
    for (const auto index : {-1, 18, 100}) {
        grid.SetDispatchState(55);
        Check(!grid.Dispatch(index, runtime) && grid.Word(0x40) == 0 &&
                  runtime.calls.size() == kCallbacks.size(),
              "invalid index clears state and rejects callback");
    }
    bool rejected = false;
    try {
        grid.SetCallback(18, 1);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    Check(rejected, "callback table bounds");
}

void TestFirstPuzzleInitialization() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    Check(grid.Dispatch(0, runtime), "first puzzle dispatch");
    Check(runtime.calls.empty() && grid.Grid().RowCount() == 4 &&
              grid.Grid().ColumnCount() == 6 &&
              grid.Word(0x28) == 0 && grid.Word(0x2c) == 0 &&
              grid.Word(0x30) == 0x42860000 &&
              grid.Word(0x34) == 0x42880000 &&
              grid.Word(0x38) == 0x7777 && grid.Word(0x48) == 60,
          "first puzzle geometry, scale, font and budget");
    for (int x = 0; x < 4; ++x) {
        Check(grid.Grid().Get(x, 5) == (x == 0 || x == 3 ? -2 : -1),
              "first puzzle tail markers");
    }
    Check(runtime.font_names == std::vector<std::string>{"Puzzle"} &&
              runtime.game_text_ids == std::vector<std::uint32_t>{0xea} &&
              grid.TitleUtf8() == "Puzzle title" &&
              runtime.codepoints == std::vector<std::uint32_t>{
                  0x5fc3, 0x6000, 21696, 24863, 30171, 31070, 24754},
          "font, database title and element characters");
    Check(grid.ElementCount() == 7 && grid.Word(0x44) == 1 &&
              grid.Word(0x40) == 0 && grid.Byte(0x4c) == 0 &&
              runtime.destroy_count == 7,
          "seven elements and final selection state");
    constexpr std::array<std::array<std::int32_t, 4>, 7> geometry = {{
        {{0, 0, 1, 1}}, {{0, 1, 1, 1}}, {{0, 3, 2, 1}},
        {{0, 2, 2, 1}}, {{2, 2, 1, 1}}, {{3, 2, 1, 1}},
        {{1, 0, 2, 2}},
    }};
    const auto begin = grid.Word(0x1c);
    for (std::size_t i = 0; i < geometry.size(); ++i) {
        const auto element = begin + static_cast<std::uint32_t>(i * 200);
        for (std::size_t word = 0; word < 4; ++word) {
            Check(runtime.words.at(element + static_cast<std::uint32_t>(word * 4)) ==
                      static_cast<std::uint32_t>(geometry[i][word]),
                  "first puzzle element geometry");
        }
        Check(runtime.words.at(element + 192) == i + 1 &&
                  runtime.words.at(element + 16) ==
                      runtime.codepoints[i],
              "first puzzle property and copied character");
    }
    const auto anchor = grid.Word(0x3c);
    Check(anchor != 0x7000 &&
              std::find(runtime.freed.begin(), runtime.freed.end(), 0x7000) !=
                  runtime.freed.end() &&
              runtime.allocation_sizes.back() == 24 &&
              runtime.words.at(anchor) == 0x8493fc &&
              runtime.words.at(anchor + 4) == 7 &&
              runtime.words.at(anchor + 8) == 1 &&
              runtime.words.at(anchor + 12) == 5 &&
              runtime.words.at(anchor + 16) == 2 &&
              runtime.words.at(anchor + 20) == 5,
          "first puzzle replacement placement rule");

    const auto old_begin = grid.Word(0x1c);
    const auto old_count = runtime.destroyed_strings.size();
    grid.InitializeFirstPuzzle(runtime);
    Check(grid.ElementCount() == 7 &&
              runtime.destroyed_strings.size() >= old_count + 7 &&
              runtime.destroyed_strings[old_count] == old_begin &&
              runtime.freed.back() == anchor,
          "reinitialization destroys prior elements and anchor");
}

void TestAdditionalPuzzleInitializers() {
    using Spec = std::array<std::uint32_t, 6>;
    const std::vector<std::vector<Spec>> layouts = {
        {{{1,2,0,1,1,0x6cc9}, {2,0,1,1,2,0x6e56},
          {3,1,1,2,1,27827}, {4,3,1,1,1,28330},
          {5,2,2,2,1,27743}, {6,1,2,1,2,27901},
          {7,0,0,2,1,28023}}},
        {{{1,0,0,1,2,0x751f}, {2,3,0,1,2,0x517b},
          {3,1,1,1,2,21463}, {4,2,1,1,2,23492},
          {5,0,3,2,1,33268}, {6,2,3,2,1,38477},
          {7,1,0,2,1,20239}}},
        {{{1,1,0,2,1,0x7761}, {2,0,1,2,1,0x6655},
          {3,2,1,2,1,0x5012}, {4,0,2,1,1,30196},
          {5,1,2,2,1,27785}, {6,3,2,1,1,26790},
          {8,0,3,2,1,97}, {9,2,3,2,1,26127},
          {7,0,0,1,1,37257}}},
        {{{1,2,0,1,2,0x725b}, {2,0,2,2,2,32650},
          {3,2,2,2,1,20820}, {4,2,3,1,1,34382},
          {5,3,3,1,1,29482}, {7,0,0,2,2,39532}}},
        {{{1,1,0,1,1,0x82b1}, {2,1,1,1,1,0x9c7c},
          {3,1,2,2,1,0x8349}, {4,3,2,1,1,34411},
          {5,0,3,2,2,40857}, {6,2,3,1,1,27700},
          {8,3,3,1,1,21494}, {9,2,4,1,1,20113},
          {7,2,0,2,2,34430}}},
    };
    constexpr std::array<std::uint32_t, 5> budgets = {70, 70, 80, 90, 70};
    for (std::size_t layout = 0; layout < layouts.size(); ++layout) {
        Runtime runtime;
        pal4::UiPropertyGrid grid(0, 0, runtime);
        Check(grid.Dispatch(static_cast<std::int32_t>(layout + 1), runtime),
              "additional puzzle callback dispatch");
        Check(runtime.calls.empty() && grid.Word(0x48) == budgets[layout] &&
                  runtime.game_text_ids == std::vector<std::uint32_t>{
                      static_cast<std::uint32_t>(0xeb + layout)} &&
                  grid.Grid().RowCount() == 4 &&
                  grid.Grid().ColumnCount() == 6 &&
                  grid.ElementCount() == layouts[layout].size() &&
                  grid.Word(0x44) == 1,
              "puzzle metadata and element count");
        for (int x = 0; x < 4; ++x) {
            Check(grid.Grid().Get(x, 5) == (x == 0 || x == 3 ? -2 : -1),
                  "puzzle boundary markers");
        }
        const auto begin = grid.Word(0x1c);
        for (std::size_t i = 0; i < layouts[layout].size(); ++i) {
            const auto& spec = layouts[layout][i];
            const auto element = begin + static_cast<std::uint32_t>(i * 200);
            for (std::size_t field = 0; field < 4; ++field) {
                Check(runtime.words.at(element +
                      static_cast<std::uint32_t>(field * 4)) == spec[field + 1],
                      "puzzle element position and extent");
            }
            Check(runtime.words.at(element + 192) == spec[0] &&
                      runtime.words.at(element + 16) == spec[5] &&
                      runtime.codepoints[i] == spec[5],
                  "puzzle element identifier and character");
        }
        const auto anchor = grid.Word(0x3c);
        Check(runtime.codepoints.size() == layouts[layout].size() &&
                  runtime.words.at(anchor) == 0x8493fc &&
                  runtime.words.at(anchor + 4) == 7 &&
                  runtime.words.at(anchor + 8) == 1 &&
                  runtime.words.at(anchor + 12) == 5 &&
                  runtime.words.at(anchor + 16) == 2 &&
                  runtime.words.at(anchor + 20) == 5,
              "puzzle rule configuration");
    }
}

struct ExpectedImageCell {
    std::uint32_t x;
    std::uint32_t y;
    const char* image;
};

struct ExpectedImageLayout {
    std::uint32_t budget;
    std::array<std::array<std::uint32_t, 4>, 2> decorations;
    std::vector<ExpectedImageCell> cells;
    std::vector<std::array<std::uint32_t, 3>> links;
};

void TestImagePuzzleLayouts() {
    const std::array<ExpectedImageLayout, 12> layouts = {{
        {3, {{{1, 0, 0, 0}, {2, 3, 0, 0}}},
         {
             {0, 0, "d"}, {1, 1, "b"}, {1, 2, "h"}, {1, 3, "a"},
             {3, 2, "g"},
         },
         {{1, 1, 0}, {4, 2, 3}, {5, 2, 2}}},
        {3, {{{3, 0, 3, 1}, {2, 3, 0, 0}}},
         {
             {3, 0, "b"}, {1, 1, "d"}, {2, 1, "a"}, {3, 1, "e"},
             {1, 2, "b"}, {2, 2, "g"}, {3, 3, "h"},
         },
         {{3, 2, 0}, {4, 2, 1}, {7, 2, 3}}},
        {3, {{{1, 0, 0, 0}, {3, 3, 0, 1}}},
         {
             {1, 0, "h"}, {2, 1, "b"}, {2, 2, "d"}, {3, 2, "h"},
             {2, 3, "f"}, {3, 3, "b"},
         },
         {{3, 1, 1}, {5, 2, 2}}},
        {3, {{{0, 0, 0, 1}, {3, 3, 0, 0}}},
         {
             {0, 0, "g"}, {0, 1, "b"}, {1, 1, "d"}, {3, 1, "c"},
             {0, 2, "f"}, {2, 2, "h"}, {3, 2, "h"}, {1, 3, "b"},
             {2, 3, "a"},
         },
         {{3, 1, 0}, {6, 1, 2}, {7, 3, 3}}},
        {4, {{{3, 0, 0, 1}, {3, 3, 0, 0}}},
         {
             {3, 0, "a"}, {1, 1, "c"}, {2, 1, "e"}, {3, 1, "g"},
             {1, 2, "f"}, {2, 2, "b"}, {3, 2, "g"}, {2, 3, "e"},
         },
         {{3, 2, 0}, {2, 2, 1}, {6, 3, 2}, {7, 3, 3}}},
        {4, {{{0, 0, 0, 0}, {2, 3, 0, 0}}},
         {
             {0, 0, "d"}, {2, 0, "b"}, {3, 0, "e"}, {0, 1, "f"},
             {0, 2, "c"}, {1, 2, "h"}, {3, 2, "g"}, {1, 3, "d"},
             {2, 3, "a"},
         },
         {{2, 1, 0}, {6, 1, 1}, {5, 1, 2}, {7, 2, 2}}},
        {4, {{{1, 0, 0, 0}, {2, 3, 0, 0}}},
         {
             {1, 0, "e"}, {2, 0, "e"}, {0, 1, "a"}, {1, 1, "f"},
             {0, 2, "h"}, {1, 2, "b"}, {3, 2, "a"}, {1, 3, "b"},
             {2, 3, "g"},
         },
         {{3, 0, 0}, {5, 0, 1}, {6, 0, 2}, {4, 1, 2}}},
        {4, {{{3, 0, 3, 1}, {2, 3, 0, 0}}},
         {
             {0, 0, "e"}, {1, 0, "h"}, {2, 0, "g"}, {3, 0, "g"},
             {0, 1, "a"}, {3, 1, "f"}, {1, 2, "h"}, {0, 3, "f"},
             {1, 3, "f"}, {2, 3, "f"}, {3, 3, "h"},
         },
         {{1, 1, 0}, {2, 1, 1}, {10, 2, 2}, {11, 2, 3}}},
        {4, {{{1, 0, 0, 0}, {3, 3, 0, 1}}},
         {
             {0, 0, "g"}, {1, 0, "d"}, {3, 0, "b"}, {0, 1, "c"},
             {1, 1, "e"}, {3, 1, "c"}, {0, 2, "e"}, {1, 2, "a"},
             {2, 2, "e"}, {3, 2, "b"}, {0, 3, "f"}, {1, 3, "g"},
             {2, 3, "e"}, {3, 3, "h"},
         },
         {{1, 1, 0}, {2, 2, 0}, {9, 2, 1}, {8, 2, 2}}},
        {3, {{{3, 0, 0, 2}, {1, 3, 0, 2}}},
         {
             {2, 0, "j1"}, {3, 0, "i1"}, {0, 1, "b1"}, {1, 1, "d1"},
             {2, 1, "i1"}, {3, 1, "f1"}, {0, 2, "d1"}, {1, 2, "g1"},
             {0, 3, "c1"}, {1, 3, "j1"}, {3, 3, "i1"},
         },
         {{6, 2, 2}, {11, 2, 3}}},
        {4, {{{2, 0, 0, 2}, {2, 3, 0, 2}}},
         {
             {0, 0, "b1"}, {1, 0, "i1"}, {3, 0, "h1"}, {0, 1, "d1"},
             {2, 1, "e1"}, {3, 1, "h1"}, {0, 2, "c1"}, {1, 2, "j1"},
             {3, 2, "a1"}, {2, 3, "f1"}, {3, 3, "a1"},
         },
         {{2, 1, 1}, {3, 2, 0}, {10, 2, 2}, {11, 2, 3}}},
        {5, {{{3, 1, 3, 2}, {1, 3, 0, 2}}},
         {
             {0, 0, "d1"}, {1, 0, "b1"}, {2, 0, "j1"}, {3, 0, "j1"},
             {2, 1, "e1"}, {3, 1, "c1"}, {0, 2, "i1"}, {1, 2, "d1"},
             {2, 2, "b1"}, {0, 3, "a1"}, {1, 3, "j1"}, {2, 3, "i1"},
             {3, 3, "g1"},
         },
         {{2, 1, 1}, {8, 0, 1}, {10, 1, 3}, {11, 1, 2}}},
    }};
    for (std::size_t layout_index = 0; layout_index < layouts.size();
         ++layout_index) {
        const auto& expected = layouts[layout_index];
        Runtime runtime;
        pal4::UiPropertyGrid grid(0, 0, runtime);
        Check(grid.Dispatch(static_cast<std::int32_t>(layout_index + 6),
                            runtime), "image puzzle callback dispatch");
        Check(runtime.calls.empty() && grid.Grid().RowCount() == 4 &&
                  grid.Grid().ColumnCount() == 4 &&
                  grid.Word(0x28) == 0 && grid.Word(0x2c) == 0 &&
                  grid.Word(0x30) == 0x42c80000 &&
                  grid.Word(0x34) == 0x42c80000 &&
                  grid.Word(0x38) == 0x7777 &&
                  grid.Word(0x48) == expected.budget &&
                  grid.Byte(0x4c) == 1 && grid.Word(0x44) == 1 &&
                  grid.Word(0x40) == 0 &&
                  grid.ElementCount() == expected.cells.size() &&
                  runtime.font_names == std::vector<std::string>{"Puzzle"},
              "image puzzle dimensions, font and budget");
        for (std::size_t slot = 0; slot < 2; ++slot) {
            const auto offset = 0x130 + slot * 0x14;
            const auto& decoration = expected.decorations[slot];
            Check(grid.Word(offset) == decoration[2] &&
                      grid.Word(offset + 4) == decoration[3] &&
                      grid.Word(offset + 8) == decoration[0] &&
                      grid.Word(offset + 12) == decoration[1] &&
                      grid.Byte(offset + 16) == 1,
                  "image puzzle decoration fields");
        }
        const auto begin = grid.Word(0x1c);
        Check(runtime.image_names.size() == expected.cells.size() * 3,
              "one base image and two overlays per element");
        for (std::size_t i = 0; i < expected.cells.size(); ++i) {
            const auto& cell = expected.cells[i];
            const auto element = begin + static_cast<std::uint32_t>(i * 200);
            const auto image = 0x9001u + static_cast<std::uint32_t>(i * 3);
            Check(runtime.image_names[i * 3] == cell.image &&
                      runtime.image_names[i * 3 + 1] == "xuanzhekuang3" &&
                      runtime.image_names[i * 3 + 2] == "xuanzhekuang2" &&
                      runtime.words.at(element) == cell.x &&
                      runtime.words.at(element + 4) == cell.y &&
                      runtime.words.at(element + 8) == 1 &&
                      runtime.words.at(element + 12) == 1 &&
                      runtime.words.at(element + 168) == image &&
                      runtime.words.at(element + 172) == image &&
                      runtime.words.at(element + 176) == image &&
                      runtime.words.at(element + 184) == image + 2 &&
                      runtime.words.at(element + 188) == image + 1 &&
                      runtime.words.at(element + 192) == i + 1 &&
                      grid.Grid().Get(static_cast<std::int32_t>(cell.x),
                                      static_cast<std::int32_t>(cell.y)) ==
                          static_cast<std::int32_t>(i + 1),
                  "image puzzle element coordinates, images and property");
        }
        const auto anchor = grid.Word(0x3c);
        const auto records = runtime.words.at(anchor + 8);
        Check(runtime.words.at(anchor) == 0x849408 &&
                  runtime.bytes.at(anchor + 4) == 0xa5 &&
                  records != 0 &&
                  runtime.words.at(anchor + 12) ==
                      records + expected.links.size() * 12 &&
                  runtime.words.at(anchor + 16) >=
                      runtime.words.at(anchor + 12) &&
                  grid.SequenceRule() != nullptr &&
                  grid.SequenceRule()->Links().size() == expected.links.size(),
              "image puzzle sequence rule and record vector");
        for (std::size_t i = 0; i < expected.links.size(); ++i) {
            const auto& link = expected.links[i];
            const auto address = records + static_cast<std::uint32_t>(i * 12);
            const auto& owned = grid.SequenceRule()->Links()[i];
            Check(runtime.words.at(address) == link[0] &&
                      runtime.words.at(address + 4) == link[1] &&
                      runtime.words.at(address + 8) == link[2] &&
                      owned.property == link[0] && owned.x == link[1] &&
                      owned.y == link[2],
                  "image puzzle linked property and coordinates");
        }
    }
}

void TestImagePuzzleRuleCleanupAndFailure() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    Check(grid.Dispatch(6, runtime), "first image puzzle initialized");
    const auto prior_anchor = grid.Word(0x3c);
    const auto prior_records = runtime.words.at(prior_anchor + 8);
    Check(grid.Dispatch(0, runtime), "puzzle reinitialized after image layout");
    Check(grid.SequenceRule() == nullptr &&
              std::find(runtime.freed.begin(), runtime.freed.end(),
                        prior_anchor) != runtime.freed.end() &&
              std::find(runtime.freed.begin(), runtime.freed.end(),
                        prior_records) != runtime.freed.end(),
          "replaced sequence rule releases its records and anchor");

    for (bool fail_anchor : {true, false}) {
        Runtime failed;
        failed.fail_sequence_anchor = fail_anchor;
        failed.fail_record_allocation = !fail_anchor;
        pal4::UiPropertyGrid partial(0, 0, failed);
        bool rejected = false;
        try {
            static_cast<void>(partial.Dispatch(6, failed));
        } catch (const std::bad_alloc&) {
            rejected = true;
        }
        Check(rejected && partial.Word(0x3c) == 0x7000 &&
                  partial.SequenceRule() == nullptr,
              "sequence allocation failure retains old rule");
    }
}

void TestDecoratedElements() {
    const std::array<std::array<const char*, 3>, 4> names = {{
        {{"fangxiao", "fangxiao2", "fangxiao3"}},
        {{"shu", "shu2", "shu3"}},
        {{"heng", "heng2", "heng3"}},
        {{"fang", "fang2", "fang3"}},
    }};
    const std::array<std::array<std::int32_t, 2>, 4> extents = {{
        {{1, 1}}, {{1, 2}}, {{2, 1}}, {{2, 2}},
    }};
    for (std::size_t kind = 0; kind < extents.size(); ++kind) {
        Runtime runtime;
        pal4::UiPropertyGrid grid(0, 0, runtime);
        grid.Grid().Resize(5, 5);
        grid.AddDecoratedElement(42, {1, 1}, extents[kind], 0x1234,
                                 runtime);
        Check(runtime.image_names ==
                  std::vector<std::string>(names[kind].begin(),
                                           names[kind].end()),
              "resource branch and lookup order");
        const auto stored = grid.Word(0x1c);
        Check(stored == 0xb000 && grid.Word(0x20) == stored + 200 &&
                  grid.Word(0x24) == stored + 200 &&
                  runtime.words.at(stored + 168) == 0x9001 &&
                  runtime.words.at(stored + 172) == 0x9002 &&
                  runtime.words.at(stored + 176) == 0x9003,
              "constructor image slots");
        Check(runtime.words.at(0x8000) == 1 &&
                  runtime.words.at(0x8004) == 1 &&
                  runtime.words.at(0x8008) ==
                      static_cast<std::uint32_t>(extents[kind][0]) &&
                  runtime.words.at(0x800c) ==
                      static_cast<std::uint32_t>(extents[kind][1]) &&
                  runtime.words.at(0x80c0) == 42,
              "property element geometry and identifier");
        Check(runtime.text_source == 0x1234 &&
                  runtime.checked_element == 0x8000 &&
                  runtime.checked_origin ==
                      std::array<std::uint32_t, 2>{1, 1} &&
                  runtime.destroyed_element == 0x8000 &&
                  runtime.destroy_count == 1,
              "text, placement, append and cleanup");
        Check(runtime.events == std::vector<std::string>{
                    "image", "image", "image", "text", "check",
                    "copy", "destroy"},
              "element operation order");
        for (std::int32_t x = 1; x < 1 + extents[kind][0]; ++x) {
            for (std::int32_t y = 1; y < 1 + extents[kind][1]; ++y) {
                Check(grid.Grid().Get(x, y) == 42, "accepted grid paint");
            }
        }
    }
}

void TestRejectedElement() {
    Runtime runtime;
    runtime.can_place = false;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    grid.Grid().Resize(3, 3);
    grid.AddDecoratedElement(7, {1, 1}, {1, 1}, 0x4567, runtime);
    Check(grid.ElementCount() == 0 && runtime.destroy_count == 1 &&
              runtime.allocation_sizes == std::vector<std::uint32_t>{4} &&
              grid.Grid().Get(1, 1) == 0,
          "rejected element skips append and paint but cleans up");
    Check(runtime.events == std::vector<std::string>{
                "image", "image", "image", "text", "check", "destroy"},
          "rejection operation order");
}

void TestSingleCellElement() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    grid.Grid().Resize(4, 4);
    grid.AddSingleCellElement(91, {2, 3}, 0x7777, runtime);
    Check(runtime.image_names == std::vector<std::string>{
                "xuanzhekuang3", "xuanzhekuang2"},
          "single-cell overlay resource order");
    const auto stored = grid.Word(0x1c);
    Check(grid.ElementCount() == 1 &&
              runtime.words.at(stored + 168) == 0x7777 &&
              runtime.words.at(stored + 172) == 0x7777 &&
              runtime.words.at(stored + 176) == 0x7777 &&
              runtime.words.at(0x80b4) == 0 &&
              runtime.words.at(0x80b8) == 0x9002 &&
              runtime.words.at(0x80bc) == 0x9001,
          "base and overlay image slots");
    Check(runtime.words.at(0x8000) == 2 &&
              runtime.words.at(0x8004) == 3 &&
              runtime.words.at(0x8008) == 1 &&
              runtime.words.at(0x800c) == 1 &&
              runtime.words.at(0x80c0) == 91 &&
              grid.Grid().Get(2, 3) == 91,
          "single-cell geometry and painting");
    Check(runtime.text_source == 0 && runtime.destroy_count == 1 &&
              runtime.events == std::vector<std::string>{
                  "image", "image", "check", "copy", "destroy"},
          "single-cell control flow and cleanup");

    Runtime rejected;
    rejected.can_place = false;
    pal4::UiPropertyGrid other(0, 0, rejected);
    other.Grid().Resize(4, 4);
    other.AddSingleCellElement(92, {2, 3}, 0x8888, rejected);
    Check(other.Grid().Get(2, 3) == 0 && other.ElementCount() == 0 &&
              rejected.destroy_count == 1,
          "rejected single-cell element");
}

void TestAnchorReplacement() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    Check(grid.ReplaceAnchor(0x9000, runtime) == 0x9000 &&
              grid.Word(0x3c) == 0x9000 &&
              runtime.freed == std::vector<std::uint32_t>{0x7000},
          "non-null anchor replacement frees previous allocation");
    Check(grid.ReplaceAnchor(0, runtime) == 0 && grid.Word(0x3c) == 0 &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x7000, 0x9000},
          "anchor can be replaced with null");
    Check(grid.ReplaceAnchor(0xaaaa, runtime) == 0 &&
              grid.Word(0x3c) == 0 && runtime.freed.size() == 2,
          "null anchor rejects replacement");

    Runtime allocation_failed;
    allocation_failed.allocation = 0;
    pal4::UiPropertyGrid empty(0, 0, allocation_failed);
    Check(empty.ReplaceAnchor(0xbbbb, allocation_failed) == 0 &&
              empty.Word(0x3c) == 0 && allocation_failed.freed.empty(),
          "failed constructor allocation preserves null anchor");
}

void TestRender() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0xa000, 41, {1, 2}, {2, 3}, {0x1111, 0x1112, 0x1113}, runtime));
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0xa0c8, 42, {3, 4}, {1, 1}, {0x2221, 0x2222, 0x2223}, runtime));
    grid.ConfigureRender(0xa000, 0xa190, {5.0f, 7.0f}, {2.0f, 3.0f},
                         0, 42, false);
    grid.Render({10.0f, 20.0f}, 0.75f, runtime);
    Check(runtime.drawn_images ==
              std::vector<std::uint32_t>{0x1111, 0x2221, 0x2223},
          "selected element draws front layer");
    Check(runtime.drawn_rects[0].left == 17.0f &&
              runtime.drawn_rects[0].right == 21.0f &&
              runtime.drawn_rects[0].top == 33.0f &&
              runtime.drawn_rects[0].bottom == 42.0f,
          "translated and scaled element geometry");
    Check(runtime.image_names.empty(), "disabled decorations do not draw");
    runtime.drawn_images.clear();
    grid.ConfigureRender(0xa000, 0xa190, {5.0f, 7.0f}, {2.0f, 3.0f},
                         0, 42, true);
    grid.Render({10.0f, 20.0f}, 0.75f, runtime);
    Check(runtime.drawn_images ==
              std::vector<std::uint32_t>{0x1111, 0x2221},
          "selection suppression removes front layer");
    runtime.drawn_images.clear();
    grid.ConfigureRender(0, 0, {0.0f, 0.0f}, {1.0f, 1.0f}, 0, 0, false);
    grid.Render({0.0f, 0.0f}, 0.5f, runtime);
    Check(runtime.drawn_images.empty(), "empty element vector");
}

void TestDecorations() {
    const std::array<const char*, 4> names = {
        "shang", "xia", "zuo", "you"};
    for (std::uint32_t type = 0; type < 4; ++type) {
        Runtime runtime;
        pal4::UiPropertyGrid grid(0, 0, runtime);
        grid.ConfigureRender(0, 0, {5.0f, 7.0f}, {2.0f, 3.0f},
                             0, 0, false);
        grid.SetDecoration(0, type, 1, 1, 2, true);
        grid.Render({10.0f, 20.0f}, 0.75f, runtime);
        Check(runtime.image_names == std::vector<std::string>{names[type]} &&
                  runtime.drawn_images ==
                      std::vector<std::uint32_t>{0x9001},
              "decoration type selects resource");
        const auto& rect = runtime.drawn_rects.at(0);
        const float left = type >= 2 ? 14.0f : 18.2f;
        const float top = type >= 2 ? 34.2f : 26.0f;
        Check(rect.left > left - 0.001f && rect.left < left + 0.001f &&
                  rect.top > top - 0.001f && rect.top < top + 0.001f &&
                  rect.right > left + 9.999f &&
                  rect.right < left + 10.001f &&
                  rect.bottom > top + 19.999f &&
                  rect.bottom < top + 20.001f,
              "decoration geometry and alignment");
    }
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    grid.ConfigureRender(0, 0, {0.0f, 0.0f}, {2.0f, 3.0f},
                         0, 0, false);
    const auto first_result = grid.SetDecoration(0, 2, 2, 0, 0, true);
    const auto second_result = grid.SetDecoration(1, 1, 2, 0, 0, true);
    Check(first_result == 2u && second_result == 1u &&
              grid.Word(0x130) == 2u && grid.Word(0x134) == 2u &&
              grid.Word(0x138) == 0u && grid.Word(0x13c) == 0u &&
              grid.Byte(0x140) == 1u &&
              grid.Word(0x144) == 1u && grid.Word(0x148) == 2u &&
              grid.Word(0x14c) == 0u && grid.Word(0x150) == 0u &&
              grid.Byte(0x154) == 1u,
          "both native decoration setters write distinct 20-byte records");
    grid.Render({0.0f, 0.0f}, 0.5f, runtime);
    Check(runtime.image_names ==
              std::vector<std::string>{"zuo", "xia"} &&
              runtime.drawn_rects.size() == 2 &&
              runtime.drawn_rects[0].left == -5.0f &&
              runtime.drawn_rects[0].top > 0.599f &&
              runtime.drawn_rects[0].top < 0.601f &&
              runtime.drawn_rects[1].left > 0.599f &&
              runtime.drawn_rects[1].left < 0.601f &&
              runtime.drawn_rects[1].top == -10.0f,
          "both slots, zero coordinate and alternate alignment");
    runtime.image_names.clear();
    runtime.drawn_images.clear();
    grid.SetDecoration(0, 4, 1, 1, 1, true);
    grid.SetDecoration(1, 1, 1, 1, 1, false);
    grid.Render({0.0f, 0.0f}, 0.5f, runtime);
    Check(runtime.image_names.empty() && runtime.drawn_images.empty(),
          "unknown type and disabled slot are skipped");
}

void TestPointerEvent() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    grid.Grid().Resize(8, 8);
    grid.ConfigureRender(0xa000, 0xa190, {5.0f, 7.0f}, {2.0f, 3.0f},
                         0, 0, false);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0xa000, 11, {1, 1}, {2, 2}, {1, 1, 1}, runtime));
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0xa0c8, 12, {2, 2}, {2, 2}, {1, 1, 1}, runtime));
    Check(grid.ProcessPointerEvent({10.0f, 20.0f},
                                   {19.0f, 33.0f}, runtime) == 2 &&
              grid.Word(0x40) == 0xa0c8 &&
              runtime.bytes.at(0xa0c4) == 1 &&
              runtime.bytes.at(0xa18c) == 1,
          "overlapping hit selects last element");
    runtime.can_place = false;
    Check(grid.ProcessPointerEvent({10.0f, 20.0f},
                                   {25.0f, 45.0f}, runtime) == 1 &&
              grid.Word(0x40) == 0xa0c8 &&
              runtime.bytes.at(0xa0c4) == 0 &&
              runtime.bytes.at(0xa18c) == 1 &&
              runtime.words.at(0xa0c8) == 2,
          "rejected move restores active hit flag");
    runtime.can_place = true;
    Check(grid.ProcessPointerEvent({10.0f, 20.0f},
                                   {25.0f, 45.0f}, runtime) == 0 &&
              runtime.words.at(0xa0c8) == 4 &&
              runtime.words.at(0xa0cc) == 5 &&
              grid.Grid().Get(4, 5) == 12,
          "accepted move commits adjusted coordinates");
    grid.SetDispatchState(0);
    Check(grid.ProcessPointerEvent({10.0f, 20.0f},
                                   {31.0f, 51.0f}, runtime) == 2 &&
              grid.Word(0x40) == 0,
          "no hit and no active element");
}

void TestDirectionalMovement() {
    using Method = std::int32_t (pal4::UiPropertyGrid::*)(
        pal4::IUiPropertyGridRuntime&);
    const std::array<Method, 4> methods = {
        &pal4::UiPropertyGrid::MoveSelectionLeft,
        &pal4::UiPropertyGrid::MoveSelectionRight,
        &pal4::UiPropertyGrid::MoveSelectionUp,
        &pal4::UiPropertyGrid::MoveSelectionDown,
    };
    const std::array<std::array<std::int32_t, 2>, 4> targets = {{
        {{1, 2}}, {{3, 2}}, {{2, 1}}, {{2, 3}},
    }};
    for (std::size_t direction = 0; direction < methods.size(); ++direction) {
        Runtime runtime;
        pal4::UiPropertyGrid grid(0, 0, runtime);
        grid.Grid().Resize(6, 6);
        static_cast<void>(pal4::UiPropertyElement::Initialize(
            0xa000, 41, {2, 2}, {1, 1}, {1, 1, 1}, runtime));
        grid.ConfigureRender(0xa000, 0xa0c8, {0.0f, 0.0f}, {1.0f, 1.0f},
                             0, 41, false);
        grid.SetDispatchState(0xa000);
        runtime.can_place = false;
        Check((grid.*methods[direction])(runtime) == 1 &&
                  runtime.words.at(0xa000) == 2,
              "rejected active directional movement");
        runtime.can_place = true;
        Check((grid.*methods[direction])(runtime) == 0 &&
                  runtime.words.at(0xa000) ==
                      static_cast<std::uint32_t>(targets[direction][0]) &&
                  runtime.words.at(0xa004) ==
                      static_cast<std::uint32_t>(targets[direction][1]),
              "accepted active directional movement");
        grid.SetDispatchState(0);
        static_cast<void>(pal4::UiPropertyElement::Initialize(
            0xa000, 41, {2, 2}, {1, 1}, {1, 1, 1}, runtime));
        static_cast<void>(pal4::UiPropertyElement::Initialize(
            0xa0c8, 42, targets[direction], {1, 1}, {1, 1, 1}, runtime));
        grid.ConfigureRender(0xa000, 0xa190, {0.0f, 0.0f}, {1.0f, 1.0f},
                             0, 41, false);
        Check((grid.*methods[direction])(runtime) == 2 &&
                  grid.Word(0x44) == 42,
              "directional search selects adjacent property");
    }
}

void TestDirectionalBoundaries() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    grid.Grid().Resize(1, 1);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0xa000, 41, {0, 0}, {1, 1}, {1, 1, 1}, runtime));
    grid.ConfigureRender(0xa000, 0xa0c8, {0.0f, 0.0f}, {1.0f, 1.0f},
                         0, 41, false);
    grid.SetDispatchState(0xa000);
    Check(grid.MoveSelectionLeft(runtime) == 1 &&
              runtime.checked_element == 0,
          "left edge skips placement callback");
    Check(grid.MoveSelectionUp(runtime) == 0 &&
              runtime.words.at(0xa000) == 0 &&
              runtime.words.at(0xa004) == 0,
          "up edge rechecks unchanged position");
    grid.SetDispatchState(0);
    Check(grid.MoveSelectionLeft(runtime) == 2 &&
              grid.Word(0x44) == 41,
          "search with no adjacent property");
    grid.ConfigureRender(0xa000, 0xa0c8, {0.0f, 0.0f}, {1.0f, 1.0f},
                         0, 99, false);
    Check(grid.MoveSelectionRight(runtime) == 2,
          "missing selected element returns without dereference");
}

void TestSelectionAndPlacement() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    grid.Grid().Resize(4, 4);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0xa000, 41, {1, 1}, {1, 1}, {1, 1, 1}, runtime));
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0xa0c8, 42, {2, 2}, {1, 1}, {1, 1, 1}, runtime));
    grid.ConfigureRender(0xa000, 0xa190, {0.0f, 0.0f}, {1.0f, 1.0f},
                         0, 42, false);
    Check(grid.FindElement(41, runtime) == 0xa000 &&
              grid.FindElement(42, runtime) == 0xa0c8 &&
              grid.FindElement(43, runtime) == 0,
          "property lookup walks 200-byte records");
    Check(grid.ActivateSelectedElement(runtime) == 0xa0c8 &&
              grid.Word(0x40) == 0xa0c8 &&
              runtime.bytes.at(0xa18c) == 1,
          "activation marks selected element");
    Check(grid.ClearActiveElement(runtime) == 42 &&
              grid.Word(0x40) == 0 && grid.Word(0x44) == 42 &&
              runtime.bytes.at(0xa18c) == 0 &&
              grid.ClearActiveElement(runtime) == 0,
          "clear preserves selected property and resets marker");
    grid.ConfigureRender(0xa000, 0xa190, {0.0f, 0.0f}, {1.0f, 1.0f},
                         0, 99, false);
    Check(grid.ActivateSelectedElement(runtime) == 0 &&
              grid.Word(0x40) == 0,
          "missing selected property is handled safely");
    Check(grid.GetAnchorProperty(runtime) == runtime.anchor_property &&
              runtime.checked_anchor == 0x7000,
          "anchor virtual property getter");
    grid.SetPlacementBudget(true, 0);
    runtime.checked_element = 0;
    Check(!grid.CanPlaceElement(0xa000, {1, 1}, runtime) &&
              runtime.checked_element == 0,
          "exhausted placement budget gates anchor callback");
    grid.SetPlacementBudget(true, 2);
    Check(grid.CanPlaceElement(0xa000, {1, 1}, runtime) &&
              runtime.checked_anchor == 0x7000,
          "positive placement budget calls anchor");
    Check(grid.CommitElementMove(0xa000, {2, 1}, runtime) == 3 &&
              grid.Word(0x48) == 1 && grid.Grid().Get(2, 1) == 41,
          "budgeted commit decrements and paints");
    grid.SetPlacementBudget(false, 0);
    Check(grid.CommitElementMove(0xa000, {1, 2}, runtime) == 2 &&
              grid.Word(0x48) == 0 && grid.Grid().Get(1, 2) == 41,
          "unbudgeted commit paints without decrement");

    Runtime no_anchor_runtime;
    no_anchor_runtime.allocation = 0;
    pal4::UiPropertyGrid no_anchor(0, 0, no_anchor_runtime);
    Check(no_anchor.GetAnchorProperty(no_anchor_runtime) == 0 &&
              !no_anchor.CanPlaceElement(0xa000, {1, 1},
                                         no_anchor_runtime) &&
              no_anchor_runtime.checked_element == 0,
          "null anchor skips both virtual calls");
}

void TestElementInsertion() {
    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x8000, 10, {1, 1}, {1, 1}, {1, 2, 3}, runtime));
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x8100, 20, {2, 2}, {1, 1}, {4, 5, 6}, runtime));
    Check(grid.InsertRepeatedElements(0, 3, 0x8000, runtime) == 15 &&
              grid.ElementCount() == 3 && grid.Word(0x1c) == 0xb000 &&
              grid.Word(0x20) == 0xb258 &&
              grid.Word(0x24) == 0xb258 &&
              runtime.allocation_sizes ==
                  std::vector<std::uint32_t>{4, 600},
          "initial allocation copies three records");
    runtime.words[0x80c0] = 99;
    for (std::size_t i = 0; i < 3; ++i) {
        Check(runtime.words.at(0xb000 + i * 200 + 192) == 10,
              "inserted records do not alias source");
    }
    Check(grid.InsertRepeatedElements(1, 1, 0x8100, runtime) == 20 &&
              grid.ElementCount() == 4 && grid.Word(0x1c) == 0xc000 &&
              grid.Word(0x20) == 0xc320 &&
              grid.Word(0x24) == 0xc4b0 &&
              runtime.freed == std::vector<std::uint32_t>{0xb000} &&
              runtime.destroyed_strings ==
                  std::vector<std::uint32_t>{0xb000, 0xb0c8, 0xb190},
          "growth copies prefix, inserted item and suffix then frees old rows");
    const auto property_at = [&](const std::size_t index) {
        return runtime.words.at(grid.Word(0x1c) +
                                static_cast<std::uint32_t>(index * 200 + 192));
    };
    Check(property_at(0) == 10 && property_at(1) == 20 &&
              property_at(2) == 10 && property_at(3) == 10,
          "growth preserves item order");
    Check(grid.InsertRepeatedElements(2, 1, 0x8100, runtime) == 200 &&
              grid.ElementCount() == 5 && grid.Word(0x1c) == 0xc000 &&
              property_at(0) == 10 && property_at(1) == 20 &&
              property_at(2) == 20 && property_at(3) == 10 &&
              property_at(4) == 10,
          "in-place middle insertion shifts and assigns records");
    const auto aliased_source = grid.Word(0x1c) + 2u * 200u;
    Check(grid.InsertRepeatedElements(0, 1, aliased_source, runtime) == 200 &&
              grid.ElementCount() == 6 && grid.Word(0x1c) == 0xc000 &&
              property_at(0) == 20 && property_at(1) == 10 &&
              property_at(2) == 20 && property_at(3) == 20 &&
              property_at(4) == 10 && property_at(5) == 10 &&
              runtime.freed.back() == 0xd000 &&
              runtime.destroyed_strings.back() == 0xd000,
          "aliased source is snapshotted before in-place shift");
    const auto allocation_count = runtime.allocation_sizes.size();
    Check(grid.InsertRepeatedElements(0, 0, 0x8100, runtime) == 0 &&
              runtime.allocation_sizes.size() == allocation_count,
          "zero-count insertion is inert");
    bool rejected = false;
    try {
        static_cast<void>(grid.InsertRepeatedElements(7, 1, 0x8100,
                                                       runtime));
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    Check(rejected, "out-of-range insertion rejected");
}

void TestInsertionFailureAndAppend() {
    Runtime failed;
    pal4::UiPropertyGrid empty(0, 0, failed);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x8000, 21, {0, 0}, {1, 1}, {1, 1, 1}, failed));
    failed.next_element_allocation = 0;
    bool failed_allocation = false;
    try {
        static_cast<void>(empty.InsertRepeatedElements(0, 1, 0x8000,
                                                        failed));
    } catch (const std::bad_alloc&) {
        failed_allocation = true;
    }
    Check(failed_allocation && empty.ElementCount() == 0 &&
              empty.Word(0x1c) == 0 && empty.Word(0x20) == 0,
          "allocation failure leaves vector unchanged");

    Runtime runtime;
    pal4::UiPropertyGrid grid(0, 0, runtime);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x8000, 21, {0, 0}, {1, 1}, {1, 1, 1}, runtime));
    static_cast<void>(grid.InsertRepeatedElements(0, 2, 0x8000, runtime));
    static_cast<void>(grid.InsertRepeatedElements(1, 1, 0x8000, runtime));
    const auto expected_end = grid.Word(0x20) + 200u;
    Check(grid.InsertRepeatedElements(3, 1, 0x8000, runtime) ==
              expected_end && grid.Word(0x20) == expected_end &&
              grid.ElementCount() == 4,
          "in-place append uses native end-pointer return branch");
}

}  // namespace

int main() {
    try {
        TestConstruction();
        TestAllocationFailure();
        TestDispatch();
        TestFirstPuzzleInitialization();
        TestAdditionalPuzzleInitializers();
        TestImagePuzzleLayouts();
        TestImagePuzzleRuleCleanupAndFailure();
        TestDecoratedElements();
        TestRejectedElement();
        TestSingleCellElement();
        TestAnchorReplacement();
        TestRender();
        TestDecorations();
        TestPointerEvent();
        TestDirectionalMovement();
        TestDirectionalBoundaries();
        TestSelectionAndPlacement();
        TestElementInsertion();
        TestInsertionFailureAndAppend();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
