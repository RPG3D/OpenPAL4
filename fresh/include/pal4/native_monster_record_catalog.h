#pragma once

#include "pal4/native_game_database.h"
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace pal4 {

// IDA 4DE7E0/4DEA90: Monster records contain a 336-byte property prefix
// and a 260-byte extension. Text buffers are owned here and marked non-shareable
// at the existing rewritten x86 copy boundary; no VC6 allocation/COW is used.
class NativeMonsterRecordCatalog final {
public:
    using Words = std::array<std::uint32_t, 149u>;
    explicit NativeMonsterRecordCatalog(const NativeGameDatabase& database);
    NativeMonsterRecordCatalog(const NativeMonsterRecordCatalog&) = delete;
    NativeMonsterRecordCatalog& operator=(const NativeMonsterRecordCatalog&) = delete;

    [[nodiscard]] const std::vector<std::string>& UnboundFields() const noexcept { return unbound_fields_; }
    [[nodiscard]] std::size_t Count() const noexcept { return records_.size(); }
    [[nodiscard]] std::uint32_t Find(std::int32_t id) const noexcept;
    [[nodiscard]] const Words& Record(std::size_t index) const { return records_.at(index); }
    [[nodiscard]] bool Owns(std::uint32_t record) const noexcept;

private:
    using Texts = std::array<std::unique_ptr<std::uint8_t[]>, 11u>;
    std::vector<std::string> unbound_fields_;
    std::vector<Words> records_;
    std::vector<Texts> texts_;
    std::vector<std::pair<std::int32_t, std::size_t>> index_;
};

} // namespace pal4
