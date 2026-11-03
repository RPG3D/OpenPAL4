#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGetItemTaskRuntime {
public:
    virtual ~IGetItemTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestruct(
        std::uint32_t object) = 0;
    virtual void FreeMemory(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t BeginText() = 0;
    virtual void ReleaseText(std::uint32_t text) = 0;
    [[nodiscard]] virtual bool HasItem(std::uint32_t id) = 0;
    [[nodiscard]] virtual bool HasEquipment(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string_view ItemName(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string_view EquipmentName(
        std::uint32_t id) = 0;
    virtual void GrantItem(std::uint32_t id,
                           std::uint32_t count) = 0;
    virtual void GrantEquipment(std::uint32_t id) = 0;
    virtual void AddMoney(std::uint32_t amount) = 0;
    [[nodiscard]] virtual std::string_view GameString(
        std::uint32_t id) = 0;
    virtual void FormatText(std::uint32_t text,
                            std::string_view format,
                            std::string_view name,
                            std::uint32_t count) = 0;
    virtual void AppendText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void PlaySound(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow() = 0;
    virtual void ShowHint(std::uint32_t window,
                          std::uint32_t text, float x, float y,
                          float opacity, bool first, bool second) = 0;
    [[nodiscard]] virtual bool HintVisible(
        std::uint32_t window) = 0;
};

class IGetItemTaskSpawnRuntime : public virtual IGetItemTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructHelperTask(
        std::uint32_t memory) = 0;
    virtual void AddHelperTask(std::uint32_t context,
                               std::uint32_t helper,
                               std::uint32_t first_timing,
                               std::uint32_t second_timing,
                               std::uint32_t third_timing) = 0;
    virtual void SetHelperOwner(std::uint32_t helper,
                                std::uint32_t owner) = 0;
    virtual void NotifyPreviousTask(std::uint32_t previous) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class GetItemTask final {
public:
    static constexpr std::uint32_t kVtable = 0x848810u;
    // IDA 0x595a20/0x595a40/0x595a60.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGetItemTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IGetItemTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        IGetItemTaskRuntime& runtime);
    // IDA 0x595a80/0x595ce0.
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IGetItemTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, IGetItemTaskRuntime& runtime);
    // IDA 0x595d30/0x595f60/0x595f70.
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, IGetItemTaskSpawnRuntime& runtime);
    [[nodiscard]] static std::uint8_t IsComplete(
        std::uint32_t object, IGetItemTaskRuntime& runtime);
    static void Initialize(std::uint32_t object,
                           std::uint32_t owner, std::uint32_t kind,
                           std::uint32_t id, std::uint32_t amount,
                           std::uint32_t context,
                           IGetItemTaskRuntime& runtime);
};

}  // namespace pal4
