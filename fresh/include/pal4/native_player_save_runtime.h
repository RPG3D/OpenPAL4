#pragma once

#include "pal4/binary_xml_config.h"
#include "pal4/native_game_save_archive_buffer.h"
#include "pal4/native_player_basic_field_schema.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace pal4 {

class NativeInventorySession;

class NativePlayerSaveRuntime final {
public:
    using ValueResolver = std::function<std::optional<BinaryXmlAttribute::Value>(
        const NativePlayerBasicField&)>;

    explicit NativePlayerSaveRuntime(NativeGameSaveArchiveBuffer& archive)
        : archive_(archive) {}

    [[nodiscard]] bool Serialize(const ValueResolver& resolve,
                                 std::span<const std::int32_t> forbidden_skills);
    [[nodiscard]] bool SerializeSlot(std::uint32_t player_slot,
                                     NativeInventorySession& inventory);

private:
    NativeGameSaveArchiveBuffer& archive_;
};

}  // namespace pal4
