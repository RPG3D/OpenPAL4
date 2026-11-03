#include "pal4/portable/combat_translation_catalog.h"

#include "pal4/archive_stream_adapter.h"
#include "pal4/archive_text_reader.h"
#include "pal4/game_data_file_loader.h"
#include "pal4/game_db_archive_reader.h"
#include "pal4/portable/package.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <variant>

namespace pal4::portable {
namespace {
constexpr std::uint32_t kFile = 1u;
constexpr std::uint32_t kStream = 1u;
constexpr std::uint32_t kDatabase = 1u;
constexpr std::uint32_t kMaxFieldCount = 4096u;
constexpr std::uint32_t kMaxRowCount = 100000u;
constexpr std::uint64_t kMaxCells = 1000000u;
constexpr std::uint32_t kMaxTextBytes = 1024u * 1024u;

// Logical handles keep the shared parser independent of host pointer width.
struct NativeField final {
    std::string name;
    std::uint32_t type{};
};
using NativeValue = std::variant<std::uint32_t, std::string, float>;

struct Table final {
    std::string name;
    std::string description;
    std::vector<NativeField> fields;
    std::vector<std::vector<NativeValue>> rows;
    std::uint32_t expected_rows{};
    bool finalized{};
};

class DatabaseReader final : public IGameDataFileLoaderRuntime,
                             public IGameDbArchiveTableRuntime {
public:
    explicit DatabaseReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

    bool LoadBootstrapEnabled() override { return false; }
    std::uint32_t LoadBootstrapScript(std::string_view) override { return 0u; }
    void ProcessBootstrapScript(std::uint32_t) override {}
    void CleanupAllScripts() override {}
    void DestroyBootstrapScript(std::uint32_t) override {}

    std::uint32_t OpenFile(const char* path, std::string_view mode) override {
        if (file_open_ || !path || mode != "rb" ||
            std::string_view(path).find("combatDb.db") == std::string_view::npos)
            return 0u;
        file_open_ = true;
        return kFile;
    }
    void CloseFile(std::uint32_t file) override {
        if (file != kFile || !file_open_) throw std::logic_error("Combat DB file owner");
        file_open_ = false;
    }
    std::uint32_t CreateStream(std::uint32_t file) override {
        if (!file_open_ || file != kFile || stream_open_)
            return 0u;
        stream_open_ = true;
        return kStream;
    }
    void DestroyStream(std::uint32_t stream) override {
        if (stream != kStream || !stream_open_)
            throw std::logic_error("Combat DB stream owner");
        stream_open_ = false;
    }
    std::string ReadText(std::uint32_t stream) override {
        RequireStream(stream);
        ArchiveStreamAdapter adapter(*this);
        ArchiveTextReader reader(adapter, *this);
        StringBufferState text{};
        if (!reader.Read(StreamState(), text))
            throw std::runtime_error("Combat DB text record is truncated or invalid");
        const auto result = TextData(text);
        std::string copy(result);
        ReleaseText(text);
        return copy;
    }
    std::uint32_t ReadWord(std::uint32_t stream) override {
        RequireStream(stream);
        std::uint8_t bytes[4]{};
        if (!ReadExact(bytes, sizeof(bytes)))
            throw std::runtime_error("Combat DB integer is truncated");
        return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8u) |
            (std::uint32_t(bytes[2]) << 16u) | (std::uint32_t(bytes[3]) << 24u);
    }
    std::string_view ExpectedSignature() override { return "GAME_DB_FLAG"; }
    bool LoadScriptEntry(std::string_view name, std::uint32_t,
                         std::uint32_t, std::uint32_t) override {
        if (++script_count_ > kMaxFieldCount)
            throw std::length_error("Combat DB property count limit");
        const std::string expected(name);
        return GameDbArchiveReader::ReadPropertyRecord(
            expected.c_str(), kDatabase, StreamState(), *this);
    }
    bool LoadTableEntry(std::string_view name, std::uint32_t,
                        std::uint32_t, std::uint32_t) override {
        if (++table_count_ > kMaxFieldCount)
            throw std::length_error("Combat DB table count limit");
        const std::string expected(name);
        return GameDbArchiveReader::ReadTable(
            expected.c_str(), static_cast<std::uint32_t>(tables_.size()),
            StreamState(), *this);
    }
    void RemoveTableEntry(std::uint32_t, std::string_view name) override {
        const auto found = table_ids_.find(std::string(name));
        if (found != table_ids_.end()) {
            tables_.erase(found->second);
            table_ids_.erase(found);
        }
    }

    std::uint32_t InvokeVirtual(std::uint32_t stream, std::uint32_t slot,
                                std::uintptr_t output,
                                std::uint32_t count) override {
        if (!stream_open_ || stream != kStream || slot != 12u ||
            count > kMaxTextBytes || position_ > bytes_.size() ||
            count > bytes_.size() - position_)
            return 0u;
        if (count) std::memcpy(reinterpret_cast<void*>(output), bytes_.data() + position_, count);
        position_ += count;
        return 1u;
    }

