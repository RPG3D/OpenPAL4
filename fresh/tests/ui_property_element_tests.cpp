#include "pal4/ui_property_element.h"

#include "pal4/indexed_property_grid.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiPropertyRenderRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> byte_writes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> drawn_images;
    std::vector<pal4::UiFloatRect> drawn_rects;
    std::vector<std::string> render_calls;
    pal4::UiFloatRect text_rect;
    float font_metric = 0;
    float drawn_depth = 0;
    int text_calls = 0;
    std::array<std::uint32_t, 4> assignment{};
    int assign_count = 0;
    std::array<std::uint32_t, 2> copy_string{};
    int copy_string_count = 0;
    std::vector<std::uint32_t> destroyed_elements;
    std::vector<std::array<std::uint32_t, 4>> assignments;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address - object, value);
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        byte_writes.emplace_back(address - object, value);
        bytes[address] = value;
    }
    std::uint32_t AssignCeguiString(std::uint32_t destination,
                                     std::uint32_t source,
                                     std::uint32_t start,
                                     std::uint32_t length) override {
        assignment = {destination, source, start, length};
        assignments.push_back(assignment);
        ++assign_count;
        return 0x7000u;
    }
    void DestroyElementString(std::uint32_t element) override {
        destroyed_elements.push_back(element);
    }
    void CopyConstructCeguiString(std::uint32_t destination,
                                  std::uint32_t source) override {
        copy_string = {destination, source};
        ++copy_string_count;
        words[destination] = words.at(source);
    }
    void PrepareWhiteColour() override {
        render_calls.emplace_back("white");
    }
    void DrawImage(std::uint32_t image, const pal4::UiFloatRect& rect,
                   float depth) override {
        render_calls.emplace_back("image");
        drawn_images.push_back(image);
        drawn_rects.push_back(rect);
        drawn_depth = depth;
    }
    float ReadFontMetric(std::uint32_t font) override {
        Check(font == 0x9000u, "font handle");
        render_calls.emplace_back("font metric");
        return font_metric;
    }
    void PrepareDefaultFontColours() override {
        render_calls.emplace_back("font colours");
    }
    void DrawText(std::uint32_t font, std::uint32_t text,
                  const pal4::UiFloatRect& rect, float depth,
                  std::uint32_t formatting) override {
        Check(font == 0x9000u && text == object + 16u &&
                  depth == 0.75f && formatting == 2u,
              "font drawing arguments");
        render_calls.emplace_back("text");
        text_rect = rect;
        ++text_calls;
    }
};

void TestInitialize() {
    Runtime runtime;
    Check(pal4::UiPropertyElement::Initialize(
              Runtime::object, 0x77u, {-3, 5}, {9, 11},
              {0x2000u, 0x2100u, 0x2200u}, runtime) == Runtime::object,
          "constructor returns original object");
    const auto expected = std::vector<std::pair<std::uint32_t,
                                                std::uint32_t>>({
        {0u, static_cast<std::uint32_t>(-3)}, {4u, 5u},
        {8u, 9u}, {12u, 11u}, {20u, 32u}, {24u, 0u},
        {32u, 0u}, {28u, 0u}, {16u, 0u}, {36u, 0u},
        {168u, 0x2000u}, {172u, 0x2100u}, {176u, 0x2200u},
        {192u, 0x77u}, {180u, 0u}, {184u, 0u}, {188u, 0u}});
    Check(runtime.writes == expected, "exact constructor word layout and order");
    Check(runtime.byte_writes == std::vector<
              std::pair<std::uint32_t, std::uint8_t>>({{196u, 0u}}),
          "constructor clears the byte at offset 196");
}

void TestSetAndAssign() {
    Runtime runtime;
    Check(pal4::UiPropertyElement::SetOverlayImages(
              Runtime::object, {0x3000u, 0u, 0x3200u}, runtime) == 0x3200u,
          "overlay setter returns third image handle");
    Check(runtime.writes == std::vector<
              std::pair<std::uint32_t, std::uint32_t>>({
                  {180u, 0x3000u}, {184u, 0u}, {188u, 0x3200u}}),
          "overlay fields written in order");
    Check(pal4::UiPropertyElement::AssignText(
              Runtime::object, 0x5000u, runtime) == 0x7000u,
          "CEGUI assignment result forwarded");
    Check(runtime.assign_count == 1 && runtime.assignment ==
              std::array<std::uint32_t, 4>{
                  Runtime::object + 16u, 0x5000u, 0u, 0xffffffffu},
          "CEGUI string subobject, source, start and npos forwarded");
}

