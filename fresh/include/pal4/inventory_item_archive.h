#pragma once

#include <cstdint>

#include "pal4/inventory_item_collection.h"

namespace pal4 {

class IInventoryItemArchiveRuntime : public IInventoryItemStreamRuntime {
public:
    virtual void BeforeSerialize(std::uint32_t object,
                                 std::uint32_t stream) = 0;
    virtual void BeforeDeserialize(std::uint32_t object,
                                   std::uint32_t stream) = 0;
    virtual void WriteStreamWords(std::uint32_t stream,
                                  std::uint32_t begin,
                                  std::uint32_t count) = 0;
    virtual void ReadStreamWords(std::uint32_t stream,
                                 std::uint32_t begin,
                                 std::uint32_t count) = 0;
};

class InventoryItemArchive final {
public:
    // IDA 0x4c7860: five item slots, keys, recent IDs and empty tree.
    [[nodiscard]] static std::uint8_t Serialize(
        std::uint32_t object, std::uint32_t stream,
        IInventoryItemArchiveRuntime& runtime);
    // IDA 0x4c7a50: restore slots, keys and recent IDs; consume tree IDs.
    [[nodiscard]] static std::uint8_t Deserialize(
        std::uint32_t object, std::uint32_t stream,
        IInventoryItemArchiveRuntime& runtime);
};

}  // namespace pal4
