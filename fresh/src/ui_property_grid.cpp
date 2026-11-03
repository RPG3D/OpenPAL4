#include "pal4/ui_property_grid.h"
#include "pal4/ui_placement_rule.h"
#include "pal4/record_array12.h"

#include <algorithm>
#include <bit>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, UiPropertyGrid::kCallbackCount>
    kInitializerAddresses = {
        0x5ad160, 0x5ad660, 0x5adb60, 0x5ae060, 0x5ae600, 0x5aeae0,
        0x5af090, 0x5af520, 0x5afad0, 0x5affc0, 0x5b0670, 0x5b0cc0,
        0x5b1390, 0x5b1a60, 0x5b2240, 0x5b2bc0, 0x5b3360, 0x5b3b50,
};

constexpr std::array<std::array<const char*, 3>, 4> kImageNames = {{
    {{"fangxiao", "fangxiao2", "fangxiao3"}},
    {{"shu", "shu2", "shu3"}},
    {{"heng", "heng2", "heng3"}},
    {{"fang", "fang2", "fang3"}},
}};

using PuzzleElementSpec = std::array<std::uint32_t, 6>;
constexpr std::array<PuzzleElementSpec, 7> kPuzzleEa = {{
    {{1, 0, 0, 1, 1, 0x5fc3}}, {{2, 0, 1, 1, 1, 0x6000}},
    {{3, 0, 3, 2, 1, 21696}}, {{4, 0, 2, 2, 1, 24863}},
    {{5, 2, 2, 1, 1, 30171}}, {{6, 3, 2, 1, 1, 31070}},
    {{7, 1, 0, 2, 2, 24754}},
}};
constexpr std::array<PuzzleElementSpec, 7> kPuzzleEb = {{
    {{1, 2, 0, 1, 1, 0x6cc9}}, {{2, 0, 1, 1, 2, 0x6e56}},
    {{3, 1, 1, 2, 1, 27827}}, {{4, 3, 1, 1, 1, 28330}},
    {{5, 2, 2, 2, 1, 27743}}, {{6, 1, 2, 1, 2, 27901}},
    {{7, 0, 0, 2, 1, 28023}},
}};
constexpr std::array<PuzzleElementSpec, 7> kPuzzleEc = {{
    {{1, 0, 0, 1, 2, 0x751f}}, {{2, 3, 0, 1, 2, 0x517b}},
    {{3, 1, 1, 1, 2, 21463}}, {{4, 2, 1, 1, 2, 23492}},
    {{5, 0, 3, 2, 1, 33268}}, {{6, 2, 3, 2, 1, 38477}},
    {{7, 1, 0, 2, 1, 20239}},
}};
constexpr std::array<PuzzleElementSpec, 9> kPuzzleEd = {{
    {{1, 1, 0, 2, 1, 0x7761}}, {{2, 0, 1, 2, 1, 0x6655}},
    {{3, 2, 1, 2, 1, 0x5012}}, {{4, 0, 2, 1, 1, 30196}},
    {{5, 1, 2, 2, 1, 27785}}, {{6, 3, 2, 1, 1, 26790}},
    {{8, 0, 3, 2, 1, 97}}, {{9, 2, 3, 2, 1, 26127}},
    {{7, 0, 0, 1, 1, 37257}},
}};
constexpr std::array<PuzzleElementSpec, 6> kPuzzleEe = {{
    {{1, 2, 0, 1, 2, 0x725b}}, {{2, 0, 2, 2, 2, 32650}},
    {{3, 2, 2, 2, 1, 20820}}, {{4, 2, 3, 1, 1, 34382}},
    {{5, 3, 3, 1, 1, 29482}}, {{7, 0, 0, 2, 2, 39532}},
}};
constexpr std::array<PuzzleElementSpec, 9> kPuzzleEf = {{
    {{1, 1, 0, 1, 1, 0x82b1}}, {{2, 1, 1, 1, 1, 0x9c7c}},
    {{3, 1, 2, 2, 1, 0x8349}}, {{4, 3, 2, 1, 1, 34411}},
    {{5, 0, 3, 2, 2, 40857}}, {{6, 2, 3, 1, 1, 27700}},
    {{8, 3, 3, 1, 1, 21494}}, {{9, 2, 4, 1, 1, 20113}},
    {{7, 2, 0, 2, 2, 34430}},
}};

struct ImageCellSpec {
    std::int32_t x;
    std::int32_t y;
    const char* image;
};
struct ImageDecorationSpec {
    std::int32_t x;
    std::int32_t y;
    std::uint32_t type;
    std::uint32_t alignment;
};
struct ImagePuzzleLayout {
    std::uint32_t budget;
    ImageDecorationSpec first;
    ImageDecorationSpec second;
    std::span<const ImageCellSpec> cells;
    std::span<const UiPlacementImageLink> links;
};

constexpr std::array<ImageCellSpec, 5> kImageCells0 = {{
    {0, 0, "d"}, {1, 1, "b"}, {1, 2, "h"},
    {1, 3, "a"}, {3, 2, "g"},
}};
constexpr std::array<UiPlacementImageLink, 3> kImageLinks0 = {{
    {1, 1, 0}, {4, 2, 3}, {5, 2, 2},
}};

