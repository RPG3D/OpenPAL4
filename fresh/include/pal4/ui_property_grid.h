#pragma once

#include "pal4/indexed_property_grid.h"
#include "pal4/ui_property_element.h"
#include "pal4/ui_placement_rule.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace pal4 {

class UiPropertyGrid;

class IUiPropertyGridRuntime : public IUiPropertyRenderRuntime {
public:
    virtual ~IUiPropertyGridRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void InvokeCallback(std::uint32_t address,
                                UiPropertyGrid& grid) = 0;
    [[nodiscard]] virtual std::uint32_t LookupFont(const char* name) = 0;
    [[nodiscard]] virtual std::string LookupGameTextUtf8(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t SetCodepointText(
        std::uint32_t codepoint) = 0;
    [[nodiscard]] virtual std::uint32_t LookupImage(
        const char* imageset, const char* image) = 0;
    [[nodiscard]] virtual std::array<float, 2> ImageDimensions(
        std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryElementAddress() = 0;
    [[nodiscard]] virtual std::uint8_t UnwrittenSequenceMarker() = 0;
    [[nodiscard]] virtual bool CheckPlacementViaAnchor(
        std::uint32_t anchor, UiPropertyGrid& grid,
        std::uint32_t element,
        const std::array<std::int32_t, 2>& origin) = 0;
    [[nodiscard]] virtual std::uint32_t GetAnchorProperty(
        std::uint32_t anchor, UiPropertyGrid& grid) = 0;
    void DestroyElementString(std::uint32_t element) override = 0;
    virtual void DestroyTemporaryElement(std::uint32_t element) = 0;
};

class UiPropertyGrid final {
public:
    static constexpr std::size_t kCallbackCount = 18;
    static constexpr std::uint32_t kAnchorVtable = 0x8493f0;

    // IDA 0x5ab2f0: both marker bytes come from unwritten stack slots.
    UiPropertyGrid(std::uint8_t base_marker, std::uint8_t marker,
                   IUiPropertyGridRuntime& runtime);

    // IDA 0x5ab440: the native negative-index path reads before the table.
    [[nodiscard]] bool Dispatch(std::int32_t index,
                                IUiPropertyGridRuntime& runtime);
    // IDA 0x5ad160: initialize the first four-by-six puzzle layout.
    void InitializeFirstPuzzle(IUiPropertyGridRuntime& runtime);
    [[nodiscard]] const std::string& TitleUtf8() const { return title_utf8_; }
    [[nodiscard]] const UiPlacementSequenceRule* SequenceRule() const {
        return sequence_anchor_ == 0 ? nullptr : &sequence_rule_;
    }

    // IDA 0x5ab470: look up three images and insert a 200-byte UI element.
    void AddDecoratedElement(
        std::uint32_t property, const std::array<std::int32_t, 2>& origin,
        const std::array<std::int32_t, 2>& extent, std::uint32_t text,
        IUiPropertyGridRuntime& runtime);
    // IDA 0x5abae0: a one-cell element with selection overlays.
    void AddSingleCellElement(
        std::uint32_t property, const std::array<std::int32_t, 2>& origin,
        std::uint32_t image, IUiPropertyGridRuntime& runtime);
    // IDA 0x5abcd0: replacement only occurs when the old pointer is set.
    [[nodiscard]] std::uint32_t ReplaceAnchor(
        std::uint32_t replacement, IUiPropertyGridRuntime& runtime);
    // IDA 0x5abcf0: render 200-byte records and both trailing decorations.
    void ConfigureRender(std::uint32_t begin, std::uint32_t end,
                         const std::array<float, 2>& local_origin,
                         const std::array<float, 2>& scale,
                         std::uint32_t font, std::uint32_t selected_property,
                         bool suppress_selection);
    void Render(const std::array<float, 2>& origin, float depth,
                IUiPropertyGridRuntime& runtime);
    // IDA 0x5ac900/0x5ac940: the two 20-byte decoration records share
    // this layout; the slot selects native offsets +0x130/+0x144.
    std::uint32_t SetDecoration(std::size_t slot, std::uint32_t type,
                                std::uint32_t alignment, std::int32_t x,
                                std::int32_t y, bool enabled);
    // IDA 0x5ac190: update hit flags or move the active element.
    [[nodiscard]] std::int32_t ProcessPointerEvent(
        const std::array<float, 2>& origin,
        const std::array<float, 2>& point,
        IUiPropertyGridRuntime& runtime);
    // IDA 0x5ac290/0x5ac3f0/0x5ac560/0x5ac6c0.
    [[nodiscard]] std::int32_t MoveSelectionLeft(
        IUiPropertyGridRuntime& runtime);
    [[nodiscard]] std::int32_t MoveSelectionRight(
        IUiPropertyGridRuntime& runtime);
    [[nodiscard]] std::int32_t MoveSelectionUp(
        IUiPropertyGridRuntime& runtime);
    [[nodiscard]] std::int32_t MoveSelectionDown(
        IUiPropertyGridRuntime& runtime);
    // IDA 0x5ac830..0x5ac8e0: selection, lookup and placement lifecycle.
    [[nodiscard]] std::uint32_t ActivateSelectedElement(
        IUiPropertyGridRuntime& runtime);
    [[nodiscard]] std::uint32_t ClearActiveElement(
        IUiPropertyGridRuntime& runtime);
    [[nodiscard]] std::uint32_t FindElement(
        std::uint32_t property, IUiPropertyElementRuntime& runtime) const;
    [[nodiscard]] std::uint32_t GetAnchorProperty(
        IUiPropertyGridRuntime& runtime);
    [[nodiscard]] bool CanPlaceElement(
        std::uint32_t element,
        const std::array<std::int32_t, 2>& origin,
        IUiPropertyGridRuntime& runtime);
    [[nodiscard]] std::int32_t CommitElementMove(
        std::uint32_t element,
        const std::array<std::int32_t, 2>& origin,
        IUiPropertyGridRuntime& runtime);
    void SetPlacementBudget(bool enabled, std::int32_t remaining);
    // IDA 0x5ac980: insert copies into the contiguous 200-byte record vector.
    [[nodiscard]] std::uint32_t InsertRepeatedElements(
        std::size_t index, std::size_t count, std::uint32_t source,
        IUiPropertyGridRuntime& runtime);
    [[nodiscard]] std::size_t ElementCount() const;

    [[nodiscard]] IndexedPropertyGrid& Grid() { return grid_; }
    [[nodiscard]] const IndexedPropertyGrid& Grid() const { return grid_; }
    [[nodiscard]] std::uint32_t Word(std::size_t offset) const;
    [[nodiscard]] std::uint8_t Byte(std::size_t offset) const;
    void SetDispatchState(std::uint32_t value);
    void SetCallback(std::size_t index, std::uint32_t address);

private:
    enum class Direction { Left, Right, Up, Down };
    void InitializePuzzleLayout(std::uint32_t callback,
                                IUiPropertyGridRuntime& runtime);
    void InitializeImagePuzzle(std::size_t layout_index,
                               IUiPropertyGridRuntime& runtime);
    void ClearElements(IUiPropertyGridRuntime& runtime);
    [[nodiscard]] std::int32_t MoveSelection(
        Direction direction, IUiPropertyGridRuntime& runtime);
    void PlaceAndAppend(std::uint32_t element,
                        const std::array<std::int32_t, 2>& origin,
                        IUiPropertyGridRuntime& runtime);
    void RenderDecoration(std::size_t offset,
                          const std::array<float, 2>& origin,
                          const std::array<float, 2>& scale, float depth,
                          IUiPropertyGridRuntime& runtime);
    void WriteWord(std::size_t offset, std::uint32_t value);
    void WriteByte(std::size_t offset, std::uint8_t value);

    IndexedPropertyGrid grid_;
    UiPlacementSequenceRule sequence_rule_;
    std::uint32_t sequence_anchor_ = 0;
    std::string title_utf8_;
    std::array<std::uint8_t, 0x158> native_state_{};
};

}  // namespace pal4
