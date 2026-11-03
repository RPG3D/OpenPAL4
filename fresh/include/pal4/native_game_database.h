#pragma once

#include "pal4/game_data_file_loader.h"
#include "pal4/game_db_archive_reader.h"
#include "pal4/native_game_database_source.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace pal4 {

struct NativeGameDbProperty {
    std::string name;
    std::string label;
    std::vector<std::pair<std::int32_t, std::string>> entries;
};

struct NativeGameDbField {
    std::string name;
    std::uint32_t type = 0u;
    std::string label;
    std::string detail;
};

using NativeGameDbValue = std::variant<std::uint32_t, std::string, float>;

struct NativeGameDbTable {
    std::string name;
    std::string description;
    std::uint32_t attribute = 0u;
    std::vector<NativeGameDbField> fields;
    std::vector<std::vector<NativeGameDbValue>> rows;
};

struct NativeRoleRecordView {
    std::uint32_t id;
    std::string_view name;
    std::string_view icon;
    std::string_view model;
    std::string_view texture;
    std::span<const NativeGameDbValue> fields;
};

class NativeGameDatabase final : private IGameDataFileLoaderRuntime,
                                 private IGameDbArchiveTableRuntime {
public:
    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view member = "pal4db.db");
    [[nodiscard]] const auto& Properties() const { return properties_; }
    [[nodiscard]] const auto& Tables() const { return tables_; }
    [[nodiscard]] std::optional<std::string_view> FindTableTextById(
        std::string_view table_name, std::uint32_t id) const;
    // IDA 0x4e9930 uses player index + 1001 as the Role record key.
    // Returned views remain valid until the next Load call.
    [[nodiscard]] std::optional<NativeRoleRecordView> FindRoleByPlayerIndex(
        std::uint32_t index) const;
    [[nodiscard]] std::size_t BytesConsumed() const { return source_.Position(); }
    [[nodiscard]] std::size_t BytesTotal() const { return source_.Size(); }
    [[nodiscard]] std::string_view LastError() const { return error_; }

private:
    struct TextAllocation {
        std::unique_ptr<char[]> bytes;
        std::uint32_t capacity;
    };

    [[nodiscard]] bool LoadBootstrapEnabled() override { return false; }
    [[nodiscard]] std::uint32_t LoadBootstrapScript(std::string_view) override;
    void ProcessBootstrapScript(std::uint32_t) override;
    void CleanupAllScripts() override;
    void DestroyBootstrapScript(std::uint32_t) override;
    [[nodiscard]] std::uint32_t OpenFile(const char*, std::string_view) override;
    void CloseFile(std::uint32_t) override;
    [[nodiscard]] std::uint32_t CreateStream(std::uint32_t) override;
    void DestroyStream(std::uint32_t) override;
    [[nodiscard]] std::string ReadText(std::uint32_t) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override;
    [[nodiscard]] std::string_view ExpectedSignature() override;
    [[nodiscard]] bool LoadScriptEntry(std::string_view, std::uint32_t,
                                      std::uint32_t, std::uint32_t) override;
    [[nodiscard]] bool LoadTableEntry(std::string_view, std::uint32_t,
                                     std::uint32_t, std::uint32_t) override;
    void RemoveTableEntry(std::uint32_t, std::string_view) override;

    [[nodiscard]] std::uint32_t InvokeVirtual(
        std::uint32_t, std::uint32_t, std::uintptr_t,
        std::uint32_t) override;
    [[nodiscard]] const char* EmptyData() override { return ""; }
    [[nodiscard]] bool AllocateText(StringBufferState&, std::uint32_t,
                                    std::uint8_t) override;
    void SetErrorFlag(StringBufferState&, std::uint8_t) override;
    void Reallocate(StringBufferState&, std::uint32_t) override;
    void SetTextSize(StringBufferState&, std::uint32_t) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    void Copy(std::uint32_t, const char*, std::uint32_t) override;
    [[nodiscard]] std::uint32_t ConstructProperty(const char*) override;
    [[nodiscard]] const char* TextData(const StringBufferState&) override;
    void SetPropertyLabel(std::uint32_t, const char*) override;
    void AddEntry(std::uint32_t, std::int32_t, const char*) override;
    void InsertProperty(std::uint32_t, std::uint32_t) override;
    void ReleaseText(StringBufferState&) override;
    void DestroyProperty(std::uint32_t) override;
    void LogBegin(const char*) override;
    void LogEnd(const char*) override;
    [[nodiscard]] std::uint32_t FindOrLoad(std::uint32_t,
                                           const char*) override;
    void RegisterObject(std::uint32_t, const char*) override;
    [[nodiscard]] bool NamesDiffer(const char*, const char*) override;
    void SetDescription(std::uint32_t, const char*) override;
    void ReserveSchema(std::uint32_t) override;
    void AddSchemaField(const char*, std::uint32_t,
                        const char*, const char*) override;
    void AssignSchema(std::uint32_t, std::uint32_t,
                      std::uint32_t) override;
    void PrepareObject(std::uint32_t) override;
    void InitializeRows(std::uint32_t) override;
    void AddDefaultValue() override;
    void ResetValue() override;
    void SetInteger(std::uint32_t) override;
    void SetText(const char*) override;
    void SetFloat(float) override;
    void CommitValue(std::uint32_t) override;
    void CommitRow(std::uint32_t) override;
    void FinalizeObject(std::uint32_t) override;
    void CleanupRows() override;
    void CleanupSchema() override;

    [[nodiscard]] TextAllocation& AllocationAt(std::uint32_t,
                                               std::uint32_t);
    [[nodiscard]] NativeGameDbTable& TableAt(std::uint32_t);

    std::filesystem::path root_;
    std::string member_;
    NativeGameDatabaseSource source_;
    std::string error_;
    std::unordered_map<std::string, NativeGameDbProperty> properties_;
    std::unordered_map<std::string, NativeGameDbTable> tables_;
    std::unordered_map<std::uint32_t, NativeGameDbProperty> pending_properties_;
    std::unordered_map<std::string, std::uint32_t> table_ids_;
    std::unordered_map<std::uint32_t, std::string> table_names_;
    std::unordered_map<std::uint32_t, TextAllocation> text_allocations_;
    std::vector<NativeGameDbField> schema_;
    std::vector<NativeGameDbValue> row_;
    NativeGameDbValue value_ = std::uint32_t{};
    std::string active_record_;
    std::uint32_t next_id_ = 1u;
    bool file_open_ = false;
    bool stream_open_ = false;
};

}  // namespace pal4