constexpr std::array<ImageCellSpec, 7> kImageCells1 = {{
    {3, 0, "b"}, {1, 1, "d"}, {2, 1, "a"},
    {3, 1, "e"}, {1, 2, "b"}, {2, 2, "g"},
    {3, 3, "h"},
}};
constexpr std::array<UiPlacementImageLink, 3> kImageLinks1 = {{
    {3, 2, 0}, {4, 2, 1}, {7, 2, 3},
}};

constexpr std::array<ImageCellSpec, 6> kImageCells2 = {{
    {1, 0, "h"}, {2, 1, "b"}, {2, 2, "d"},
    {3, 2, "h"}, {2, 3, "f"}, {3, 3, "b"},
}};
constexpr std::array<UiPlacementImageLink, 2> kImageLinks2 = {{
    {3, 1, 1}, {5, 2, 2},
}};

constexpr std::array<ImageCellSpec, 9> kImageCells3 = {{
    {0, 0, "g"}, {0, 1, "b"}, {1, 1, "d"},
    {3, 1, "c"}, {0, 2, "f"}, {2, 2, "h"},
    {3, 2, "h"}, {1, 3, "b"}, {2, 3, "a"},
}};
constexpr std::array<UiPlacementImageLink, 3> kImageLinks3 = {{
    {3, 1, 0}, {6, 1, 2}, {7, 3, 3},
}};

constexpr std::array<ImageCellSpec, 8> kImageCells4 = {{
    {3, 0, "a"}, {1, 1, "c"}, {2, 1, "e"},
    {3, 1, "g"}, {1, 2, "f"}, {2, 2, "b"},
    {3, 2, "g"}, {2, 3, "e"},
}};
constexpr std::array<UiPlacementImageLink, 4> kImageLinks4 = {{
    {3, 2, 0}, {2, 2, 1}, {6, 3, 2}, {7, 3, 3},
}};

constexpr std::array<ImageCellSpec, 9> kImageCells5 = {{
    {0, 0, "d"}, {2, 0, "b"}, {3, 0, "e"},
    {0, 1, "f"}, {0, 2, "c"}, {1, 2, "h"},
    {3, 2, "g"}, {1, 3, "d"}, {2, 3, "a"},
}};
constexpr std::array<UiPlacementImageLink, 4> kImageLinks5 = {{
    {2, 1, 0}, {6, 1, 1}, {5, 1, 2}, {7, 2, 2},
}};

constexpr std::array<ImageCellSpec, 9> kImageCells6 = {{
    {1, 0, "e"}, {2, 0, "e"}, {0, 1, "a"},
    {1, 1, "f"}, {0, 2, "h"}, {1, 2, "b"},
    {3, 2, "a"}, {1, 3, "b"}, {2, 3, "g"},
}};
constexpr std::array<UiPlacementImageLink, 4> kImageLinks6 = {{
    {3, 0, 0}, {5, 0, 1}, {6, 0, 2}, {4, 1, 2},
}};

constexpr std::array<ImageCellSpec, 11> kImageCells7 = {{
    {0, 0, "e"}, {1, 0, "h"}, {2, 0, "g"},
    {3, 0, "g"}, {0, 1, "a"}, {3, 1, "f"},
    {1, 2, "h"}, {0, 3, "f"}, {1, 3, "f"},
    {2, 3, "f"}, {3, 3, "h"},
}};
constexpr std::array<UiPlacementImageLink, 4> kImageLinks7 = {{
    {1, 1, 0}, {2, 1, 1}, {10, 2, 2}, {11, 2, 3},
}};

constexpr std::array<ImageCellSpec, 14> kImageCells8 = {{
    {0, 0, "g"}, {1, 0, "d"}, {3, 0, "b"},
    {0, 1, "c"}, {1, 1, "e"}, {3, 1, "c"},
    {0, 2, "e"}, {1, 2, "a"}, {2, 2, "e"},
    {3, 2, "b"}, {0, 3, "f"}, {1, 3, "g"},
    {2, 3, "e"}, {3, 3, "h"},
}};
constexpr std::array<UiPlacementImageLink, 4> kImageLinks8 = {{
    {1, 1, 0}, {2, 2, 0}, {9, 2, 1}, {8, 2, 2},
}};

constexpr std::array<ImageCellSpec, 11> kImageCells9 = {{
    {2, 0, "j1"}, {3, 0, "i1"}, {0, 1, "b1"},
    {1, 1, "d1"}, {2, 1, "i1"}, {3, 1, "f1"},
    {0, 2, "d1"}, {1, 2, "g1"}, {0, 3, "c1"},
    {1, 3, "j1"}, {3, 3, "i1"},
}};
constexpr std::array<UiPlacementImageLink, 2> kImageLinks9 = {{
    {6, 2, 2}, {11, 2, 3},
}};

constexpr std::array<ImageCellSpec, 11> kImageCells10 = {{
    {0, 0, "b1"}, {1, 0, "i1"}, {3, 0, "h1"},
    {0, 1, "d1"}, {2, 1, "e1"}, {3, 1, "h1"},
    {0, 2, "c1"}, {1, 2, "j1"}, {3, 2, "a1"},
    {2, 3, "f1"}, {3, 3, "a1"},
}};
constexpr std::array<UiPlacementImageLink, 4> kImageLinks10 = {{
    {2, 1, 1}, {3, 2, 0}, {10, 2, 2}, {11, 2, 3},
}};