    const char* EmptyData() override { return ""; }
    bool AllocateText(StringBufferState& text, std::uint32_t length,
                      std::uint8_t) override {
        if (!length) {
            ReleaseText(text);
            return false;
        }
        Reallocate(text, length);
        return true;
    }
    void SetErrorFlag(StringBufferState& text, std::uint8_t) override {
        ReleaseText(text);
    }
    void Reallocate(StringBufferState& text, std::uint32_t length) override {
        if (length > kMaxTextBytes) throw std::length_error("Combat DB text limit");
        ReleaseText(text);
        const auto base = next_text_handle_;
        const auto allocation_size = length + 2u;
        next_text_handle_ += ((allocation_size + 15u) & ~15u) + 16u;
        if (!next_text_handle_) throw std::overflow_error("Combat DB text handles exhausted");
        TextAllocation allocation;
        allocation.bytes.resize(allocation_size);
        text_allocations_.emplace(base, std::move(allocation));
        text.data = base + 1u;
        text.length = 0u;
        text.capacity = length;
    }
    void SetTextSize(StringBufferState& text, std::uint32_t length) override {
        text.length = length;
        WriteByte(text.data + length, 0u);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        const auto [allocation, offset] = TextAt(address);
        return allocation.bytes[offset];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        const auto [allocation, offset] = TextAt(address);
        allocation.bytes[offset] = value;
    }
    void Copy(std::uint32_t destination, const char* source,
              std::uint32_t length) override {
        auto [allocation, offset] = TextAt(destination);
        if (length > allocation.bytes.size() - offset)
            throw std::out_of_range("Combat DB text copy");
        std::memcpy(allocation.bytes.data() + offset, source, length);
    }

    std::uint32_t ConstructProperty(const char*) override { return next_property_++; }
    const char* TextData(const StringBufferState& text) override {
        if (!text.data) return "";
        auto [allocation, offset] = TextAt(text.data);
        (void)offset;
        return reinterpret_cast<const char*>(allocation.bytes.data() + 1u);
    }
    void SetPropertyLabel(std::uint32_t, const char*) override {}
    void AddEntry(std::uint32_t, std::int32_t, const char*) override {}
    void InsertProperty(std::uint32_t, std::uint32_t) override {}
    void ReleaseText(StringBufferState& text) override {
        if (text.data) text_allocations_.erase(text.data - 1u);
        text = {};
    }
    void DestroyProperty(std::uint32_t) override {}

    void LogBegin(const char*) override {}
    void LogEnd(const char*) override {}
    std::uint32_t FindOrLoad(std::uint32_t index, const char* name) override {
        const auto found = tables_.find(index + 1u);
        if (found == tables_.end() || found->second.name != name) return 0u;
        return index + 1u;
    }
    void RegisterObject(std::uint32_t index, const char* name) override {
        if (index + 1u == 0u || tables_.contains(index + 1u) ||
            table_ids_.contains(name))
            throw std::logic_error("duplicate Combat DB table index");
        Table table;
        table.name = name;
        tables_.emplace(index + 1u, std::move(table));
        table_ids_.emplace(name, index + 1u);
    }
    bool NamesDiffer(const char* actual, const char* expected) override {
        return std::string_view(actual) != expected;
    }
    void SetDescription(std::uint32_t object, const char* description) override {
        At(object).description = description;
    }
    void ReserveSchema(std::uint32_t count) override {
        if (count > kMaxFieldCount) throw std::length_error("Combat DB schema limit");
        schema_.clear();
        schema_.reserve(count);
    }
    void AddSchemaField(const char* name, std::uint32_t type,
                        const char*, const char*) override {
        if (type < 1u || type > 3u || schema_.size() >= kMaxFieldCount)
            throw std::runtime_error("Combat DB field schema invalid");
        schema_.push_back({name, type});
    }
    void AssignSchema(std::uint32_t object, std::uint32_t attribute,
                      std::uint32_t row_count) override {
        auto& table = At(object);
        if (row_count > kMaxRowCount ||
            std::uint64_t(row_count) * schema_.size() > kMaxCells)
            throw std::length_error("Combat DB row limit");
        table.fields = schema_;
        table.expected_rows = row_count;
        table.finalized = false;
        (void)attribute;
    }
    void PrepareObject(std::uint32_t object) override { (void)At(object); }
    void InitializeRows(std::uint32_t field_count) override {
        if (field_count != schema_.size())
            throw std::logic_error("Combat DB row/schema mismatch");
        row_.assign(field_count, std::uint32_t{});
    }
    void AddDefaultValue() override {}
    void ResetValue() override { value_ = std::uint32_t{}; }
    void SetInteger(std::uint32_t value) override { value_ = value; }
    void SetText(const char* value) override { value_ = std::string(value); }
    void SetFloat(float value) override { value_ = value; }
    void CommitValue(std::uint32_t field) override {
        if (field >= row_.size()) throw std::out_of_range("Combat DB row field");
        row_[field] = value_;
    }
    void CommitRow(std::uint32_t object) override {
        auto& table = At(object);
        if (table.rows.size() >= table.expected_rows ||
            row_.size() != table.fields.size())
            throw std::logic_error("Combat DB row count mismatch");
        table.rows.push_back(row_);
    }
    void FinalizeObject(std::uint32_t object) override {
        auto& table = At(object);
        if (table.rows.size() != table.expected_rows)
            throw std::logic_error("Combat DB table is incomplete");
        table.finalized = true;
    }
    void CleanupRows() override { row_.clear(); }
    void CleanupSchema() override { schema_.clear(); }

