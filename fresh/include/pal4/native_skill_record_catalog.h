#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace pal4 {
class NativeGameDatabase;
struct NativeGameDbTable;

// Full132-byte Skill records; shared SkillPropertyRegistry defines all fields.
// The small catalog/table images bind only the fields used by SkillDataLookup.
class NativeSkillRecordCatalog final {
public:
    static constexpr std::uint32_t kRecordBytes = 132u;
    using Record = std::array<std::uint32_t, kRecordBytes / 4u>;
    explicit NativeSkillRecordCatalog(const NativeGameDatabase& database);
    explicit NativeSkillRecordCatalog(const NativeGameDbTable& table);
    NativeSkillRecordCatalog(const NativeSkillRecordCatalog&) = delete;
    NativeSkillRecordCatalog& operator=(const NativeSkillRecordCatalog&) = delete;
    [[nodiscard]] std::uint32_t Address(std::uint32_t id) const;
    [[nodiscard]] std::uint32_t CatalogAddress() const noexcept;
    // IDA588F70: nonnull records pass through; null resolves to the owned
    // original fallback. Foreign pointers are rejected at the native boundary.
    [[nodiscard]] std::uint32_t ResolveRecord(std::uint32_t address);
    [[nodiscard]] bool OwnsRecord(std::uint32_t address) const noexcept;
    [[nodiscard]] std::size_t Size() const noexcept { return records_.size(); }
    [[nodiscard]] const Record& At(std::size_t index) const { return records_.at(index); }
private:
    struct Index { std::int32_t key; std::uint32_t position; };
    static_assert(sizeof(Index) == 8u);
    std::vector<Record> records_;
    std::vector<std::array<std::unique_ptr<std::uint8_t[]>, 4u>> text_;
    std::vector<Index> index_;
    std::array<std::uint32_t, 35u> table_{};
    std::array<std::uint32_t, 2u> catalog_{};
    Record fallback_{};
    std::uint8_t fallback_guard_{};
};
}  // namespace pal4