constexpr std::array<ImageCellSpec, 13> kImageCells11 = {{
    {0, 0, "d1"}, {1, 0, "b1"}, {2, 0, "j1"},
    {3, 0, "j1"}, {2, 1, "e1"}, {3, 1, "c1"},
    {0, 2, "i1"}, {1, 2, "d1"}, {2, 2, "b1"},
    {0, 3, "a1"}, {1, 3, "j1"}, {2, 3, "i1"},
    {3, 3, "g1"},
}};
constexpr std::array<UiPlacementImageLink, 4> kImageLinks11 = {{
    {2, 1, 1}, {8, 0, 1}, {10, 1, 3}, {11, 1, 2},
}};

constexpr std::array<ImagePuzzleLayout, 12> kImagePuzzleLayouts = {{
    {3, {1, 0, 0, 0}, {2, 3, 0, 0},
     kImageCells0, kImageLinks0},
    {3, {3, 0, 3, 1}, {2, 3, 0, 0},
     kImageCells1, kImageLinks1},
    {3, {1, 0, 0, 0}, {3, 3, 0, 1},
     kImageCells2, kImageLinks2},
    {3, {0, 0, 0, 1}, {3, 3, 0, 0},
     kImageCells3, kImageLinks3},
    {4, {3, 0, 0, 1}, {3, 3, 0, 0},
     kImageCells4, kImageLinks4},
    {4, {0, 0, 0, 0}, {2, 3, 0, 0},
     kImageCells5, kImageLinks5},
    {4, {1, 0, 0, 0}, {2, 3, 0, 0},
     kImageCells6, kImageLinks6},
    {4, {3, 0, 3, 1}, {2, 3, 0, 0},
     kImageCells7, kImageLinks7},
    {4, {1, 0, 0, 0}, {3, 3, 0, 1},
     kImageCells8, kImageLinks8},
    {3, {3, 0, 0, 2}, {1, 3, 0, 2},
     kImageCells9, kImageLinks9},
    {4, {2, 0, 0, 2}, {2, 3, 0, 2},
     kImageCells10, kImageLinks10},
    {5, {3, 1, 3, 2}, {1, 3, 0, 2},
     kImageCells11, kImageLinks11},
}};

class GridRecordArrayRuntime final : public IRecordArray12Runtime {
public:
    explicit GridRecordArrayRuntime(IUiPropertyGridRuntime& runtime)
        : runtime_(runtime) {}

    std::uint32_t ReadWord(std::uint32_t address) override {
        return runtime_.ReadWord(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        runtime_.WriteWord(address, value);
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        return runtime_.Allocate(size);
    }
    void Free(std::uint32_t pointer) override { runtime_.Free(pointer); }

private:
    IUiPropertyGridRuntime& runtime_;
};

}  // namespace

UiPropertyGrid::UiPropertyGrid(const std::uint8_t base_marker,
                               const std::uint8_t marker,
                               IUiPropertyGridRuntime& runtime)
    : grid_(base_marker) {
    WriteByte(0x18, marker);
    for (std::size_t offset = 0x1c; offset <= 0x38; offset += 4) {
        WriteWord(offset, 0);
    }
    const auto anchor = runtime.Allocate(4);
    if (anchor != 0) runtime.WriteWord(anchor, kAnchorVtable);
    WriteWord(0x3c, anchor);
    for (std::size_t offset = 0x40; offset <= 0x48; offset += 4) {
        WriteWord(offset, 0);
    }
    WriteByte(0x4c, 0);
    WriteWord(0x98, 0);
    WriteWord(0x9c, 32);
    for (std::size_t offset = 0xa0; offset <= 0xac; offset += 4) {
        WriteWord(offset, 0);
    }
    WriteWord(0x138, 0);
    WriteWord(0x13c, 0);
    WriteByte(0x140, 0);
    WriteWord(0x14c, 0);
    WriteWord(0x150, 0);
    WriteByte(0x154, 0);
    for (std::size_t index = 0; index < kCallbackCount; ++index) {
        SetCallback(index, kInitializerAddresses[index]);
    }
}

bool UiPropertyGrid::Dispatch(const std::int32_t index,
                              IUiPropertyGridRuntime& runtime) {
    WriteWord(0x40, 0);
    if (index < 0 || index >= static_cast<std::int32_t>(kCallbackCount)) {
        return false;
    }
    const auto callback = Word(0x50 + static_cast<std::size_t>(index) * 4);
    if (static_cast<std::size_t>(index) < 6 &&
        callback == kInitializerAddresses[static_cast<std::size_t>(index)]) {
        InitializePuzzleLayout(callback, runtime);
    } else if (static_cast<std::size_t>(index) >= 6 &&
               callback == kInitializerAddresses[static_cast<std::size_t>(index)]) {
        InitializeImagePuzzle(static_cast<std::size_t>(index) - 6, runtime);
    } else if (callback != 0) {
        runtime.InvokeCallback(callback, *this);
    }
    return true;
}

void UiPropertyGrid::InitializeFirstPuzzle(IUiPropertyGridRuntime& runtime) {
    InitializePuzzleLayout(0x5ad160, runtime);
}

