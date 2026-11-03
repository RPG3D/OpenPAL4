#pragma once

#include <cstdint>

namespace pal4 {

struct PlayerManagerResultArray {
    std::uint8_t incoming_byte{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity_end{};
};

class IPlayerManagerSlotCollectionRuntime {
public:
    virtual ~IPlayerManagerSlotCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void ProcessInventory(std::uint32_t slots,
                                  PlayerManagerResultArray& result) = 0;
    virtual void ProcessIndexedValues(std::uint32_t collection,
                                      PlayerManagerResultArray& result) = 0;
    virtual void ProcessFixedValues(std::uint32_t collection,
                                    PlayerManagerResultArray& result) = 0;
    virtual void HandleFade(std::uint32_t script,
                            std::uint32_t value,
                            std::uint32_t marker) = 0;
    virtual void FinalizeScript(std::uint32_t script) = 0;
};

class PlayerManagerSlotCollectionRuntimeStub :
    public IPlayerManagerSlotCollectionRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t Free(std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void ProcessInventory(std::uint32_t,
                          PlayerManagerResultArray&) override;
    void ProcessIndexedValues(std::uint32_t,
                              PlayerManagerResultArray&) override;
    void ProcessFixedValues(std::uint32_t,
                            PlayerManagerResultArray&) override;
    void HandleFade(std::uint32_t, std::uint32_t,
                    std::uint32_t) override;
    void FinalizeScript(std::uint32_t) override;
};

class PlayerManagerSlotCollectionProcessor final {
public:
    // IDA 0x43f7e0: collect object values and dispatch script fade events.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t object, std::uint8_t incoming_stack_byte,
        IPlayerManagerSlotCollectionRuntime& runtime);
};

}  // namespace pal4
