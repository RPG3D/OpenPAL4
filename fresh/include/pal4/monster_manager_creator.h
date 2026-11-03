#pragma once

#include "pal4/mode_string_buffer.h"

#include <cstdint>

namespace pal4 {

struct MonsterCreationArguments {
    std::uint32_t name{};
    std::uint32_t config{};
    std::uint32_t position{};
    std::uint32_t facing{};
    std::uint32_t source_record{};
    std::uint32_t enabled{};
};

class IMonsterManagerCreateRuntime {
public:
    virtual ~IMonsterManagerCreateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t address) = 0;
    virtual void SetErrorFlag(ModeStringBufferRecord& text,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(
        ModeStringBufferRecord& text, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    virtual void SetTextSize(ModeStringBufferRecord& text,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t SearchNode(
        std::uint32_t tree, const ModeStringBufferRecord& name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void FreeTextHeader(std::uint32_t address) = 0;
    virtual void LoadScripts() = 0;
    virtual void CleanupScripts() = 0;
    [[nodiscard]] virtual std::uint32_t AllocateObject(
        std::uint32_t bytes) = 0;
    virtual void FreeObjectStorage(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructObject(
        std::uint32_t storage) = 0;
    virtual void CallSlot30(std::uint32_t object, std::uint32_t name,
                            std::uint32_t config,
                            std::uint32_t enabled) = 0;
    virtual void CallSlot4(std::uint32_t object,
                           std::uint32_t value,
                           std::uint32_t zero) = 0;
    virtual void CallSlot8(std::uint32_t object,
                           std::uint32_t value,
                           std::uint32_t zero) = 0;
    virtual void SetStateFlag(std::uint32_t object,
                              std::uint32_t flag) = 0;
    virtual void AssignName(ModeStringBufferRecord& destination,
                            const ModeStringBufferRecord& source,
                            std::uint32_t start,
                            std::uint32_t count) = 0;
    virtual void InsertNode(std::uint32_t tree,
                            const ModeStringBufferRecord& name,
                            std::uint32_t object) = 0;
    virtual void InitializeComponent(std::uint32_t object,
                                     std::uint32_t source_record,
                                     std::uint32_t dispatch_fields) = 0;
};

class MonsterManagerCreateRuntimeStub : public IMonsterManagerCreateRuntime {
public:
    std::uint32_t CStringLength(std::uint32_t) override;
    void SetErrorFlag(ModeStringBufferRecord&, std::uint8_t) override;
    bool AllocateText(ModeStringBufferRecord&, std::uint32_t,
                      std::uint8_t) override;
    void CopyBytes(std::uint32_t, std::uint32_t,
                   std::uint32_t) override;
    void SetTextSize(ModeStringBufferRecord&,
                     std::uint32_t) override;
    std::uint32_t SearchNode(std::uint32_t,
                             const ModeStringBufferRecord&) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void FreeTextHeader(std::uint32_t) override;
    void LoadScripts() override;
    void CleanupScripts() override;
    std::uint32_t AllocateObject(std::uint32_t) override;
    void FreeObjectStorage(std::uint32_t) override;
    std::uint32_t ConstructObject(std::uint32_t) override;
    void CallSlot30(std::uint32_t, std::uint32_t,
                    std::uint32_t, std::uint32_t) override;
    void CallSlot4(std::uint32_t, std::uint32_t,
                   std::uint32_t) override;
    void CallSlot8(std::uint32_t, std::uint32_t,
                   std::uint32_t) override;
    void SetStateFlag(std::uint32_t, std::uint32_t) override;
    void AssignName(ModeStringBufferRecord&,
                    const ModeStringBufferRecord&,
                    std::uint32_t, std::uint32_t) override;
    void InsertNode(std::uint32_t,
                    const ModeStringBufferRecord&,
                    std::uint32_t) override;
    void InitializeComponent(std::uint32_t, std::uint32_t,
                             std::uint32_t) override;
};

class MonsterManagerCreator final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x73cu;
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t manager, const MonsterCreationArguments& arguments,
        IMonsterManagerCreateRuntime& runtime);
};

}  // namespace pal4
