#include "pal4/game_db_row_formatter.h"

#include <iomanip>
#include <locale>
#include <sstream>

namespace pal4 {

std::uint8_t GameDbRowFormatter::Dump(
    const GameDbSchemaArrayState& schemas,
    const GameDbValueArrayState& row,
    std::string& output, IGameDbRowFormatRuntime& runtime) {
    const auto schema_begin = schemas.words[1];
    const auto field_count = schema_begin == 0u
                                 ? 0u : (schemas.words[2] - schema_begin) / 60u;
    const auto row_count = row.begin == 0u
                               ? 0u : (row.end - row.begin) / 8u;
    if (field_count != row_count) {
        output = "ERROR, the row doesn't belong to the table!! ";
        return 1u;
    }

    output += "\ndump dbRow info :\n ID\tFieldInfo\tValue\n";
    std::string last_line;
    for (std::uint32_t index = 0u; index < field_count; ++index) {
        const auto field = schema_begin + index * 60u;
        const auto kind = runtime.FieldType(field);
        if (kind >= 1u && kind <= 3u) {
            const char* name = runtime.FieldName(field);
            if (name == nullptr)
                name = "";
            std::ostringstream line;
            line.imbue(std::locale::classic());
            line << ' ' << index << '\t' << name << '\t';
            switch (kind) {
            case 1u:
                line << runtime.IntegerAt(row, index);
                break;
            case 2u: {
                const char* text = runtime.StringAt(row, index);
                line << (text == nullptr ? "" : text);
                break;
            }
            case 3u:
                line << std::fixed << std::setprecision(6)
                     << runtime.FloatAt(row, index);
                break;
            }
            line << " \n";
            last_line = line.str();
        }
        output += last_line;
    }
    return 0u;
}

}  // namespace pal4
