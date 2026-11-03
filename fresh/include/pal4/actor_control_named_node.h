#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ActorControlNamedNodeRecord {
    std::uint32_t vtable{};
    std::uint32_t text_flags{};
    std::uint32_t text_data{};
    std::uint32_t text_size{};
    std::uint32_t text_capacity{};
    std::uint32_t extra0{};
    std::uint32_t extra1{};
};
static_assert(sizeof(ActorControlNamedNodeRecord) == 28u);
static_assert(offsetof(ActorControlNamedNodeRecord, text_data) == 8u);

class IActorControlNamedNodeRuntime {
public:
    virtual ~IActorControlNamedNodeRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t TextLength(
        std::uint32_t address) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t value) = 0;
    virtual void ReallocateText(std::uint32_t record,
                                std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
    [[nodiscard]] virtual std::uint32_t FreeAddress(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeActorControl() = 0;
    [[nodiscard]] virtual std::uint32_t FloorAttributeInstance() = 0;
    [[nodiscard]] virtual std::int32_t CompareBytes(
        std::uint32_t left, std::uint32_t right,
        std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void PlaySound(std::uint32_t manager, std::uint32_t node,
                           std::uint32_t sound, std::uint32_t position,
                           std::uint32_t mode, std::uint32_t max_distance,
                           std::uint32_t min_distance) = 0;
    virtual void InvokeSoundCallback(std::uint32_t callback,
                                     std::uint32_t sound) = 0;
};

class ActorControlNamedNodeRuntimeStub : public IActorControlNamedNodeRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    [[nodiscard]] std::uint32_t TextLength(
        std::uint32_t address) override;
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t value) override;
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override;
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t length) override;
    [[noreturn]] void ThrowLength() override;
    [[nodiscard]] std::uint32_t FreeAddress(
        std::uint32_t address) override;
    [[nodiscard]] std::uint32_t InitializeActorControl() override;
    [[nodiscard]] std::uint32_t FloorAttributeInstance() override;
    [[nodiscard]] std::int32_t CompareBytes(
        std::uint32_t left, std::uint32_t right,
        std::uint32_t length) override;
    [[nodiscard]] std::uint32_t AudioManager() override;
    void PlaySound(std::uint32_t manager, std::uint32_t node,
                   std::uint32_t sound, std::uint32_t position,
                   std::uint32_t mode, std::uint32_t max_distance,
                   std::uint32_t min_distance) override;
    void InvokeSoundCallback(std::uint32_t callback,
                             std::uint32_t sound) override;
};

class ActorControlNamedNode final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x8425a0u;
    static constexpr std::uint32_t kDerivedVtable = 0x842590u;
    static constexpr std::uint32_t kEmptyErrorString = 0x8dc780u;
    static constexpr std::uint32_t kEmptyText = 0x841964u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t node, std::uint32_t source,
        IActorControlNamedNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t node, IActorControlNamedNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t TriggerFloorSound(
        std::uint32_t node, IActorControlNamedNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteBase(
        std::uint32_t node, std::uint32_t flags,
        IActorControlNamedNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetOwner(
        std::uint32_t node, std::uint32_t owner,
        IActorControlNamedNodeRuntime& runtime);
};

}  // namespace pal4
