#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobMachineSnapshotRuntime {
public:
    virtual ~IGobMachineSnapshotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InitializeStringRecord(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseStringRecord(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadStringRecord(
        std::uint32_t record) = 0;
    virtual void AssignStringRecord(std::uint32_t record,
                                    std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t SnapshotType(
        std::uint32_t snapshot) = 0;
    virtual void SetVisible(std::uint32_t object,
                            std::uint32_t visible) = 0;
    virtual void SetPosition(std::uint32_t object,
                             std::uint32_t vector, std::uint32_t mode) = 0;
    virtual void SetRotation(std::uint32_t object,
                             std::uint32_t vector, std::uint32_t mode) = 0;
    virtual void SelectAnimation(std::uint32_t state,
                                 std::string_view name) = 0;
    [[nodiscard]] virtual float AnimationLength(
        std::uint32_t state) = 0;
    virtual void SetAnimationTime(std::uint32_t state,
                                  float time) = 0;
    [[nodiscard]] virtual std::uint32_t ClearAnimationFlag(
        std::uint32_t state, std::uint32_t flag) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlDocument(
        std::string_view root) = 0;
    virtual void WriteXmlString(std::uint32_t document,
                                std::string_view key,
                                std::string_view value) = 0;
    virtual void WriteXmlInt(std::uint32_t document,
                             std::string_view key,
                             std::int32_t value) = 0;
    virtual void WriteXmlFloat(std::uint32_t document,
                               std::string_view key, float value) = 0;
    virtual void FlushXml(std::uint32_t document,
                          std::uint32_t stream) = 0;
    [[nodiscard]] virtual bool ReadXml(std::uint32_t document,
                                       std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::string ReadXmlString(
        std::uint32_t document, std::string_view key,
        std::string_view fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadXmlInt(
        std::uint32_t document, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadXmlFloat(
        std::uint32_t document, std::string_view key,
        float fallback) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyXmlDocument(
        std::uint32_t document) = 0;
};

class GobMachineStateSnapshot final {
public:
    static constexpr std::uint32_t kSize = 0x50u;
    static constexpr std::uint32_t kVtable = 0x84901cu;
    static constexpr std::uint32_t kBaseVtable = 0x841e40u;

    [[nodiscard]] static std::uint32_t Create(
        IGobMachineSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t snapshot, IGobMachineSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t snapshot, IGobMachineSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t WriteConfig(
        std::uint32_t snapshot, std::uint32_t stream,
        IGobMachineSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadConfig(
        std::uint32_t snapshot, std::uint32_t stream,
        IGobMachineSnapshotRuntime& runtime);
};

}  // namespace pal4
