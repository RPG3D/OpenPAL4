#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

enum class RenderWarePlugin : std::size_t {
    MipmapKL,
    World,
    Morph,
    Collision,
    UserData,
    HAnim,
    Anim,
    Skin,
    NormMap,
    MatFX,
    UVAnim,
    LODAtomic,
    PVS,
    Random,
    LtMap,
    PTank,
    PrtStd,
    Spline,
    GamePlugin8449,
    GamePlugin8450,
    GamePlugin8451,
    GamePlugin8452,
    Count
};

class IRenderWarePluginRuntime {
public:
    virtual ~IRenderWarePluginRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Attach(RenderWarePlugin plugin) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeLogger() = 0;
    virtual void AppendLog(std::uint32_t logger, std::string_view format,
                           std::string_view source_file,
                           std::uint32_t source_line) = 0;
};

class RenderWarePluginInitializer final {
public:
    [[nodiscard]] std::uint32_t Initialize(
        IRenderWarePluginRuntime& runtime) const;
};

}  // namespace pal4
