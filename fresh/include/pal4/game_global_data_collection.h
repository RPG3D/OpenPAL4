#pragma once

#include <cstdint>

namespace pal4 {

class IGameGlobalDataCollectionRuntime {
public:
    virtual ~IGameGlobalDataCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GlobalDataSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t NilNode() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t PreviousNode(std::uint32_t& node) = 0;
    [[nodiscard]] virtual std::uint8_t InsertNode(
        std::uint32_t tree, std::uint32_t child, std::uint32_t parent,
        std::int32_t key) = 0;
};

class GameGlobalDataCollection final {
public:
    // IDA 0x5c3b10: insert into the tree at global data +0x84.
    [[nodiscard]] static std::uint8_t InsertKey(
        std::int32_t key, IGameGlobalDataCollectionRuntime& runtime);
    // IDA 0x5c3bc0: read global data +0x90.
    [[nodiscard]] static std::uint32_t WordAt90(
        IGameGlobalDataCollectionRuntime& runtime);
};

}  // namespace pal4