void UiPropertyGrid::InitializePuzzleLayout(
    const std::uint32_t callback, IUiPropertyGridRuntime& runtime) {
    std::uint32_t budget = 0;
    std::uint32_t text_id = 0;
    std::span<const PuzzleElementSpec> elements;
    switch (callback) {
    case 0x5ad160: budget = 60; text_id = 0xea; elements = kPuzzleEa; break;
    case 0x5ad660: budget = 70; text_id = 0xeb; elements = kPuzzleEb; break;
    case 0x5adb60: budget = 70; text_id = 0xec; elements = kPuzzleEc; break;
    case 0x5ae060: budget = 80; text_id = 0xed; elements = kPuzzleEd; break;
    case 0x5ae600: budget = 90; text_id = 0xee; elements = kPuzzleEe; break;
    case 0x5aeae0: budget = 70; text_id = 0xef; elements = kPuzzleEf; break;
    default: throw std::invalid_argument("unknown puzzle initializer");
    }
    WriteWord(0x28, 0);
    WriteWord(0x2c, 0);
    WriteWord(0x30, std::bit_cast<std::uint32_t>(67.0f));
    WriteWord(0x34, std::bit_cast<std::uint32_t>(68.0f));
    WriteWord(0x48, budget);
    WriteWord(0x38, runtime.LookupFont("Puzzle"));

    grid_.Resize(4, 6);
    static_cast<void>(grid_.Set(0, 5, -2));
    static_cast<void>(grid_.Set(3, 5, -2));
    static_cast<void>(grid_.Set(1, 5, -1));
    static_cast<void>(grid_.Set(2, 5, -1));
    ClearElements(runtime);
    WriteByte(0x4c, 0);
    title_utf8_ = runtime.LookupGameTextUtf8(text_id);
    for (const auto& item : elements) {
        AddDecoratedElement(item[0],
                            {static_cast<std::int32_t>(item[1]),
                             static_cast<std::int32_t>(item[2])},
                            {static_cast<std::int32_t>(item[3]),
                             static_cast<std::int32_t>(item[4])},
                            runtime.SetCodepointText(item[5]), runtime);
    }
    WriteWord(0x44, 1);
    WriteWord(0x40, 0);
    const auto anchor = runtime.Allocate(24);
    if (anchor == 0) throw std::bad_alloc();
    runtime.WriteWord(anchor, UiPlacementRule::kVtable);
    runtime.WriteWord(anchor + 4, 7);
    runtime.WriteWord(anchor + 8, 1);
    runtime.WriteWord(anchor + 12, 5);
    runtime.WriteWord(anchor + 16, 2);
    runtime.WriteWord(anchor + 20, 5);
    static_cast<void>(ReplaceAnchor(anchor, runtime));
}

void UiPropertyGrid::ClearElements(IUiPropertyGridRuntime& runtime) {
    const auto begin = Word(0x1c);
    const auto end = Word(0x20);
    const auto replacement = UiPropertyElement::AssignRange(
        end, end, begin, runtime);
    UiPropertyElement::DestroyRange(replacement, end, runtime);
    WriteWord(0x20, replacement);
}

void UiPropertyGrid::InitializeImagePuzzle(
    const std::size_t layout_index, IUiPropertyGridRuntime& runtime) {
    const auto& layout = kImagePuzzleLayouts.at(layout_index);
    WriteWord(0x28, 0);
    WriteWord(0x2c, 0);
    WriteWord(0x30, std::bit_cast<std::uint32_t>(100.0f));
    WriteWord(0x34, std::bit_cast<std::uint32_t>(100.0f));
    WriteWord(0x38, runtime.LookupFont("Puzzle"));
    grid_.Resize(4, 4);
    ClearElements(runtime);
    WriteByte(0x4c, 1);
    WriteWord(0x48, layout.budget);
    SetDecoration(0, layout.first.type, layout.first.alignment,
                  layout.first.x, layout.first.y, true);
    SetDecoration(1, layout.second.type, layout.second.alignment,
                  layout.second.x, layout.second.y, true);

    for (std::size_t index = 0; index < layout.cells.size(); ++index) {
        const auto& cell = layout.cells[index];
        const auto image = runtime.LookupImage("youshang0", cell.image);
        AddSingleCellElement(static_cast<std::uint32_t>(index + 1),
                             {cell.x, cell.y}, image, runtime);
    }
    WriteWord(0x44, 1);
    WriteWord(0x40, 0);

    const auto anchor = runtime.Allocate(20);
    if (anchor == 0) throw std::bad_alloc();
    runtime.WriteWord(anchor, UiPlacementSequenceRule::kVtable);
    runtime.WriteByte(anchor + 4, runtime.UnwrittenSequenceMarker());
    runtime.WriteWord(anchor + 8, 0);
    runtime.WriteWord(anchor + 12, 0);
    runtime.WriteWord(anchor + 16, 0);
    GridRecordArrayRuntime records(runtime);
    UiPlacementSequenceRule rule;
    try {
        for (const auto& link : layout.links) {
            if (!RecordArray12::InsertValue(
                    anchor + 4, runtime.ReadWord(anchor + 12), 1,
                    {link.property, static_cast<std::uint32_t>(link.x),
                     static_cast<std::uint32_t>(link.y)}, records)) {
                throw std::bad_alloc();
            }
            rule.AddLink(link);
        }
    } catch (...) {
        const auto data = runtime.ReadWord(anchor + 8);
        if (data != 0) runtime.Free(data);
        runtime.Free(anchor);
        throw;
    }
    if (ReplaceAnchor(anchor, runtime) == 0) {
        const auto data = runtime.ReadWord(anchor + 8);
        if (data != 0) runtime.Free(data);
        runtime.Free(anchor);
        return;
    }
    sequence_rule_ = std::move(rule);
    sequence_anchor_ = anchor;
}

