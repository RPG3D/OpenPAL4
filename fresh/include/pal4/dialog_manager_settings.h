#pragma once

#include "pal4/dialog_manager_lifecycle.h"
#include "pal4/global_data_settings.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace pal4 {

class IDialogManagerSettingsWriteRuntime :
    public IGlobalDataSettingsSaveRuntime {
public:
    virtual void ClearData(DialogManagerState& state) = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(
        const char* path, const char* mode) = 0;
    virtual void InitializeFileStream(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t CreateOutputArchive(
        std::uint32_t file) = 0;
    virtual void WriteFlagString(std::uint32_t archive,
                                 const std::string& flags) = 0;
    [[nodiscard]] virtual std::uint32_t FirstNode(
        const DialogManagerState& state, std::size_t tree_offset) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(
        std::uint32_t node, bool primary_tree) = 0;
    [[nodiscard]] virtual std::uint32_t NodeValue(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t ReadArrayWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXml(const char* root) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t xml, const std::string& name) = 0;
    virtual void SetAttribute(std::uint32_t attribute,
                              std::uint32_t value) = 0;
    virtual void AddAttribute(std::uint32_t xml, const std::string& name,
                              std::uint32_t value) = 0;
    virtual void WriteConfig(std::uint32_t xml,
                             std::uint32_t archive) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
    virtual void DestroyXml(std::uint32_t xml) = 0;
    virtual void DestroyArchive(std::uint32_t archive) = 0;
};

class IDialogManagerSettingsReadRuntime : public IGlobalDataSettingsRuntime {
public:
    virtual void Reset(DialogManagerState& state) = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(
        const char* path, const char* mode) = 0;
    virtual void InitializeFileStream(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t CreateInputArchive(
        std::uint32_t file) = 0;
    [[nodiscard]] virtual std::string ReadFlagString(
        std::uint32_t archive) = 0;
    virtual void ClearTree(DialogManagerState& state,
                           std::size_t tree_offset) = 0;
    virtual void InsertTreeValue(DialogManagerState& state,
                                 std::size_t tree_offset,
                                 std::uint32_t value) = 0;
    virtual void WriteArrayWord(std::uint32_t address,
                                std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXml(const char* root) = 0;
    [[nodiscard]] virtual bool XmlVersionMatches(
        std::uint32_t xml, std::uint32_t archive) = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> XmlInteger(
        std::uint32_t xml, const std::string& name) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
    virtual void DestroyXml(std::uint32_t xml) = 0;
    virtual void DestroyArchive(std::uint32_t archive) = 0;
};

class DialogManagerSettings final {
public:
    // IDA 0x5ce6e0.
    [[nodiscard]] static std::string FlagsToString(
        const DialogManagerState& state);
    // IDA 0x5cd570.
    static void Save(DialogManagerState& state,
                     IDialogManagerSettingsWriteRuntime& runtime);
    // IDA 0x5cdaa0.
    static void Load(DialogManagerState& state,
                     IDialogManagerSettingsReadRuntime& runtime);
};

}  // namespace pal4
