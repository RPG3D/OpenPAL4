#pragma once

#include "pal4/config_value_lookup.h"
#include "pal4/collection_slot_operations.h"
#include "pal4/indexed_collection_reset.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IHostServerStreamRuntime {
public:
    virtual ~IHostServerStreamRuntime() = default;
    virtual void AssignMarker(ConfigStringState& marker,
                              std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint8_t WriteCount(
        std::uint32_t stream, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t WriteBytes(
        std::uint32_t stream, std::uint32_t address,
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class HostServerStreamRuntimeStub final : public IHostServerStreamRuntime {
public:
    void AssignMarker(ConfigStringState& marker,
                      std::string_view text) override;
    std::uint32_t EmptyStringAddress() override;
    std::uint8_t WriteCount(std::uint32_t stream,
                            std::uint32_t value) override;
    std::uint8_t WriteBytes(std::uint32_t stream, std::uint32_t address,
                            std::uint32_t size) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Free(std::uint32_t address) override;
};

class IHostServerReadRuntime : public ICollectionSlotRuntime {
public:
    ~IHostServerReadRuntime() override = default;
    [[nodiscard]] virtual std::uint8_t ReadMarker(
        std::uint32_t stream, ConfigStringState& marker) = 0;
    [[nodiscard]] virtual std::int32_t CompareMarker(
        const ConfigStringState& marker, std::string_view expected) = 0;
    [[nodiscard]] virtual std::uint8_t ReadCount(
        std::uint32_t stream, std::uint32_t& count) = 0;
    [[nodiscard]] virtual std::uint8_t ReadBytes(
        std::uint32_t stream, std::uint32_t destination,
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 2> PairFillWords() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class HostServerReadRuntimeStub final : public IHostServerReadRuntime {
public:
    std::uint8_t ReadMarker(std::uint32_t stream,
                            ConfigStringState& marker) override;
    std::int32_t CompareMarker(const ConfigStringState& marker,
                               std::string_view expected) override;
    std::uint8_t ReadCount(std::uint32_t stream,
                           std::uint32_t& count) override;
    std::uint8_t ReadBytes(std::uint32_t stream,
                           std::uint32_t destination,
                           std::uint32_t size) override;
    std::array<std::uint32_t, 2> PairFillWords() override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    void BeforeFreePairs(std::uint32_t begin, std::uint32_t end) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Free(std::uint32_t address) override;
};

class HostServerDataCodec final {
public:
    static constexpr std::string_view kMarker = "MGC_HOST_SER_FLAG";

    [[nodiscard]] static bool Serialize(
        const IndexedCollectionResetState& state, std::uint32_t stream,
        IHostServerStreamRuntime& runtime);
    [[nodiscard]] static bool Deserialize(
        IndexedCollectionResetState& state, std::uint32_t stream,
        IHostServerReadRuntime& runtime);
};

}  // namespace pal4