void UiPropertyGrid::AddDecoratedElement(
    const std::uint32_t property,
    const std::array<std::int32_t, 2>& origin,
    const std::array<std::int32_t, 2>& extent,
    const std::uint32_t text,
    IUiPropertyGridRuntime& runtime) {
    const auto& names = kImageNames[
        (extent[0] > 1 ? 2u : 0u) + (extent[1] > 1 ? 1u : 0u)];
    const auto primary = runtime.LookupImage("youshang0", names[0]);
    const auto secondary = runtime.LookupImage("youshang0", names[1]);
    const auto tertiary = runtime.LookupImage("youshang0", names[2]);
    const auto element = runtime.TemporaryElementAddress();
    static_cast<void>(UiPropertyElement::Initialize(
        element, property, origin, extent,
        {primary, secondary, tertiary}, runtime));
    try {
        static_cast<void>(UiPropertyElement::AssignText(element, text, runtime));
        PlaceAndAppend(element, origin, runtime);
    } catch (...) {
        runtime.DestroyTemporaryElement(element);
        throw;
    }
    runtime.DestroyTemporaryElement(element);
}

void UiPropertyGrid::AddSingleCellElement(
    const std::uint32_t property,
    const std::array<std::int32_t, 2>& origin,
    const std::uint32_t image,
    IUiPropertyGridRuntime& runtime) {
    const auto element = runtime.TemporaryElementAddress();
    static_cast<void>(UiPropertyElement::Initialize(
        element, property, origin, {1, 1}, {image, image, image}, runtime));
    try {
        const auto third = runtime.LookupImage("youshang0", "xuanzhekuang3");
        const auto second = runtime.LookupImage("youshang0", "xuanzhekuang2");
        static_cast<void>(UiPropertyElement::SetOverlayImages(
            element, {0, second, third}, runtime));
        PlaceAndAppend(element, origin, runtime);
    } catch (...) {
        runtime.DestroyTemporaryElement(element);
        throw;
    }
    runtime.DestroyTemporaryElement(element);
}

void UiPropertyGrid::PlaceAndAppend(
    const std::uint32_t element,
    const std::array<std::int32_t, 2>& origin,
    IUiPropertyGridRuntime& runtime) {
    if (CanPlaceElement(element, origin, runtime)) {
        static_cast<void>(UiPropertyElement::MoveAndPaint(
            element, origin, grid_, runtime));
        static_cast<void>(InsertRepeatedElements(
            ElementCount(), 1, element, runtime));
    }
}

std::uint32_t UiPropertyGrid::ReplaceAnchor(
    const std::uint32_t replacement,
    IUiPropertyGridRuntime& runtime) {
    const auto previous = Word(0x3c);
    if (previous == 0) return 0;
    if (previous == sequence_anchor_) {
        const auto links = runtime.ReadWord(previous + 8);
        if (links != 0) runtime.Free(links);
        sequence_rule_ = UiPlacementSequenceRule{};
        sequence_anchor_ = 0;
    }
    runtime.Free(previous);
    WriteWord(0x3c, replacement);
    return replacement;
}

void UiPropertyGrid::ConfigureRender(
    const std::uint32_t begin, const std::uint32_t end,
    const std::array<float, 2>& local_origin,
    const std::array<float, 2>& scale, const std::uint32_t font,
    const std::uint32_t selected_property,
    const bool suppress_selection) {
    WriteWord(0x1c, begin);
    WriteWord(0x20, end);
    WriteWord(0x28, std::bit_cast<std::uint32_t>(local_origin[0]));
    WriteWord(0x2c, std::bit_cast<std::uint32_t>(local_origin[1]));
    WriteWord(0x30, std::bit_cast<std::uint32_t>(scale[0]));
    WriteWord(0x34, std::bit_cast<std::uint32_t>(scale[1]));
    WriteWord(0x38, font);
    WriteWord(0x40, suppress_selection ? 1u : 0u);
    WriteWord(0x44, selected_property);
}

void UiPropertyGrid::Render(const std::array<float, 2>& origin,
                            const float depth,
                            IUiPropertyGridRuntime& runtime) {
    const std::array<float, 2> translated = {
        origin[0] + std::bit_cast<float>(Word(0x28)),
        origin[1] + std::bit_cast<float>(Word(0x2c)),
    };
    const std::array<float, 2> scale = {
        std::bit_cast<float>(Word(0x30)),
        std::bit_cast<float>(Word(0x34)),
    };
    const auto begin = Word(0x1c);
    const auto end = Word(0x20);
    if (begin != 0 && end >= begin) {
        const auto count = (end - begin) / 200u;
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto element = begin + index * 200u;
            const bool front = Word(0x40) == 0 &&
                runtime.ReadWord(element + 192u) == Word(0x44);
            UiPropertyElement::Render(element, translated, scale, depth,
                                      Word(0x38), front, runtime);
        }
    }
    RenderDecoration(0x130, translated, scale, depth, runtime);
    RenderDecoration(0x144, translated, scale, depth, runtime);
}

std::uint32_t UiPropertyGrid::SetDecoration(
    const std::size_t slot, const std::uint32_t type,
    const std::uint32_t alignment, const std::int32_t x,
    const std::int32_t y, const bool enabled) {
    if (slot >= 2) throw std::out_of_range("UI decoration slot");
    const auto offset = 0x130 + slot * 0x14;
    WriteWord(offset, type);
    WriteWord(offset + 4, alignment);
    WriteWord(offset + 8, static_cast<std::uint32_t>(x));
    WriteWord(offset + 12, static_cast<std::uint32_t>(y));
    WriteByte(offset + 16, enabled ? 1u : 0u);
    return type;
}

