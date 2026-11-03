#pragma once

#include "pal4/game_system_data.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class IGameDataPersistenceRuntime {
public:
    virtual ~IGameDataPersistenceRuntime() = default;
    virtual void PrepareTarget(std::uint32_t target,
                               std::string_view marker) = 0;
    [[nodiscard]] virtual std::optional<std::string> ReadMarker(
        std::uint32_t source) = 0;
    virtual void ProcessNativeRecords(std::uint32_t stream,
                                      bool reading) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNode(std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ParseNode(
        std::uint32_t source, std::string_view name) = 0;
    [[nodiscard]] virtual bool CheckVersionAndType(
        std::uint32_t node, std::uint32_t source) = 0;
    [[nodiscard]] virtual bool HasAttribute(
        std::uint32_t node, std::string_view key) = 0;
    virtual void SetInt(std::uint32_t node, std::string_view key,
                        std::int32_t value) = 0;
    virtual void AddInt(std::uint32_t node, std::string_view key,
                        std::int32_t value) = 0;
    virtual void SetString(std::uint32_t node, std::string_view key,
                           std::string_view value) = 0;
    virtual void AddString(std::uint32_t node, std::string_view key,
                           std::string_view value) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t node, std::string_view key,
        std::int32_t default_value) = 0;
    [[nodiscard]] virtual std::string ReadString(
        std::uint32_t node, std::string_view key,
        std::string_view default_value) = 0;
    [[nodiscard]] virtual std::uint8_t Write(
        std::uint32_t node, std::uint32_t target) = 0;
    virtual void DestroyNode(std::uint32_t node) = 0;
    virtual void ReleaseNativeRecords(std::uint32_t stream) = 0;
    virtual void ReleaseMarker() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectInternalId(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t TimerFrameDuration(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalDataSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t PetManager() = 0;
};

class GameDataPersistence final {
public:
    explicit GameDataPersistence(std::uint32_t address) : address_(address) {}

    // IDA 0x5c2f60 / 0x5c3370.
    [[nodiscard]] std::uint8_t Serialize(
        std::uint32_t target, IGameDataPersistenceRuntime& runtime) const;
    [[nodiscard]] bool Deserialize(
        std::uint32_t source, IGameDataPersistenceRuntime& runtime,
        IGameSystemDataRuntime& core_runtime) const;

private:
    std::uint32_t address_;
};

}  // namespace pal4
