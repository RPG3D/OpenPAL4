#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct GlobalSystemBuffer {
    std::uint32_t marker{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity_end{};
};
static_assert(sizeof(GlobalSystemBuffer) == 0x10u);

struct GlobalSystemCollection {
    GlobalSystemBuffer entries;
    GlobalSystemBuffer processed;
    GlobalSystemBuffer sorted;
};
static_assert(sizeof(GlobalSystemCollection) == 0x30u);

struct GlobalSystemText {
    std::uint32_t marker{};
    StringBufferState buffer{};
};
static_assert(sizeof(GlobalSystemText) == 0x10u);

class IGlobalSystemRuntime {
public:
    virtual ~IGlobalSystemRuntime() = default;
    virtual void RegisterCleanup() = 0;
    virtual void SetErrorFlag(GlobalSystemText& text,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateEntry(
        std::uint32_t bytes) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void InitializeEntryText(std::uint32_t address,
                                     std::uint8_t incoming_byte) = 0;
    virtual void SetText(GlobalSystemText& text,
                         std::string_view value) = 0;
    virtual void AssignSourceText(GlobalSystemText& text,
                                  std::uint32_t source_address) = 0;
    virtual void AppendEmptyErrorString(GlobalSystemText& text) = 0;
    virtual void AssignEntryText(std::uint32_t address,
                                 const GlobalSystemText& text) = 0;
    virtual void PushBack(GlobalSystemCollection& collection,
                          std::uint32_t entry) = 0;
    [[nodiscard]] virtual std::uint32_t GameDataCollection(
        std::uint32_t member_offset) = 0;
    [[nodiscard]] virtual std::uint32_t GameDataObjectCollection(
        std::uint32_t member_offset) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEventCollection(
        std::uint32_t member_offset) = 0;
    virtual void ProcessAll(GlobalSystemCollection& collection,
                            std::uint32_t source) = 0;
    virtual void Sort(GlobalSystemCollection& collection) = 0;
    virtual void ReleaseText(GlobalSystemText& text) = 0;
    virtual void DestroyUnlinkedEntry(std::uint32_t entry) = 0;
    virtual void CleanupCollection(GlobalSystemCollection& collection) = 0;
};

class GlobalSystemRuntimeStub : public IGlobalSystemRuntime {
public:
    void RegisterCleanup() override;
    void SetErrorFlag(GlobalSystemText&, std::uint8_t) override;
    std::uint32_t AllocateEntry(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void InitializeEntryText(std::uint32_t, std::uint8_t) override;
    void SetText(GlobalSystemText&, std::string_view) override;
    void AssignSourceText(GlobalSystemText&, std::uint32_t) override;
    void AppendEmptyErrorString(GlobalSystemText&) override;
    void AssignEntryText(std::uint32_t,
                         const GlobalSystemText&) override;
    void PushBack(GlobalSystemCollection&, std::uint32_t) override;
    std::uint32_t GameDataCollection(std::uint32_t) override;
    std::uint32_t GameDataObjectCollection(std::uint32_t) override;
    std::uint32_t ScriptEventCollection(std::uint32_t) override;
    void ProcessAll(GlobalSystemCollection&, std::uint32_t) override;
    void Sort(GlobalSystemCollection&) override;
    void ReleaseText(GlobalSystemText&) override;
    void DestroyUnlinkedEntry(std::uint32_t) override;
    void CleanupCollection(GlobalSystemCollection&) override;
};

class GlobalSystem final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8e4348u;
    static constexpr std::uint32_t kEntryVtable = 0x843368u;
    static constexpr std::uint32_t kBaseVtable = 0x843380u;
    static constexpr std::uint32_t kSourceTextArray = 0x8ef3a8u;

    [[nodiscard]] GlobalSystemCollection& Initialize(
        std::uint8_t first_stack_byte, std::uint8_t second_stack_byte,
        IGlobalSystemRuntime& runtime);
    void Cleanup(IGlobalSystemRuntime& runtime);

    [[nodiscard]] const GlobalSystemCollection& Collection() const {
        return collection_;
    }

private:
    GlobalSystemCollection collection_{};
    std::uint8_t construction_guard_{};
    std::uint8_t entries_guard_{};
};

}  // namespace pal4
