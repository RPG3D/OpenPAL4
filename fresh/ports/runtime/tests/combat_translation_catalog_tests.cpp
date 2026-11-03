#include "pal4/portable/combat_translation_catalog.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void Word(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        out.push_back(static_cast<std::uint8_t>(value >> (i * 8u)));
}
void Text(std::vector<std::uint8_t>& out, std::string_view value) {
    Word(out, static_cast<std::uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}
void Field(std::vector<std::uint8_t>& out, std::string_view name) {
    Text(out, name);
    Word(out, 2u);
    Text(out, "");
    Text(out, "");
}
std::vector<std::uint8_t> Database(std::string_view first_field = "English",
                                   bool omit_translation = false) {
    std::vector<std::uint8_t> bytes;
    Text(bytes, "GAME_DB_FLAG");
    Word(bytes, 20060228u);
    Word(bytes, 0u);
    Word(bytes, 1u);
    Text(bytes, "CombatTranslation");
    Word(bytes, 20060225u);
    Text(bytes, "CombatTranslation");
    Text(bytes, "Original combat UI text");
    Word(bytes, 0u);
    Word(bytes, omit_translation ? 1u : 2u);
    Word(bytes, 2u);
    Field(bytes, first_field);
    if (!omit_translation) Field(bytes, "Translation");
    Word(bytes, 20060223u);
    Text(bytes, "EASI_NONE");
    if (!omit_translation) Text(bytes, "\xce\xde\xd2\xec\xb3\xa3 ");
    Word(bytes, 20060224u);
    Word(bytes, 20060223u);
    Text(bytes, "Second");
    if (!omit_translation) Text(bytes, "Second translation");
    Word(bytes, 20060224u);
    Word(bytes, 20060226u);
    return bytes;
}
void PortableRowsAndMalformedInput() {
    const auto bytes = Database();
    auto catalog = pal4::portable::CombatTranslationCatalog::ReadDatabase(bytes);
    Check(catalog.Size() == 2u, "portable parser retains original rows");
    Check(catalog.Find("EASI_NONE") ==
              std::string_view("\xce\xde\xd2\xec\xb3\xa3 "),
          "lookup preserves original encoded bytes and trailing space");
    Check(!catalog.Find("easi_none") && !catalog.Find("__missing__"),
          "lookup has no case folding or invented fallback");

    for (std::size_t length = 0; length < bytes.size(); ++length) {
        bool rejected{};
        try {
            (void)pal4::portable::CombatTranslationCatalog::ReadDatabase(
                std::span(bytes).first(length));
        } catch (const std::exception&) { rejected = true; }
        Check(rejected, "every truncated combat database prefix is rejected");
    }
    bool trailing_rejected{};
    auto trailing = bytes;
    trailing.push_back(0u);
    try { (void)pal4::portable::CombatTranslationCatalog::ReadDatabase(trailing); }
    catch (const std::exception&) { trailing_rejected = true; }
    Check(trailing_rejected, "trailing database bytes are rejected");

    bool schema_rejected{};
    const auto bad_schema = Database("EnglishText");
    try { (void)pal4::portable::CombatTranslationCatalog::ReadDatabase(bad_schema); }
    catch (const std::exception&) { schema_rejected = true; }
    Check(schema_rejected, "wrong translation field name is rejected");
    bool short_schema_rejected{};
    const auto short_schema = Database("English", true);
    try { (void)pal4::portable::CombatTranslationCatalog::ReadDatabase(short_schema); }
    catch (const std::exception&) { short_schema_rejected = true; }
    Check(short_schema_rejected, "missing translation field is rejected");

    // Repeated construction/destruction uses only owned C++ data; no package
    // file, stream, or borrowed database view escapes the catalog lifetime.
    for (unsigned i = 0; i < 8u; ++i) {
        auto repeated = pal4::portable::CombatTranslationCatalog::ReadDatabase(bytes);
        Check(repeated.Find("Second") == std::string_view("Second translation"),
              "catalog can be reconstructed after prior owner teardown");
    }
}
void OriginalPackage(int argc, char** argv) {
    if (argc < 2) return;
    for (unsigned pass = 0; pass < 3u; ++pass) {
        const auto catalog = pal4::portable::CombatTranslationCatalog::ReadFromGameData(argv[1]);
        Check(catalog.Size() == 229u, "original combatDb table has 229 rows");
        Check(catalog.Find("EASI_NONE") ==
                  std::string_view("\xce\xde\xd2\xec\xb3\xa3 "),
              "original package combat translation matches verified native bytes");
        Check(!catalog.Find("easi_none") && !catalog.Find("__missing_translation__"),
              "original package lookup preserves exact native key semantics");
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        PortableRowsAndMalformedInput();
        OriginalPackage(argc, argv);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