void TestOccupancyAndMovement() {
    Runtime runtime;
    pal4::IndexedPropertyGrid grid(0);
    grid.Resize(5, 5);
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        Runtime::object, 7u, {1, 1}, {2, 2}, {0, 0, 0}, runtime));
    Check(pal4::UiPropertyElement::CanOccupy(
              Runtime::object, {1, 1}, grid, runtime) &&
              pal4::UiPropertyElement::CanOccupy(
                  Runtime::object, {2, 1}, grid, runtime),
          "same and adjacent positions accepted when grid is clear");
    Check(!pal4::UiPropertyElement::CanOccupy(
              Runtime::object, {3, 1}, grid, runtime),
          "Manhattan displacement above one rejected");
    static_cast<void>(grid.Set(2, 2, 9));
    Check(!pal4::UiPropertyElement::CanOccupy(
              Runtime::object, {1, 1}, grid, runtime),
          "different nonzero property blocks occupancy");
    static_cast<void>(grid.Set(2, 2, 7));
    Check(pal4::UiPropertyElement::CanOccupy(
              Runtime::object, {1, 1}, grid, runtime),
          "matching property is allowed");
    pal4::IndexedPropertyGrid small(0);
    small.Resize(3, 3);
    Check(!pal4::UiPropertyElement::CanOccupy(
              Runtime::object, {2, 1}, small, runtime),
          "out-of-range grid sentinel blocks occupancy");

    runtime.writes.clear();
    Check(pal4::UiPropertyElement::MoveAndPaint(
              Runtime::object, {2, 1}, grid, runtime) == 4,
          "move returns new right edge");
    Check(runtime.writes == std::vector<
              std::pair<std::uint32_t, std::uint32_t>>({
                  {0u, 2u}, {4u, 1u}}),
          "move updates only origin words");
    Check(grid.Get(1, 1) == 0 && grid.Get(1, 2) == 0 &&
              grid.Get(2, 1) == 7 && grid.Get(2, 2) == 7 &&
              grid.Get(3, 1) == 7 && grid.Get(3, 2) == 7,
          "old cells cleared and new rectangle painted");

    Runtime empty;
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        Runtime::object, 4u, {0, 0}, {0, 0}, {0, 0, 0}, empty));
    Check(pal4::UiPropertyElement::CanOccupy(
              Runtime::object, {0, 0}, grid, empty) &&
              pal4::UiPropertyElement::MoveAndPaint(
                  Runtime::object, {1, 0}, grid, empty) == 1,
          "zero-sized rectangle skips property scans");
}

