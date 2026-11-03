#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "pal4/save_window_lifecycle.h"
#include "pal4/save_slot_data.h"

namespace pal4 {

struct SaveListExtra {
    std::array<std::uint32_t, 4> words{};
};

class ISaveWindowPageRuntime {
public:
    virtual ~ISaveWindowPageRuntime() = default;
    [[nodiscard]] virtual float ScrollbarExtent(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::int32_t GlobalSaveIndex() = 0;
    [[nodiscard]] virtual float GlobalScrollPosition() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
    virtual void AddChild(std::uint32_t parent,
                          std::uint32_t child) = 0;
    virtual void SetClippedByParent(std::uint32_t window, bool clipped) = 0;
    virtual void SetX(std::uint32_t window, std::uint32_t mode,
                      float value) = 0;
    virtual void SetY(std::uint32_t window, std::uint32_t mode,
                      float value) = 0;
    virtual void CopyButtonText(std::uint32_t target,
                                std::uint32_t source) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual bool LoadSave(std::int32_t slot,
                                        std::uint32_t mode,
                                        SaveSlotData& data,
                                        SaveListExtra& extra) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateItem(std::size_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t NumberedItemAux(
        std::int32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructItem(
        std::uint32_t storage, std::int32_t slot,
        const SaveSlotData& data, std::uint32_t auxiliary,
        std::uint32_t zero, const SaveListExtra& extra) = 0;
    virtual void SetItemSize(std::uint32_t item, float size) = 0;
    virtual void AddRow(std::uint32_t list, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    virtual void SetItem(std::uint32_t list, std::uint32_t item,
                         std::uint32_t column, std::uint32_t row) = 0;
    virtual void ReleaseExtra(SaveListExtra& extra) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
    virtual void SetScrollPosition(std::uint32_t list, float value) = 0;
    virtual void RequestRedraw(std::uint32_t list) = 0;
};

class SaveWindowPageRuntimeStub : public ISaveWindowPageRuntime {
public:
    [[nodiscard]] float ScrollbarExtent(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::int32_t GlobalSaveIndex() override { return 0; }
    [[nodiscard]] float GlobalScrollPosition() override { return 0.0f; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void RemoveChild(std::uint32_t, std::uint32_t) override {}
    void AddChild(std::uint32_t, std::uint32_t) override {}
    void SetClippedByParent(std::uint32_t, bool) override {}
    void SetX(std::uint32_t, std::uint32_t, float) override {}
    void SetY(std::uint32_t, std::uint32_t, float) override {}
    void CopyButtonText(std::uint32_t, std::uint32_t) override {}
    void ResetList(std::uint32_t) override {}
    [[nodiscard]] bool LoadSave(std::int32_t, std::uint32_t,
                                SaveSlotData&, SaveListExtra&) override {
        return false;
    }
    [[nodiscard]] std::uint32_t AllocateItem(std::size_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t NumberedItemAux(
        std::int32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructItem(
        std::uint32_t storage, std::int32_t,
        const SaveSlotData&, std::uint32_t,
        std::uint32_t, const SaveListExtra&) override { return storage; }
    void SetItemSize(std::uint32_t, float) override {}
    void AddRow(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t RowCount(std::uint32_t) override { return 0u; }
    void SetItem(std::uint32_t, std::uint32_t,
                 std::uint32_t, std::uint32_t) override {}
    void ReleaseExtra(SaveListExtra&) override {}
    void SetHoverRow(std::uint32_t, std::uint32_t) override {}
    void SetScrollPosition(std::uint32_t, float) override {}
    void RequestRedraw(std::uint32_t) override {}
};

class SaveWindowPage final {
public:
    static void Populate(SaveWindowState& state, std::int32_t page,
                         std::uint8_t reset,
                         ISaveWindowPageRuntime& runtime);
};

}  // namespace pal4
