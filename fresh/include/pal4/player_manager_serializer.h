#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlayerSerializationFlag {
    std::uint32_t data{};
    std::uint32_t length{};
};

struct PlayerAttributeCursor {
    std::uint32_t node{};
    std::uint32_t end{};
};

class IPlayerManagerSerializeRuntime {
public:
    virtual ~IPlayerManagerSerializeRuntime() = default;
    virtual void InitializeFlag(PlayerSerializationFlag& flag,
                                std::uint8_t initial_byte) = 0;
    [[nodiscard]] virtual bool AllocateFlag(PlayerSerializationFlag& flag,
                                            std::uint32_t length,
                                            std::uint8_t mode) = 0;
    virtual void CopyFlagText(PlayerSerializationFlag& flag,
                              std::uint32_t source,
                              std::uint32_t length) = 0;
    virtual void SetFlagSize(PlayerSerializationFlag& flag,
                             std::uint32_t length) = 0;
    virtual void DataReady(std::uint32_t data,
                           const PlayerSerializationFlag& flag) = 0;
    virtual void FillFlag(std::uint32_t data,
                          PlayerSerializationFlag& flag) = 0;
    [[nodiscard]] virtual std::int32_t CompareFlag(
        const PlayerSerializationFlag& flag, std::uint32_t source,
        std::uint32_t source_length) = 0;
    virtual void ProcessPlayer(std::uint32_t slot, std::uint32_t data) = 0;
    virtual void ProcessLoadedData(std::uint32_t manager) = 0;
    virtual void LoadPlayer(std::uint32_t slot, std::uint32_t data) = 0;
    [[nodiscard]] virtual std::uint32_t CreateDocument(
        std::string_view root, std::uint32_t option) = 0;
    virtual void DestroyDocument(std::uint32_t document) = 0;
    [[nodiscard]] virtual bool CheckDocumentVersion(
        std::uint32_t document, std::uint32_t data) = 0;
    [[nodiscard]] virtual std::uint32_t FindLeadingAttribute(
        std::uint32_t document, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateAttribute(
        std::string_view name, std::uint32_t value) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    virtual void SetAttributeInt(std::uint32_t attribute,
                                 std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AttributeToInt(
        std::uint32_t attribute, std::uint32_t default_value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void SetVector(std::uint32_t document, std::uint32_t name,
                           std::uint32_t source) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 3> GetVector(
        std::uint32_t document, std::uint32_t name) = 0;
    virtual void ApplyTransform(
        std::uint32_t slot, std::uint32_t virtual_slot,
        const std::array<std::uint32_t, 3>& value,
        std::uint32_t option) = 0;
    virtual void SetPlayerState(std::uint32_t manager,
                                std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateLookupKey(
        std::string_view name, std::uint8_t& mode_byte) = 0;
    [[nodiscard]] virtual PlayerAttributeCursor FindTreeAttribute(
        std::uint32_t key) = 0;
    virtual void ResetLookupKey(std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint8_t WriteDocument(
        std::uint32_t document, std::uint32_t data) = 0;
};

class PlayerManagerSerializeRuntimeStub : public IPlayerManagerSerializeRuntime {
public:
    void InitializeFlag(PlayerSerializationFlag&, std::uint8_t) override;
    bool AllocateFlag(PlayerSerializationFlag&, std::uint32_t,
                      std::uint8_t) override;
    void CopyFlagText(PlayerSerializationFlag&, std::uint32_t,
                      std::uint32_t) override;
    void SetFlagSize(PlayerSerializationFlag&, std::uint32_t) override;
    void DataReady(std::uint32_t, const PlayerSerializationFlag&) override;
    void FillFlag(std::uint32_t, PlayerSerializationFlag&) override;
    std::int32_t CompareFlag(const PlayerSerializationFlag&,
                             std::uint32_t, std::uint32_t) override;
    void ProcessPlayer(std::uint32_t, std::uint32_t) override;
    void ProcessLoadedData(std::uint32_t) override;
    void LoadPlayer(std::uint32_t, std::uint32_t) override;
    std::uint32_t CreateDocument(std::string_view, std::uint32_t) override;
    void DestroyDocument(std::uint32_t) override;
    bool CheckDocumentVersion(std::uint32_t, std::uint32_t) override;
    std::uint32_t FindLeadingAttribute(std::uint32_t,
                                       std::string_view) override;
    std::uint32_t CreateAttribute(std::string_view, std::uint32_t) override;
    void AddAttribute(std::uint32_t, std::uint32_t) override;
    void DestroyAttribute(std::uint32_t) override;
    void SetAttributeInt(std::uint32_t, std::uint32_t) override;
    std::uint32_t AttributeToInt(std::uint32_t, std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    void Free(std::uint32_t) override;
    void SetVector(std::uint32_t, std::uint32_t, std::uint32_t) override;
    std::array<std::uint32_t, 3> GetVector(
        std::uint32_t, std::uint32_t) override;
    void ApplyTransform(std::uint32_t, std::uint32_t,
        const std::array<std::uint32_t, 3>&, std::uint32_t) override;
    void SetPlayerState(std::uint32_t, std::uint32_t) override;
    std::uint32_t CreateLookupKey(std::string_view,
                                  std::uint8_t&) override;
    PlayerAttributeCursor FindTreeAttribute(std::uint32_t) override;
    void ResetLookupKey(std::uint32_t) override;
    std::uint8_t WriteDocument(std::uint32_t, std::uint32_t) override;
};

class PlayerManagerSerializer final {
public:
    static constexpr std::uint32_t kFlagSource = 0x8a7578u;
    static constexpr std::uint32_t kLeaderPositionName = 0x8e4394u;
    static constexpr std::uint32_t kLeaderRotationName = 0x8e4380u;
    static constexpr std::uint32_t kEmptyName = 0x841964u;

    // IDA 0x441b00. The incoming byte is read from uninitialized stack storage.
    [[nodiscard]] static std::uint8_t Serialize(
        std::uint32_t manager, std::uint32_t data,
        std::uint8_t incoming_stack_byte,
        IPlayerManagerSerializeRuntime& runtime);
    // IDA 0x441eb0. Returns false for flag mismatch or XML version failure.
    [[nodiscard]] static bool Deserialize(
        std::uint32_t manager, std::uint32_t data,
        std::uint8_t incoming_stack_byte,
        IPlayerManagerSerializeRuntime& runtime);
};

}  // namespace pal4