void UiPropertyGrid::RenderDecoration(
    const std::size_t offset,
    const std::array<float, 2>& origin,
    const std::array<float, 2>& scale, const float depth,
    IUiPropertyGridRuntime& runtime) {
    if (Byte(offset + 16) == 0) return;
    static constexpr std::array<const char*, 4> kNames = {
        "shang", "xia", "zuo", "you"};
    const auto type = Word(offset);
    if (type >= kNames.size()) return;
    const auto image = runtime.LookupImage("youshang0", kNames[type]);
    const auto dimensions = runtime.ImageDimensions(image);
    float x = static_cast<float>(static_cast<std::int32_t>(Word(offset + 8))) *
                  scale[0] + origin[0];
    float y = static_cast<float>(static_cast<std::int32_t>(Word(offset + 12))) *
                  scale[1] + origin[1];
    const auto alignment = Word(offset + 4);
    if (type >= 2) {
        x += (Word(offset + 8) != 0 ? scale[0] : 0.0f) -
             dimensions[0] * 0.5f;
        if (alignment == 1) y += scale[0] * 0.6f;
        if (alignment == 2) y += scale[0] * 0.3f;
    } else {
        y += (Word(offset + 12) != 0 ? scale[1] : 0.0f) -
             dimensions[1] * 0.5f;
        if (alignment == 1) x += scale[0] * 0.6f;
        if (alignment == 2) x += scale[0] * 0.3f;
    }
    runtime.PrepareWhiteColour();
    runtime.DrawImage(image,
                      {x, x + dimensions[0], y, y + dimensions[1]},
                      depth);
}

std::int32_t UiPropertyGrid::ProcessPointerEvent(
    const std::array<float, 2>& origin,
    const std::array<float, 2>& point,
    IUiPropertyGridRuntime& runtime) {
    const std::array<float, 2> translated = {
        origin[0] + std::bit_cast<float>(Word(0x28)),
        origin[1] + std::bit_cast<float>(Word(0x2c)),
    };
    const std::array<float, 2> scale = {
        std::bit_cast<float>(Word(0x30)),
        std::bit_cast<float>(Word(0x34)),
    };
    const auto cell = IndexedPropertyGrid::Coordinates(
        translated, scale, point[0], point[1]);
    std::uint32_t hit = 0;
    const auto begin = Word(0x1c);
    const auto end = Word(0x20);
    if (begin != 0 && end >= begin) {
        for (std::uint32_t offset = 0; offset < end - begin;
             offset += 200u) {
            const auto element = begin + offset;
            const bool contains = UiPropertyElement::ContainsPoint(
                element, cell, runtime);
            runtime.WriteByte(element + 196u, contains ? 1u : 0u);
            if (contains) hit = element;
        }
    }
    if (hit != 0) {
        WriteWord(0x40, hit);
        return 2;
    }
    const auto active = Word(0x40);
    if (active == 0) {
        WriteWord(0x40, 0);
        return 2;
    }
    const auto adjusted = UiPropertyElement::AdjustPosition(
        active, cell, runtime);
    const bool can_place = CanPlaceElement(active, adjusted, runtime);
    if (can_place) static_cast<void>(CommitElementMove(
        active, adjusted, runtime));
    runtime.WriteByte(active + 196u, 1u);
    return can_place ? 0 : 1;
}

std::int32_t UiPropertyGrid::MoveSelectionLeft(
    IUiPropertyGridRuntime& runtime) {
    return MoveSelection(Direction::Left, runtime);
}

std::int32_t UiPropertyGrid::MoveSelectionRight(
    IUiPropertyGridRuntime& runtime) {
    return MoveSelection(Direction::Right, runtime);
}

std::int32_t UiPropertyGrid::MoveSelectionUp(
    IUiPropertyGridRuntime& runtime) {
    return MoveSelection(Direction::Up, runtime);
}

std::int32_t UiPropertyGrid::MoveSelectionDown(
    IUiPropertyGridRuntime& runtime) {
    return MoveSelection(Direction::Down, runtime);
}

