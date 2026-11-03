#include "pal4/native_combat_translator.h"
#include "pal4/native_game_database.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cstddef>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::invalid_argument("usage: native_combat_translator_tests GAME_ROOT");
        const std::filesystem::path root(argv[1]);
        pal4::NativeCombatTranslator translator;
        Check(!translator.Loaded() && !translator.Table() && !translator.Find("EASI_NONE"),
              "cold translator has no borrowed table");
        Check(translator.Load(root), "load original combat translation database");
        Check(translator.Loaded() && translator.Table()->rows.size() == 229u,
              "original combat database binds all 229 translation rows");
        // Original GBK bytes, no locale conversion or English fallback.
        Check(translator.Find("EASI_NONE") == std::string_view("\xce\xde\xd2\xec\xb3\xa3 "),
              "original abnormal-status translation bytes");
        Check(!translator.Find("easi_none") && !translator.Find("__missing_translation__"),
              "missing/case-different lookup has no invented translation");
        pal4::NativeGameDatabase reference;
        Check(reference.Load(root, "combatDb.db"), "load original comparison table");
        for (const auto& row : reference.Tables().at("CombatTranslation").rows)
            Check(translator.Find(std::get<std::string>(row[0])) ==
                      std::string_view(std::get<std::string>(row[1])),
                  "each original translation survives native binding");
        const auto* retained = translator.Table();
        const auto text = translator.Find("EASI_NONE");
        Check(!translator.Load(root / "__missing_combat_root__") &&
                  !translator.LastError().empty() && translator.Table() == retained &&
                  translator.Find("EASI_NONE") == text,
              "failed backend load preserves the live table and text views");
        Check(translator.Load(root) && translator.Table()->rows.size() == 229u,
              "repeat successful load replaces the owned backend safely");
        Check(!translator.Load(root, "shopdb.db") && !translator.Loaded() &&
                  !translator.Table() && !translator.Find("EASI_NONE"),
              "loaded backend without exact CombatTranslation clears the binding");
        Check(translator.Load(root), "reload after missing named table");
        translator.Reset();
        translator.Reset();
        Check(!translator.Loaded() && !translator.Table(), "repeated final teardown clears borrowed table");
        Check(translator.Load(root), "translator can reconstruct after final teardown");
        struct Parent {
            std::array<std::uint32_t,95> prefix;
            pal4::NativeCombatTranslatorImage translator;
            std::array<std::uint32_t,31> tail;
        } parent{};
        static_assert(offsetof(Parent,translator)==380u && sizeof(Parent)==520u);
        parent.prefix.fill(0x13579bdfu);parent.tail.fill(0x2468ace0u);
        for(unsigned cycle=0;cycle<3u;++cycle) {
            {
                pal4::NativeCombatTranslator embedded(parent.translator);
                Check(embedded.Address()==reinterpret_cast<std::uint32_t>(&parent)+380u &&
                      parent.translator.words[0]!=0u && !parent.translator.words[1],
                      "constructor creates actual backend in parent before database load");
                auto reject_duplicate=[&] {
                    const auto before=parent.translator.words;
                    bool rejected=false;
                    try {pal4::NativeCombatTranslator duplicate(parent.translator);}
                    catch(const std::logic_error&) {rejected=true;}
                    Check(rejected && before==parent.translator.words,
                          "duplicate translator preserves all existing image fields");
                };
                reject_duplicate();
                Check(embedded.Load(root) && parent.translator.words[1]==
                      reinterpret_cast<std::uint32_t>(embedded.Table()),"parent translation table aliases real database");
                for(const auto& row:reference.Tables().at("CombatTranslation").rows)
                    Check(embedded.Find(std::get<std::string>(row[0]))==std::string_view(std::get<std::string>(row[1])),
                          "embedded translator preserves every original translation");
                reject_duplicate();
                const auto fields=parent.translator.words;
                Check(!embedded.Load(root/"__missing_combat_root__") && fields==parent.translator.words,
                      "embedded failed load preserves real parent table/backend");
                embedded.Reset();embedded.Reset();
                Check(!parent.translator.words[0] && !parent.translator.words[1],
                      "embedded reset clears parent backend/table");
                reject_duplicate(); // Reset does not relinquish ownership.
                if(cycle!=1u)Check(embedded.Load(root),"embedded reconstructs backend after reset");
            }
            Check(!parent.translator.words[0] && !parent.translator.words[1] &&
                  std::all_of(parent.prefix.begin(),parent.prefix.end(),[](auto v){return v==0x13579bdfu;}) &&
                  std::all_of(parent.tail.begin(),parent.tail.end(),[](auto v){return v==0x2468ace0u;}),
                  "translator destruction preserves parent and releases storage for reuse");
        }
        std::cout<<"Native embedded translator: parent=520 offset=380 cycles=3 all229 rows, duplicate-safe cold/live/reset\n";
        std::cout << "Native combat translator: original229 rows, exact bytes, failed-load retention, "
                     "missing-table clear, reload and repeated teardown verified\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
