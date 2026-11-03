#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/script_minimap_level.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool ok) { if (!ok) throw std::runtime_error("compiled minimap level dispatch"); }
class Runtime final : public pal4::IScriptMinimapLevelRuntime {
public:
    std::vector<std::uint32_t> levels;
    std::uint32_t player_index{2u}, updates{};
    std::uint32_t MinimapManager() override { return 0x3000u; }
    void SetTexture(std::uint32_t manager, std::uint32_t level) override {
        Check(manager == 0x3000u); levels.push_back(level);
    }
    std::uint32_t PlayerManager() override { return 0x1000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x3c50u); return player_index;
    }
    void UpdateGrid(std::uint32_t minimap, std::uint32_t position, std::uint32_t rotation) override {
        const auto player = 0x1000u + player_index * 0xb14u;
        Check(minimap == 0x3000u && position == player + 0xacu && rotation == player + 0xa0u);
        ++updates;
    }
};
}
int main() {
    try {
        Runtime first, second;
        pal4::NativeGameScriptServices services{}; services.minimap_level = &first;
        pal4::NativeGameScriptCallbacks callbacks(services);
        pal4::NativeAngelScriptEngine engine; engine.SetCallbacks(&callbacks);
        constexpr char code[] = "void Run(){giSetMinimapLevel(1);giSetMinimapLevel(-1);giSetMinimapLevel(0);}";
        Check(engine.Raw()->AddScriptSection("minimap", "minimap.cs", code, sizeof(code)-1u) >= 0);
        Check(engine.Raw()->Build("minimap") >= 0);
        asIScriptContext* context{}; Check(engine.Raw()->CreateContext(&context) >= 0 && context);
        try {
            const auto run = [&] { Check(context->Prepare(engine.FunctionId("minimap", "Run")) >= 0); return context->Execute(); };
            Check(run() == asEXECUTION_FINISHED && first.updates == 3u &&
                  first.levels == std::vector<std::uint32_t>{1u,0xffffffffu,0u});
            services.minimap_level = &second; second.player_index = 1u;
            pal4::NativeGameScriptCallbacks replacement(services);
            engine.SetContextCallbacks(context, &replacement);
            Check(run() == asEXECUTION_FINISHED && second.updates == 3u && first.updates == 3u);
            pal4::NativeGameScriptCallbacks missing({}); engine.SetContextCallbacks(context, &missing);
            Check(run() == asEXECUTION_EXCEPTION && first.updates == 3u && second.updates == 3u);
            context->Release();
        } catch (...) { context->Release(); throw; }
        std::cout << "Compiled minimap level: -221, signed sentinel, current player stride, context rebind and missing owner pass\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
