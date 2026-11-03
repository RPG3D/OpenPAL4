#include "pal4/portable/combat_formula_catalog.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void Word(std::string& out, std::uint32_t value) {
    for (unsigned shift = 0u; shift < 32u; shift += 8u)
        out.push_back(static_cast<char>(value >> shift));
}

void Text(std::string& out, std::string_view value) {
    Word(out, static_cast<std::uint32_t>(value.size()));
    out.append(value);
}

struct Field final { std::string_view name; std::uint32_t type; };

void BeginTable(std::string& out, std::string_view name,
                std::span<const Field> fields, std::uint32_t rows) {
    Text(out, name); Word(out, 20060225u); Text(out, name); Text(out, "");
    Word(out, 0u); Word(out, static_cast<std::uint32_t>(fields.size()));
    Word(out, rows);
    for (const auto& field : fields) {
        Text(out, field.name); Word(out, field.type); Text(out, ""); Text(out, "");
    }
}

void FinishRow(std::string& out) {
    Word(out, 20060224u);
}

void FinishTable(std::string& out) {
    Word(out, 20060226u);
}

void AddTextTable(std::string& out, std::string_view name,
                  std::string_view value_field, std::string_view value) {
    constexpr Field fields[]{{"ID",1u},{"TYPE",1u},{"PA_NAME",2u},{"PA_DESCRIPTION",2u},{"PA_PROPERTY",2u},{"PA_ICON",2u},{"PA_MODEL",2u},{"PA_TEXTURE",2u},{"PA_ATTACHED_EFFECT",2u}};
    constexpr Field text_fields[]{{"ID", 1u}, {"STRING", 2u}};
    if (name == "Property") {
        BeginTable(out, name, fields, 1u);
        Word(out, 20060223u); Word(out, 7u); Word(out, 4u); Text(out, value);
        for(unsigned i=0;i<6;++i)Text(out, "");
    } else {
        BeginTable(out, name, text_fields, 1u);
        Word(out, 20060223u); Word(out, 7u); Text(out, value);
    }
    FinishRow(out);
    FinishTable(out);
    (void)value_field;
}

std::string Database(std::uint32_t formula_rows, bool duplicate_key = false,
                     bool inverted_physical = false) {
    std::string data;
    Text(data, "GAME_DB_FLAG"); Word(data, 20060228u); Word(data, 0u);
    Word(data, 4u);
    AddTextTable(data, "Property", "PA_NAME", "test");
    AddTextTable(data, "Tutorial", "STRING", "tutorial");
    AddTextTable(data, "GameString", "STRING", "string");

    constexpr Field formula_fields[]{{"ID", 1u}, {"Value", 3u}};
    BeginTable(data, "CombatFormulaConfig", formula_fields, formula_rows);
    for (std::uint32_t row = 0u; row < formula_rows; ++row) {
        std::uint32_t key = 32u - row;
        if (duplicate_key && row == 1u) key = 32u;
        float value = static_cast<float>(key) / 10.0f;
        if (inverted_physical && key == 2u) value = 1.2f;
        if (inverted_physical && key == 3u) value = 0.8f;
        Word(data, 20060223u); Word(data, key);
        Word(data, std::bit_cast<std::uint32_t>(value)); FinishRow(data);
    }
    FinishTable(data);
    return data;
}

void LoadsOriginalSharedKeyTable() {
    const auto bytes = Database(32u);
    const auto catalog = pal4::portable::CombatFormulaCatalog::ReadDatabase(
        std::span(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()));
    for (std::int32_t key = 1; key <= 32; ++key)
        Check(catalog.FloatAt(key - 1) == static_cast<float>(key) / 10.0f,
              "shared loader resolves sorted formula key to original row value");

    const auto inverted = Database(32u, false, true);
    const auto warned = pal4::portable::CombatFormulaCatalog::ReadDatabase(
        std::span(reinterpret_cast<const std::uint8_t*>(inverted.data()), inverted.size()));
    Check(warned.Warnings().size() == 1u,
          "shared formula loader preserves its physical endpoint warning");
}

void RejectsMissingAndDuplicateKeys() {
    const auto expect_failure = [](const std::string& bytes) {
        bool failed{};
        try {
            static_cast<void>(pal4::portable::CombatFormulaCatalog::ReadDatabase(
                std::span(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size())));
        } catch (const std::exception&) { failed = true; }
        Check(failed, "malformed CombatFormulaConfig is rejected");
    };
    expect_failure(Database(31u));
    expect_failure(Database(32u, true));
}
} // namespace

int main() {
    try {
        LoadsOriginalSharedKeyTable();
        RejectsMissingAndDuplicateKeys();
        std::cout << "combat formula catalog contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