std::int32_t UiPropertyGrid::MoveSelection(
    const Direction direction, IUiPropertyGridRuntime& runtime) {
    const bool horizontal = direction == Direction::Left ||
                            direction == Direction::Right;
    const bool backward = direction == Direction::Left ||
                          direction == Direction::Up;
    const auto active = Word(0x40);
    if (active != 0) {
        std::array<std::int32_t, 2> target = {
            static_cast<std::int32_t>(runtime.ReadWord(active)),
            static_cast<std::int32_t>(runtime.ReadWord(active + 4u)),
        };
        const std::size_t axis = horizontal ? 0 : 1;
        if (!backward || target[axis] > 0) {
            target[axis] += backward ? -1 : 1;
        } else if (direction == Direction::Left) {
            return 1;
        }
        if (!CanPlaceElement(active, target, runtime)) return 1;
        static_cast<void>(CommitElementMove(active, target, runtime));
        return 0;
    }

    const auto selected = Word(0x44);
    const auto current = FindElement(selected, runtime);
    if (current == 0) return 2;
    const auto x = static_cast<std::int32_t>(runtime.ReadWord(current));
    const auto y = static_cast<std::int32_t>(runtime.ReadWord(current + 4u));
    const auto width = static_cast<std::int32_t>(
        runtime.ReadWord(current + 8u));
    const auto height = static_cast<std::int32_t>(
        runtime.ReadWord(current + 12u));
    const auto rows = grid_.RowCount();
    const auto columns = grid_.ColumnCount();
    const auto outer_limit = horizontal ? columns : rows;
    const auto border = horizontal ?
        (backward ? x - 1 : x + width) :
        (backward ? y - 1 : y + height);
    const auto center = horizontal ? y : x;
    const auto line_limit = horizontal ? rows : columns;
    const auto begin = Word(0x1c);
    const auto end = Word(0x20);
    if (begin == 0 || end < begin) return 2;
    const auto count = (end - begin) / 200u;
    for (std::int32_t distance = 0; distance < outer_limit; ++distance) {
        for (std::int32_t side = 0; side < 2; ++side) {
            const auto cross = center + (side == 0 ? distance : -distance);
            if (cross < 0 || cross >= outer_limit ||
                border < 0 || border >= line_limit) continue;
            for (std::int32_t line = border;
                 line >= 0 && line < line_limit;
                 line += backward ? -1 : 1) {
                const std::array<std::int32_t, 2> point = horizontal ?
                    std::array<std::int32_t, 2>{line, cross} :
                    std::array<std::int32_t, 2>{cross, line};
                for (std::uint32_t index = 0; index < count; ++index) {
                    const auto candidate = begin + index * 200u;
                    const auto property = runtime.ReadWord(candidate + 192u);
                    if (property != selected &&
                        UiPropertyElement::ContainsPoint(candidate, point,
                                                         runtime)) {
                        WriteWord(0x44, property);
                        return 2;
                    }
                }
            }
        }
    }
    return 2;
}

std::uint32_t UiPropertyGrid::ActivateSelectedElement(
    IUiPropertyGridRuntime& runtime) {
    static_cast<void>(ClearActiveElement(runtime));
    const auto element = FindElement(Word(0x44), runtime);
    WriteWord(0x40, element);
    if (element != 0) runtime.WriteByte(element + 196u, 1u);
    return element;
}

std::uint32_t UiPropertyGrid::ClearActiveElement(
    IUiPropertyGridRuntime& runtime) {
    const auto element = Word(0x40);
    if (element == 0) return 0;
    runtime.WriteByte(element + 196u, 0u);
    const auto property = runtime.ReadWord(element + 192u);
    WriteWord(0x40, 0);
    WriteWord(0x44, property);
    return property;
}

std::uint32_t UiPropertyGrid::FindElement(
    const std::uint32_t property,
    IUiPropertyElementRuntime& runtime) const {
    const auto begin = Word(0x1c);
    const auto end = Word(0x20);
    if (begin == 0 || end < begin) return 0;
    for (std::uint32_t offset = 0; offset + 200u <= end - begin;
         offset += 200u) {
        const auto element = begin + offset;
        if (runtime.ReadWord(element + 192u) == property) return element;
    }
    return 0;
}

std::uint32_t UiPropertyGrid::GetAnchorProperty(
    IUiPropertyGridRuntime& runtime) {
    const auto anchor = Word(0x3c);
    if (anchor == 0) return 0;
    return runtime.GetAnchorProperty(anchor, *this);
}

bool UiPropertyGrid::CanPlaceElement(
    const std::uint32_t element,
    const std::array<std::int32_t, 2>& origin,
    IUiPropertyGridRuntime& runtime) {
    if (Byte(0x4c) != 0 &&
        static_cast<std::int32_t>(Word(0x48)) <= 0) return false;
    const auto anchor = Word(0x3c);
    return anchor != 0 && runtime.CheckPlacementViaAnchor(
        anchor, *this, element, origin);
}

std::int32_t UiPropertyGrid::CommitElementMove(
    const std::uint32_t element,
    const std::array<std::int32_t, 2>& origin,
    IUiPropertyGridRuntime& runtime) {
    if (Byte(0x4c) != 0) WriteWord(0x48, Word(0x48) - 1u);
    return UiPropertyElement::MoveAndPaint(element, origin, grid_, runtime);
}

void UiPropertyGrid::SetPlacementBudget(
    const bool enabled, const std::int32_t remaining) {
    WriteByte(0x4c, enabled ? 1u : 0u);
    WriteWord(0x48, static_cast<std::uint32_t>(remaining));
}

std::size_t UiPropertyGrid::ElementCount() const {
    const auto begin = Word(0x1c);
    const auto end = Word(0x20);
    return begin == 0 || end < begin ? 0u : (end - begin) / 200u;
}