    [[nodiscard]] std::size_t Position() const noexcept { return position_; }
    [[nodiscard]] const Table& TranslationTable() const {
        const auto found = table_ids_.find("CombatTranslation");
        if (found == table_ids_.end())
            throw std::runtime_error("combatDb.db has no CombatTranslation table");
        const auto& table = tables_.at(found->second);
        if (!table.finalized || table.fields.size() != 2u ||
            table.fields[0].name != "English" || table.fields[0].type != 2u ||
            table.fields[1].name != "Translation" || table.fields[1].type != 2u)
            throw std::runtime_error("CombatTranslation schema mismatch");
        for (const auto& row : table.rows)
            if (row.size() != 2u ||
                !std::holds_alternative<std::string>(row[0]) ||
                !std::holds_alternative<std::string>(row[1]))
                throw std::runtime_error("CombatTranslation row mismatch");
        return table;
    }
    [[nodiscard]] bool AtEnd() const noexcept { return position_ == bytes_.size(); }

private:
    struct TextAllocation { std::vector<std::uint8_t> bytes; };
    using TextReference = std::pair<TextAllocation&, std::size_t>;

    void RequireStream(std::uint32_t stream) const {
        if (!stream_open_ || stream != kStream)
            throw std::logic_error("Combat DB stream is not open");
    }
    bool ReadExact(void* destination, std::size_t count) {
        if (position_ > bytes_.size() || count > bytes_.size() - position_)
            return false;
        std::memcpy(destination, bytes_.data() + position_, count);
        position_ += count;
        return true;
    }
    TextReference TextAt(std::uint32_t address) {
        for (auto& [base, allocation] : text_allocations_) {
            if (address >= base && address - base < allocation.bytes.size())
                return {allocation, address - base};
        }
        throw std::out_of_range("Combat DB logical text handle");
    }
    Table& At(std::uint32_t object) {
        const auto found = tables_.find(object);
        if (found == tables_.end()) throw std::invalid_argument("Combat DB table handle");
        return found->second;
    }
    ArchiveStreamAdapterState StreamState() const { return {kStream, 1u, {}}; }

    std::span<const std::uint8_t> bytes_;
    std::size_t position_{};
    std::uint32_t next_text_handle_{0x10000u}, next_property_{1u};
    std::uint32_t script_count_{}, table_count_{};
    bool file_open_{}, stream_open_{};
    std::unordered_map<std::uint32_t, TextAllocation> text_allocations_;
    std::unordered_map<std::uint32_t, Table> tables_;
    std::unordered_map<std::string, std::uint32_t> table_ids_;
    std::vector<NativeField> schema_;
    std::vector<NativeValue> row_;
    NativeValue value_{std::uint32_t{}};
};
} // namespace

CombatTranslationCatalog CombatTranslationCatalog::ReadFromGameData(
    const std::filesystem::path& game_data) {
    std::ifstream input(game_data / "database.cpk", std::ios::binary);
    if (!input) throw std::runtime_error("cannot open original database.cpk");
    PackageReader package(input);
    const auto bytes = package.Read("combatDb.db", 32u * 1024u * 1024u);
    return ReadDatabase(bytes);
}

CombatTranslationCatalog CombatTranslationCatalog::ReadDatabase(
    std::span<const std::uint8_t> database) {
    if (database.empty() || database.size() > 32u * 1024u * 1024u)
        throw std::runtime_error("invalid combatDb.db size");
    DatabaseReader runtime(database);
    if (!GameDataFileLoader::Load("gamedata\\database\\combatDb.db",
                                  kDatabase, 0u, runtime))
        throw std::runtime_error("combatDb.db header or table parse failed");
    if (!runtime.AtEnd()) throw std::runtime_error("combatDb.db has trailing data");

    CombatTranslationCatalog result;
    const auto& table = runtime.TranslationTable();
    result.rows_.reserve(table.rows.size());
    for (const auto& row : table.rows)
        result.rows_.emplace_back(std::get<std::string>(row[0]),
                                  std::get<std::string>(row[1]));
    return result;
}

std::optional<std::string_view> CombatTranslationCatalog::Find(
    std::string_view english) const noexcept {
    const auto found = std::find_if(rows_.begin(), rows_.end(),
        [english](const auto& row) { return row.first == english; });
    if (found == rows_.end()) return std::nullopt;
    return found->second;
}

std::size_t CombatTranslationCatalog::Size() const noexcept { return rows_.size(); }

} // namespace pal4::portable
