#include "pal4/native_script_money_transaction.h"
#include "pal4/native_script_inventory_query.h"
#include "pal4/native_script_inventory_change.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_game_database.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/game_text_format.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace { void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
} }

int main(int argc, char** argv) try {
    Check(argc == 2, "original root required");
    pal4::NativeGameDatabase database;
    Check(database.Load(argv[1]), "original GameDB");
    pal4::NativeInventorySession inventory;
    inventory.BindDatabase(database); (void)inventory.Start(); inventory.Reset();
    unsigned creates{}, shown{}, sounds{};
    bool allow_hint = true, throw_show = false;
    std::string hint_text;
    pal4::NativeScriptInventoryChange hints(inventory, database,
        [&] { ++creates; return allow_hint ? 1u : 0u; },
        [&](unsigned window, std::string_view text, float x, float y,
            float scale, unsigned mode, std::uint8_t enabled) {
            Check(window == 1u && x == 0.5f && y == 0.15f && scale == 1.0f &&
                  mode == 0u && enabled == 1u, "original money hint ABI");
            if (throw_show) throw std::runtime_error("money hint fixture failure");
            ++shown; hint_text = text;
        },
        [&](std::string_view path, unsigned mode) {
            Check(path == "gamedata\\PALSound\\WA40.wav" && mode == 1u,
                  "original money gain sound"); ++sounds;
        });
    pal4::NativeScriptMoneyTransaction money(inventory, hints);
    pal4::NativeScriptInventoryQuery query(inventory);
    pal4::NativeGameScriptServices services;
    services.money_transaction = &money;
    services.inventory_query = &query;
    pal4::NativeGameScriptCallbacks callbacks(services);
    pal4::NativeAngelScriptEngine engine; engine.SetCallbacks(&callbacks);
    const std::string script =
        "void Gain(){giAddMoney(1000,true);} "
        "void Query(){global_mvar=giGetMoney();} "
        "void HasOne(){global_mvar=giCheckPackProperty(3121,1)?1:0;} "
        "void HasTwo(){global_mvar=giCheckPackProperty(3121,2)?1:0;} "
        "void HasZero(){global_mvar=giCheckPackProperty(3121,0)?1:0;} "
        "void HasNegative(){global_mvar=giCheckPackProperty(3121,-1)?1:0;} "
        "void Missing(){global_mvar=giCheckPackProperty(-7,-1)?1:0;} "
        "void Pay(){giPayMoney(7,true);} "
        "void NegativeGain(){giAddMoney(-4,false);} "
        "void NegativePay(){giPayMoney(-4,false);} "
        "void Silent(){giAddMoney(10,false);} "
        "void Lower(){giPayMoney(1000000,false);} "
        "void Upper(){giAddMoney(2147483647,false);} ";
    Check(engine.Raw()->AddScriptSection("money", "money.cs", script.data(),
          static_cast<int>(script.size())) >= 0 && engine.Raw()->Build("money") >= 0,
          "actual SDK money script compilation");
    asIScriptContext* context{};
    Check(engine.Raw()->CreateContext(&context) >= 0 && context, "SDK context");
    struct Release { asIScriptContext* value; ~Release(){ (void)value->Release(); } } release{context};
    const auto run = [&](const char* name) {
        const auto function = engine.FunctionId("money", name);
        Check(function >= 0 && context->Prepare(function) >= 0, "prepare money script");
        return context->Execute();
    };
    const auto value = [&] { return inventory.ReadDword(inventory.Address() + 0x134u); };
    const auto query_is = [&](const char* name, int expected) {
        if (run(name) != asEXECUTION_FINISHED || engine.GlobalStoryVariable() != expected)
            throw std::runtime_error(std::string("SDK CheckPackProperty result: ") + name);
    };
    query_is("HasOne", 0); query_is("HasZero", 0); query_is("HasNegative", 0);
    inventory.BindItemObserver([](std::uint32_t){});
    (void)inventory.AddItem(3121u, 1);
    query_is("HasOne", 1); query_is("HasTwo", 0);
    query_is("HasZero", 1); query_is("HasNegative", 1); query_is("Missing", 0);
    (void)inventory.AddItem(3121u, 1); query_is("HasTwo", 1);
    (void)inventory.RemoveItem(3121u, 2); query_is("HasOne", 0);
    Check(!creates && !shown && !sounds && value() == 0u,
          "inventory queries never show hints, play audio or change money");
    Check(run("Gain") == asEXECUTION_FINISHED && value() == 1000u &&
          creates == 1 && shown == 1 && sounds == 1, "script gain and real inventory");
    Check(hint_text == pal4::FormatGameTextInteger(*database.FindTableTextById("GameString", 0xc2u), 1000),
          "original gain GameString C2");
    Check(run("Query") == asEXECUTION_FINISHED && engine.GlobalStoryVariable() == 1000 && value() == 1000u &&
          creates == 1 && shown == 1 && sounds == 1, "GetMoney reads actual inventory without hint/audio/mutation");
    Check(run("Pay") == asEXECUTION_FINISHED && value() == 993u &&
          creates == 2 && shown == 2 && sounds == 1, "pay has hint and no gain sound");
    Check(hint_text == pal4::FormatGameTextInteger(*database.FindTableTextById("GameString", 0x10bu), 7),
          "original payment GameString10B");
    Check(run("NegativeGain") == asEXECUTION_FINISHED && value() == 989u &&
          run("NegativePay") == asEXECUTION_FINISHED && value() == 993u,
          "SDK signed amounts reach shared add/pay semantics");
    Check(run("Silent") == asEXECUTION_FINISHED && value() == 1003u &&
          creates == 2 && shown == 2 && sounds == 3, "show=false skips hint, retains gain sound");
    allow_hint = false;
    Check(run("Gain") == asEXECUTION_FINISHED && value() == 2003u &&
          shown == 2 && sounds == 4, "missing hint does not suppress money mutation or sound");
    allow_hint = true; throw_show = true;
    Check(run("Gain") == asEXECUTION_EXCEPTION && value() == 3003u && sounds == 4 &&
          std::string_view(reinterpret_cast<const char*>(money.FormattedTextAddress())).empty(),
          "presentation failure cleans text without rolling back prior money change");
    throw_show = false;
    Check(run("Lower") == asEXECUTION_FINISHED && value() == 0u &&
          run("Upper") == asEXECUTION_FINISHED && value() == 9999999u &&
          run("Upper") == asEXECUTION_FINISHED && value() == 0u,
          "actual inventory clamps lower/upper and preserves signed-add wrap");
    bool rejected{};
    try { (void)pal4::FormatGameTextInteger("%s", 1); }
    catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected && pal4::FormatGameTextInteger("", 1).empty() &&
          pal4::FormatGameTextInteger("%d", -2147483647 - 1) == "-2147483648",
          "integer format validates vararg type, empty fallback and signed range");
    inventory.Close();
    Check(!inventory.OutstandingAllocations(), "inventory storage retired");
    std::cout << "PASS actual SDK CheckPackProperty/GetMoney/AddMoney/PayMoney with original GameDB, live item mutation, signed deltas, "
                 "hints, sound, missing hint, exception cleanup and inventory bounds\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n'; return 1;
}
