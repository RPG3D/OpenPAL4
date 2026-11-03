#pragma once

#include "pal4/ui_red_black_tree_rotation.h"

#include <cstdint>

namespace pal4 {

class IUiRedBlackTreeRemoveRuntime : public IUiRedBlackTreeLinkRuntime {
public:
    virtual ~IUiRedBlackTreeRemoveRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class IUiCeguiRedBlackTreeRemoveRuntime
    : public IUiRedBlackTreeRemoveRuntime {
public:
    virtual void DestroyCeguiString(std::uint32_t address) = 0;
};

class UiRedBlackTreeRemove final {
public:
    // IDA 0x64b710: 36-byte effect nodes, colour at +32, no text payload.
    [[nodiscard]] static std::uint32_t RemoveEffectNode(
        std::uint32_t tree, std::uint32_t node,
        IUiRedBlackTreeRemoveRuntime& runtime);
    // IDA 0x664a60: 40-byte texture dictionary nodes, colour at +36.
    [[nodiscard]] static std::uint32_t RemoveTextureNode(
        std::uint32_t tree, std::uint32_t node,
        IUiRedBlackTreeRemoveRuntime& runtime);
    // IDA 0x4c07f0. Returns output and writes the next iterator node to it.
    [[nodiscard]] static std::uint32_t Remove(
        std::uint32_t tree, std::uint32_t output, std::uint32_t node,
        IUiRedBlackTreeRemoveRuntime& runtime);

    // IDA 0x4c1060. Uses colour at +0xb4 and a CEGUI string payload.
    [[nodiscard]] static std::uint32_t RemoveCegui(
        std::uint32_t tree, std::uint32_t output, std::uint32_t node,
        IUiCeguiRedBlackTreeRemoveRuntime& runtime);

    // IDA 0x4c1da0. Factory nodes store colour at +0x14 and own no text.
    [[nodiscard]] static std::uint32_t RemoveFactory(
        std::uint32_t tree, std::uint32_t output, std::uint32_t node,
        IUiRedBlackTreeRemoveRuntime& runtime);
};

}  // namespace pal4
