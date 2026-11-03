#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pal4 {

// Native ownership uses the host STL; the legacy state below is an evidence ABI.
struct AnimationStringTableData final {
    std::string name;
    std::vector<std::pair<std::string, std::string>> entries;
};

struct AnimationStringTableState final {
    std::uint32_t header = 0;
    std::uint32_t text = 0;
    std::uint32_t length = 0;
    std::uint32_t capacity = 0;
    std::uint32_t tag = 0;
    std::uint32_t begin = 0;
    std::uint32_t end = 0;
    std::uint32_t limit = 0;
};
static_assert(sizeof(AnimationStringTableState) == 32);

struct AnimationStringFieldState final {
    std::uint32_t header = 0;
    std::uint32_t text = 0;
    std::uint32_t length = 0;
    std::uint32_t capacity = 0;
};
static_assert(sizeof(AnimationStringFieldState) == 16);

struct AnimationStringRecordState final {
    AnimationStringFieldState name{};
    AnimationStringFieldState value{};
};
static_assert(sizeof(AnimationStringRecordState) == 32);

class IAnimationStringTableRuntime {
public:
    virtual ~IAnimationStringTableRuntime() = default;
    [[nodiscard]] virtual AnimationStringTableState& TableAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual AnimationStringFieldState& FieldAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t left, std::uint32_t right) = 0;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ParseInteger(
        std::uint32_t text) = 0;
    [[nodiscard]] virtual float ParseFloat(std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint8_t ReadReference(
        std::uint32_t address) = 0;
    virtual void WriteReference(std::uint32_t address,
                                std::uint8_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t table,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t table,
                                             std::uint32_t length,
                                             bool exact) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::uint32_t source,
                          std::uint32_t length) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetTextSize(std::uint32_t table,
                             std::uint32_t length) = 0;
    virtual void EraseText(std::uint32_t table,
                           std::uint32_t position,
                           std::uint32_t count) = 0;
    virtual void DetachText(std::uint32_t field) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
    virtual void LoadTableText(std::uint32_t table,
                               std::uint32_t file) = 0;
    virtual void ReadRecordCount(std::uint32_t file,
                                 std::uint32_t& count) = 0;
    virtual void ResetTemporaryText(AnimationStringFieldState& field,
                                    std::uint8_t flag) = 0;
    virtual void LoadTemporaryText(AnimationStringFieldState& field,
                                   std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateRecordArray(
        std::uint32_t bytes) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void CopyTemplateRecord(
        std::uint32_t destination,
        const AnimationStringRecordState& source) = 0;
    virtual void AssignRecordField(std::uint32_t destination,
                                   std::uint32_t source) = 0;
    virtual void AssignTemplateField(
        std::uint32_t destination,
        const AnimationStringFieldState& source) = 0;
};

class AnimationStringTableRuntimeStub
    : public IAnimationStringTableRuntime {
public:
    AnimationStringTableState& TableAt(std::uint32_t) override {
        return table_;
    }
    AnimationStringFieldState& FieldAt(std::uint32_t) override {
        return field_;
    }
    std::uint32_t EmptyText() override { return 0; }
    std::int32_t CompareText(std::uint32_t, std::uint32_t) override {
        return 1;
    }
    std::uint32_t CStringLength(std::uint32_t) override { return 0; }
    std::int32_t ParseInteger(std::uint32_t) override { return 0; }
    float ParseFloat(std::uint32_t) override { return 0.0f; }
    std::uint8_t ReadReference(std::uint32_t) override { return 0; }
    void WriteReference(std::uint32_t, std::uint8_t) override {}
    void SetErrorFlag(std::uint32_t, std::uint8_t) override {}
    bool AllocateText(std::uint32_t, std::uint32_t, bool) override {
        return false;
    }
    void CopyText(std::uint32_t, std::uint32_t,
                  std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void SetTextSize(std::uint32_t, std::uint32_t) override {}
    void EraseText(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {}
    void DetachText(std::uint32_t) override {}
    std::uint32_t Free(std::uint32_t) override { return 0; }
    void LoadTableText(std::uint32_t, std::uint32_t) override {}
    void ReadRecordCount(std::uint32_t, std::uint32_t&) override {}
    void ResetTemporaryText(AnimationStringFieldState&,
                            std::uint8_t) override {}
    void LoadTemporaryText(AnimationStringFieldState&,
                           std::uint32_t) override {}
    std::uint32_t AllocateRecordArray(std::uint32_t) override { return 0; }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override;
    void CopyTemplateRecord(
        std::uint32_t destination,
        const AnimationStringRecordState& source) override;
    void AssignRecordField(std::uint32_t destination,
                           std::uint32_t source) override;
    void AssignTemplateField(std::uint32_t,
                             const AnimationStringFieldState&) override;

private:
    AnimationStringTableState table_{};
    AnimationStringFieldState field_{};
};

class AnimationStringTable final {
public:
    [[nodiscard]] static bool LoadFromBytes(
        AnimationStringTableData& table, std::span<const std::uint8_t>& bytes);
    [[nodiscard]] static const std::string* FindString(
        const AnimationStringTableData& table, std::string_view name);
    [[nodiscard]] static std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source,
        IAnimationStringTableRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t table, IAnimationStringTableRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindString(
        std::uint32_t table, std::uint32_t name,
        IAnimationStringTableRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyString(
        std::uint32_t table, std::uint32_t destination,
        std::uint32_t name, std::uint32_t fallback,
        IAnimationStringTableRuntime& runtime);
    [[nodiscard]] static std::int32_t ConvertStringToInt(
        std::uint32_t table, std::uint32_t name,
        std::int32_t fallback, IAnimationStringTableRuntime& runtime);
    [[nodiscard]] static float ConvertStringToFloat(
        std::uint32_t table, std::uint32_t name,
        float fallback, IAnimationStringTableRuntime& runtime);
    [[nodiscard]] static bool LoadFromFile(
        std::uint32_t table, std::uint32_t file,
        IAnimationStringTableRuntime& runtime);
    static void InsertRecords(
        std::uint32_t table, std::uint32_t position,
        std::uint32_t count, const AnimationStringRecordState& record,
        IAnimationStringTableRuntime& runtime);
    static void CopyRecord(
        std::uint32_t destination, std::uint32_t source,
        IAnimationStringTableRuntime& runtime);
    static void CopyRecord(
        std::uint32_t destination,
        const AnimationStringRecordState& source,
        IAnimationStringTableRuntime& runtime);
    [[nodiscard]] static std::uint32_t AssignRecord(
        std::uint32_t destination, std::uint32_t source,
        IAnimationStringTableRuntime& runtime);
    static void AssignField(
        std::uint32_t destination,
        const AnimationStringFieldState& source,
        IAnimationStringTableRuntime& runtime);
};

}  // namespace pal4