void TestRender() {
    Runtime runtime;
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        Runtime::object, 7u, {2, 3}, {4, 5},
        {0x2000u, 0x2100u, 0x2200u}, runtime));
    static_cast<void>(pal4::UiPropertyElement::SetOverlayImages(
        Runtime::object, {0x3000u, 0x3100u, 0x3200u}, runtime));
    runtime.words[Runtime::object + 16u] = 0x5000u;
    runtime.font_metric = 10.0f;
    pal4::UiPropertyElement::Render(Runtime::object, {10.0f, 20.0f},
                                    {5.0f, 2.0f}, 0.75f, 0x9000u,
                                    true, runtime);
    Check(runtime.drawn_images == std::vector<std::uint32_t>({
              0x2000u, 0x3000u, 0x2200u, 0x3200u}),
          "normal base, overlay and front draw order");
    Check(runtime.render_calls == std::vector<std::string>({
              "white", "image", "image", "image", "image",
              "font metric", "font colours", "text"}),
          "white and default font colour setup around native draws");
    Check(runtime.drawn_rects[0].left == 20.0f &&
              runtime.drawn_rects[0].right == 40.0f &&
              runtime.drawn_rects[0].top == 26.0f &&
              runtime.drawn_rects[0].bottom == 36.0f &&
              runtime.drawn_depth == 0.75f,
          "screen rectangle uses origin and separate scales");
    Check(runtime.text_calls == 1 && runtime.text_rect.left == 20.0f &&
              runtime.text_rect.right == 40.0f && runtime.text_rect.top == 26.0f &&
              runtime.text_rect.bottom == 36.0f,
          "font height equal to cell height preserves both axes");

    runtime.drawn_images.clear();
    runtime.render_calls.clear();
    runtime.bytes[Runtime::object + 196u] = 1u;
    runtime.words[Runtime::object + 172u] = 0u;
    runtime.font_metric = 8.0f;
    pal4::UiPropertyElement::Render(Runtime::object, {10.0f, 20.0f},
                                    {5.0f, 2.0f}, 0.75f, 0x9000u,
                                    false, runtime);
    Check(runtime.drawn_images == std::vector<std::uint32_t>({0x3100u}),
          "selected state skips empty selected image and draws its overlay");
    Check(runtime.text_rect.left == 20.0f && runtime.text_rect.right == 40.0f &&
              runtime.text_rect.top == 27.0f && runtime.text_rect.bottom == 35.0f,
          "asymmetric cell centers font height vertically and preserves full horizontal extent");

    runtime.drawn_images.clear();
    runtime.render_calls.clear();
    runtime.font_metric = 30.0f;
    pal4::UiPropertyElement::Render(Runtime::object, {10.0f, 20.0f},
                                    {5.0f, 2.0f}, 0.75f, 0x9000u,
                                    false, runtime);
    Check(runtime.text_rect.left == 20.0f &&
              runtime.text_rect.right == 40.0f && runtime.text_rect.top == 26.0f &&
              runtime.text_rect.bottom == 36.0f,
          "oversized font height preserves original text rectangle");
    runtime.words[Runtime::object + 16u] = 0u;
    runtime.text_calls = 0;
    pal4::UiPropertyElement::Render(Runtime::object, {10.0f, 20.0f},
                                    {5.0f, 2.0f}, 0.75f, 0x9000u,
                                    false, runtime);
    Check(runtime.text_calls == 0, "empty text skips font drawing");

    Runtime empty;
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        Runtime::object, 0u, {0, 0}, {1, 1}, {0, 0, 0}, empty));
    pal4::UiPropertyElement::Render(Runtime::object, {0.0f, 0.0f},
                                    {1.0f, 1.0f}, 0.75f, 0u,
                                    false, empty);
    Check(empty.drawn_images == std::vector<std::uint32_t>({0u}) &&
              empty.text_calls == 0,
          "normal base image call is unconditional and null font skips text");
}

void TestGeometryHelpers() {
    Runtime runtime;
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        Runtime::object, 7u, {2, 3}, {4, 5}, {0, 0, 0}, runtime));
    Check(pal4::UiPropertyElement::ContainsPoint(
              Runtime::object, {2, 3}, runtime) &&
              pal4::UiPropertyElement::ContainsPoint(
                  Runtime::object, {5, 7}, runtime),
          "rectangle includes lower edge and final interior cell");
    for (const auto point :
         std::array<std::array<std::int32_t, 2>, 4>{{
             {1, 3}, {2, 2}, {6, 3}, {2, 8}}}) {
        Check(!pal4::UiPropertyElement::ContainsPoint(
                  Runtime::object, point, runtime),
              "rectangle excludes lower-outside and upper edges");
    }
    const auto rect = pal4::UiPropertyElement::ScreenRect(
        Runtime::object, {10.0f, 20.0f}, {5.0f, 2.0f}, runtime);
    Check(rect.left == 20.0f && rect.right == 40.0f &&
              rect.top == 26.0f && rect.bottom == 36.0f,
          "screen rectangle uses per-axis scale and origin");

    Check(pal4::UiPropertyElement::AdjustPosition(
              Runtime::object, {8, 9}, runtime) ==
              std::array<std::int32_t, 2>{5, 5},
          "target beyond both origins shifts by extent minus one");
    Check(pal4::UiPropertyElement::AdjustPosition(
              Runtime::object, {2, 9}, runtime) ==
              std::array<std::int32_t, 2>{2, 5},
          "equal x origin is not adjusted");
    Check(pal4::UiPropertyElement::AdjustPosition(
              Runtime::object, {0, 1}, runtime) ==
              std::array<std::int32_t, 2>{0, 1},
          "targets below both origins remain unchanged");
}

