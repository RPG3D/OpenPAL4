#include "pal4/world_map_flight.h"
#include "pal4/renderware_vector2.h"

#include <cmath>

namespace pal4 {
namespace {

std::uint32_t ReleaseName(const std::uint32_t name,
                          const std::uint32_t prior_result,
                          IWorldMapFlightRuntime& runtime) {
    if (name == 0u) return prior_result;
    const auto header = name - 1u;
    const auto count = runtime.ReadByte(header);
    if (count == 0u || count == 0xffu) return runtime.Free(header);
    runtime.WriteByte(header, static_cast<std::uint8_t>(count - 1u));
    return header;
}

std::uint32_t AdvanceBody(
    const std::uint32_t frame, const std::uint32_t script_name,
    IWorldMapFlightRuntime& runtime, IRenderWareVector2& renderware) {
    const auto velocity_x = runtime.ReadFloat(frame + 0x240u);
    const auto speed_x = runtime.ReadFloat(frame + 0x268u);
    const auto velocity_y = runtime.ReadFloat(frame + 0x244u);
    const auto speed_y = runtime.ReadFloat(frame + 0x268u);
    // IDA484C2D..484C9A: the game's x87 PC24 mode rounds each FMUL
    // before its travel/marker FADDs, even while dx remains on the x87 stack.
    // Widening dx to double changes the real Q06->M11 flight after six ticks.
    const float dx = velocity_x * speed_x;
    const float dy = velocity_y * speed_y;
    const float travel_x = runtime.ReadFloat(frame + 0x250u) + dx;
    runtime.WriteFloat(frame + 0x250u, travel_x);
    const auto travel_y = runtime.ReadFloat(frame + 0x254u) + dy;
    runtime.WriteFloat(frame + 0x254u, travel_y);
    const float position_x = runtime.ReadFloat(frame + 0x248u) + dx;
    runtime.WriteFloat(frame + 0x248u, position_x);
    const auto position_y = runtime.ReadFloat(frame + 0x24cu) + dy;
    runtime.WriteFloat(frame + 0x24cu, position_y);
    runtime.SetPosition(runtime.ReadWord(frame + 0x238u),
                        position_x, position_y);

    const auto length = renderware.Length(travel_x, travel_y);
    const auto threshold = runtime.ReadFloat(frame + 0x23cu);
    std::uint32_t result = 0x100u;
    if (!std::isunordered(length, static_cast<double>(threshold)) &&
        length >= threshold) {
        const auto manager = runtime.FrameManager();
        const auto collection = runtime.ReadWord(manager + 0x1a0u);
        runtime.RemoveFlyMember(collection, frame + 0x258u);
        runtime.InitializeScripts();
        const auto name = script_name == 0u ? runtime.EmptyText() :
                                               script_name;
        const auto mode = runtime.ScriptMode();
        const auto interpreter = runtime.CreateInterpreter(
            mode, "WorldMap", name, name, WorldMapFlight::kScriptFile,
            WorldMapFlight::kReader);
        if (interpreter != 0u) {
            const auto engine = runtime.ScriptEngine();
            if (runtime.AddScriptModule(engine + 0x54u, interpreter)) {
                const auto error_engine = runtime.ScriptEngine();
                const auto function_name = runtime.FunctionName(interpreter);
                const auto message = runtime.FullPath(interpreter);
                runtime.PushPreparedModule(error_engine + 0x54u,
                                              message, function_name);
                runtime.ReleaseInterpreter(interpreter);
                const auto vtable = runtime.ReadWord(frame);
                runtime.CallHide(frame, runtime.ReadWord(vtable + 4u));
            }
        }
        result = runtime.CleanupAllScripts();
    }
    return result;
}

}  // namespace

std::uint32_t WorldMapFlight::Advance(
    const std::uint32_t frame, const std::uint32_t script_name,
    IWorldMapFlightRuntime& runtime, IRenderWareVector2& renderware) {
    std::uint32_t result = 0u;
    try {
        result = AdvanceBody(frame, script_name, runtime, renderware);
    } catch (...) {
        (void)ReleaseName(script_name, 0u, runtime);
        throw;
    }
    return ReleaseName(script_name, result, runtime);
}

}  // namespace pal4
