#pragma once

#include "pal4/game_db_archive_reader.h"
#include "pal4/game_db_typed_value.h"

#include <cstdint>
#include <string>

namespace pal4 {

class IGameDbRowFormatRuntime {
public:
    virtual ~IGameDbRowFormatRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FieldType(
        std::uint32_t schema_address) = 0;
    [[nodiscard]] virtual const char* FieldName(
        std::uint32_t schema_address) = 0;
    [[nodiscard]] virtual std::int32_t IntegerAt(
        const GameDbValueArrayState& row, std::uint32_t index) = 0;
    [[nodiscard]] virtual const char* StringAt(
        const GameDbValueArrayState& row, std::uint32_t index) = 0;
    [[nodiscard]] virtual double FloatAt(
        const GameDbValueArrayState& row, std::uint32_t index) = 0;
};

class GameDbRowFormatter final {
public:
    // IDA 0x656550: dump matching schema/row values or replace with error.
    [[nodiscard]] static std::uint8_t Dump(
        const GameDbSchemaArrayState& schemas,
        const GameDbValueArrayState& row,
        std::string& output, IGameDbRowFormatRuntime& runtime);
};

}  // namespace pal4