std::uint32_t UiPropertyGrid::InsertRepeatedElements(
    const std::size_t index, const std::size_t count,
    const std::uint32_t source,
    IUiPropertyGridRuntime& runtime) {
    const auto old_count = ElementCount();
    if (index > old_count) throw std::out_of_range("UI element insertion index");
    if (count == 0) return 0;
    const auto begin = Word(0x1c);
    const auto end = Word(0x20);
    const auto capacity_end = Word(0x24);
    if (count > (std::numeric_limits<std::uint32_t>::max() / 200u) -
                    old_count) {
        throw std::length_error("UI element vector size");
    }
    const auto needed = old_count + count;
    const auto capacity = begin == 0 || capacity_end < begin ?
        0u : static_cast<std::size_t>((capacity_end - begin) / 200u);
    if (needed > capacity) {
        const auto new_capacity = old_count + std::max(old_count, count);
        if (new_capacity > std::numeric_limits<std::uint32_t>::max() / 200u) {
            throw std::length_error("UI element vector capacity");
        }
        const auto new_begin = runtime.Allocate(
            static_cast<std::uint32_t>(new_capacity * 200u));
        if (new_begin == 0) throw std::bad_alloc();
        std::size_t constructed = 0;
        try {
            for (std::size_t i = 0; i < index; ++i, ++constructed) {
                UiPropertyElement::CopyConstruct(
                    new_begin + static_cast<std::uint32_t>(constructed * 200u),
                    begin + static_cast<std::uint32_t>(i * 200u), runtime);
            }
            for (std::size_t i = 0; i < count; ++i, ++constructed) {
                UiPropertyElement::CopyConstruct(
                    new_begin + static_cast<std::uint32_t>(constructed * 200u),
                    source, runtime);
            }
            for (std::size_t i = index; i < old_count; ++i, ++constructed) {
                UiPropertyElement::CopyConstruct(
                    new_begin + static_cast<std::uint32_t>(constructed * 200u),
                    begin + static_cast<std::uint32_t>(i * 200u), runtime);
            }
        } catch (...) {
            for (std::size_t i = 0; i < constructed; ++i) {
                runtime.DestroyElementString(
                    new_begin + static_cast<std::uint32_t>(i * 200u));
            }
            runtime.Free(new_begin);
            throw;
        }
        for (std::size_t i = 0; i < old_count; ++i) {
            runtime.DestroyElementString(
                begin + static_cast<std::uint32_t>(i * 200u));
        }
        if (begin != 0) runtime.Free(begin);
        WriteWord(0x1c, new_begin);
        WriteWord(0x20, new_begin + static_cast<std::uint32_t>(needed * 200u));
        WriteWord(0x24, new_begin +
                           static_cast<std::uint32_t>(new_capacity * 200u));
        return static_cast<std::uint32_t>(5u * needed);
    }

    std::uint32_t copy_source = source;
    std::uint32_t snapshot = 0;
    if (source >= begin && source < end) {
        snapshot = runtime.Allocate(200u);
        if (snapshot == 0) throw std::bad_alloc();
        try {
            UiPropertyElement::CopyConstruct(snapshot, source, runtime);
        } catch (...) {
            runtime.Free(snapshot);
            throw;
        }
        copy_source = snapshot;
    }
    try {
        for (std::size_t i = old_count; i > index; --i) {
            const auto destination_index = i + count - 1;
            const auto destination = begin +
                static_cast<std::uint32_t>(destination_index * 200u);
            const auto from = begin + static_cast<std::uint32_t>((i - 1) * 200u);
            if (destination_index >= old_count) {
                UiPropertyElement::CopyConstruct(destination, from, runtime);
            } else {
                static_cast<void>(UiPropertyElement::Assign(
                    destination, from, runtime));
            }
        }
        for (std::size_t i = 0; i < count; ++i) {
            const auto destination_index = index + i;
            const auto destination = begin +
                static_cast<std::uint32_t>(destination_index * 200u);
            if (destination_index >= old_count) {
                UiPropertyElement::CopyConstruct(
                    destination, copy_source, runtime);
            } else {
                static_cast<void>(UiPropertyElement::Assign(
                    destination, copy_source, runtime));
            }
        }
    } catch (...) {
        if (snapshot != 0) {
            runtime.DestroyElementString(snapshot);
            runtime.Free(snapshot);
        }
        throw;
    }
    if (snapshot != 0) {
        runtime.DestroyElementString(snapshot);
        runtime.Free(snapshot);
    }
    WriteWord(0x20, end + static_cast<std::uint32_t>(count * 200u));
    return static_cast<std::uint32_t>(
        old_count - index < count ? end + count * 200u : count * 200u);
}

std::uint32_t UiPropertyGrid::Word(const std::size_t offset) const {
    if (offset > native_state_.size() - 4) {
        throw std::out_of_range("UI property grid word offset");
    }
    return static_cast<std::uint32_t>(native_state_[offset]) |
           (static_cast<std::uint32_t>(native_state_[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(native_state_[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(native_state_[offset + 3]) << 24);
}

std::uint8_t UiPropertyGrid::Byte(const std::size_t offset) const {
    return native_state_.at(offset);
}

void UiPropertyGrid::SetDispatchState(const std::uint32_t value) {
    WriteWord(0x40, value);
}

void UiPropertyGrid::SetCallback(const std::size_t index,
                                 const std::uint32_t address) {
    if (index >= kCallbackCount) {
        throw std::out_of_range("UI property grid callback index");
    }
    WriteWord(0x50 + index * 4, address);
}

void UiPropertyGrid::WriteWord(const std::size_t offset,
                               const std::uint32_t value) {
    if (offset > native_state_.size() - 4) {
        throw std::out_of_range("UI property grid word offset");
    }
    for (std::size_t byte = 0; byte < 4; ++byte) {
        native_state_[offset + byte] =
            static_cast<std::uint8_t>(value >> (byte * 8));
    }
}

void UiPropertyGrid::WriteByte(const std::size_t offset,
                               const std::uint8_t value) {
    native_state_.at(offset) = value;
}

}  // namespace pal4
