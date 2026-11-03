#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using SaveDataWords = std::array<std::uint32_t, 13>;
using SaveExtraWords = std::array<std::uint32_t, 4>;

class ISystemSettingSaveListRuntime {
public:
    virtual ~ISystemSettingSaveListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list, std::uint32_t index) = 0;
    virtual void AddColumn(std::uint32_t list, std::uint32_t id,
                           float width) = 0;
    [[nodiscard]] virtual bool LoadSaveFile(std::int32_t slot,
                                            std::uint32_t mode,
                                            SaveDataWords& data,
                                            SaveExtraWords& extra) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t SlotMetadata(std::int32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructItem(
        std::uint32_t allocation, std::int32_t slot,
        const SaveDataWords& data, std::uint32_t metadata,
        bool option_two, const SaveExtraWords& extra) = 0;
    virtual void SetItemSize(std::uint32_t item, float size) = 0;
    virtual void AddRow(std::uint32_t list, std::uint32_t row_id) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    virtual void SetItem(std::uint32_t list, std::uint32_t item,
                         std::uint32_t column, std::uint32_t row) = 0;
    virtual void ReleaseExtra(SaveExtraWords& extra) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
    virtual void SetScrollPosition(std::uint32_t scrollbar,
                                   float position) = 0;
    virtual void RequestRedraw(std::uint32_t list) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
    virtual void AddChild(std::uint32_t parent,
                          std::uint32_t child) = 0;
    virtual void SetClippedByParent(std::uint32_t child,
                                    bool clipped) = 0;
    virtual void SetXPosition(std::uint32_t window,
                              std::uint32_t scale,
                              std::uint32_t offset_bits) = 0;
    virtual void SetYPosition(std::uint32_t window,
                              std::uint32_t scale,
                              std::uint32_t offset_bits) = 0;
    virtual void CopyText(std::uint32_t target,
                          std::uint32_t source) = 0;
};

class SystemSettingSaveList final {
public:
    static void Populate(std::uint32_t frame, std::int32_t page,
                         std::uint32_t unused_mode,
                         ISystemSettingSaveListRuntime& runtime);
};

}  // namespace pal4