void TestRecordCopyAndAssign() {
    Runtime runtime;
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x1000, 57, {2, 3}, {4, 5}, {11, 12, 13}, runtime));
    static_cast<void>(pal4::UiPropertyElement::SetOverlayImages(
        0x1000, {14, 15, 16}, runtime));
    runtime.words[0x1010] = 2;
    runtime.bytes[0x10c4] = 1;
    pal4::UiPropertyElement::CopyConstruct(0, 0x1000, runtime);
    Check(runtime.copy_string_count == 0, "null copy destination");
    pal4::UiPropertyElement::CopyConstruct(0x2000, 0x1000, runtime);
    Check(runtime.copy_string_count == 1 &&
              runtime.copy_string ==
                  std::array<std::uint32_t, 2>{0x2010, 0x1010} &&
              runtime.words.at(0x2010) == 2 &&
              runtime.words.at(0x2014) == 32,
          "copy constructs CEGUI string storage");
    for (const std::uint32_t offset :
         {0u, 4u, 8u, 12u, 168u, 172u, 176u, 180u, 184u, 188u, 192u}) {
        Check(runtime.words.at(0x2000 + offset) ==
                  runtime.words.at(0x1000 + offset),
              "copy constructs geometry and trailing fields");
    }
    Check(runtime.bytes.at(0x20c4) == 1, "copy constructs hit byte");
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        0x3000, 99, {0, 0}, {1, 1}, {1, 2, 3}, runtime));
    Check(pal4::UiPropertyElement::Assign(0x3000, 0x1000, runtime) ==
              0x3000 &&
              runtime.assignment ==
                  std::array<std::uint32_t, 4>{
                      0x3010, 0x1010, 0, 0xffffffffu},
          "assignment reuses destination CEGUI string");
    for (const std::uint32_t offset :
         {0u, 4u, 8u, 12u, 168u, 172u, 176u, 180u, 184u, 188u, 192u}) {
        Check(runtime.words.at(0x3000 + offset) ==
                  runtime.words.at(0x1000 + offset),
              "assignment copies geometry and trailing fields");
    }
    Check(runtime.bytes.at(0x30c4) == 1, "assignment copies hit byte");
}

void TestRecordRanges() {
    Runtime runtime;
    constexpr std::uint32_t source = 0x1200;
    constexpr std::uint32_t destination = 0x2000;
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        source, 11, {1, 2}, {1, 1}, {21, 22, 23}, runtime));
    static_cast<void>(pal4::UiPropertyElement::Initialize(
        source + 200, 12, {3, 4}, {2, 1}, {31, 32, 33}, runtime));
    Check(pal4::UiPropertyElement::AssignRange(
              source, source, destination, runtime) == destination &&
              runtime.assignments.empty(),
          "empty element assignment range");
    Check(pal4::UiPropertyElement::AssignRange(
              source, source + 400, destination, runtime) ==
              destination + 400 &&
              runtime.assignments ==
                  std::vector<std::array<std::uint32_t, 4>>{
                      {destination + 16, source + 16, 0, 0xffffffffu},
                      {destination + 216, source + 216, 0, 0xffffffffu}},
          "range assigns strings forward and returns destination end");
    for (std::uint32_t index = 0; index < 2; ++index) {
        const auto offset = index * 200;
        for (std::uint32_t field :
             {0u, 4u, 8u, 12u, 168u, 172u, 176u, 180u, 184u, 188u,
              192u}) {
            Check(runtime.words.at(destination + offset + field) ==
                      runtime.words.at(source + offset + field),
                  "range copies geometry, images and properties");
        }
        Check(runtime.bytes.at(destination + offset + 196) ==
                  runtime.bytes.at(source + offset + 196),
              "range copies hit bytes");
    }
    pal4::UiPropertyElement::DestroyRange(destination, destination,
                                          runtime);
    Check(runtime.destroyed_elements.empty(), "empty destruction range");
    pal4::UiPropertyElement::DestroyRange(destination, destination + 400,
                                          runtime);
    Check(runtime.destroyed_elements ==
              std::vector<std::uint32_t>{destination, destination + 200},
          "range destroys two strings in order");
    bool invalid_assign = false;
    bool invalid_destroy = false;
    try {
        static_cast<void>(pal4::UiPropertyElement::AssignRange(
            source, source + 201, destination, runtime));
    } catch (const std::invalid_argument&) {
        invalid_assign = true;
    }
    try {
        pal4::UiPropertyElement::DestroyRange(destination + 1,
                                              destination + 200, runtime);
    } catch (const std::invalid_argument&) {
        invalid_destroy = true;
    }
    Check(invalid_assign && invalid_destroy,
          "misaligned record ranges rejected");
}

}  // namespace

int main() {
    try {
        TestInitialize();
        TestSetAndAssign();
        TestOccupancyAndMovement();
        TestRender();
        TestGeometryHelpers();
        TestRecordCopyAndAssign();
        TestRecordRanges();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
