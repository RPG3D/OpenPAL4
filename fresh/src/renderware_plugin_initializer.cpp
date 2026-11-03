#include "pal4/renderware_plugin_initializer.h"

#include <array>
#include <string>

namespace pal4 {
namespace {

struct PluginStep {
    std::string_view failure_name;
    std::uint32_t failure_line;
};

constexpr std::array<PluginStep,
                     static_cast<std::size_t>(RenderWarePlugin::Count)> kSteps{{
    {"RpMipmapKLPluginAttach", 0xa4u},
    {"RpWorldPluginAttach", 0xacu},
    {"RpMorphPluginAttach", 0xb3u},
    {"RpCollisionPluginAttach", 0xc2u},
    {"RpUserDataPluginAttach", 0xcau},
    {"RpHAnimPluginAttach", 0xd1u},
    {"RtAnimInitialize", 0xd7u},
    {"RpSkinPluginAttach", 0xddu},
    {"RpNormMapPluginAttach", 0xe3u},
    {"RpMatFXPluginAttach", 0xe9u},
    {"RpUVAnimPluginAttach", 0xf0u},
    {"RpLODAtomicPluginAttach", 0xf8u},
    {"RpPVSPluginAttach", 0x100u},
    {"RpRandomPluginAttach", 0x108u},
    {"RpLtMapPluginAttach", 0x110u},
    {"RpPTankPluginAttach", 0x11bu},
    {"", 0u},
    {"RpSplinePluginAttach", 0x127u},
    {"RpAtomicAASorterCBPluginAttach", 0x12eu},
    {"RtkDistOptPluginAttach", 0x135u},
    {"RtkHookAtomicPluginAttach", 0x13cu},
    {"RtkVISAtomicPluginAttach", 0x143u},
}};

constexpr std::string_view kSourceFile =
    R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)";
constexpr std::string_view kSuccessFormat =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCAttachPlugins success\n\n\n";

void LogFailure(IRenderWarePluginRuntime& runtime, const PluginStep& step) {
    std::string format =
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCAttachPlugins failed <";
    format += step.failure_name;
    format += ">]\n\n\n";
    runtime.AppendLog(runtime.InitializeLogger(), format, kSourceFile,
                      step.failure_line);
}

}  // namespace

std::uint32_t RenderWarePluginInitializer::Initialize(
    IRenderWarePluginRuntime& runtime) const {
    for (std::size_t index = 0u; index < kSteps.size(); ++index) {
        const auto plugin = static_cast<RenderWarePlugin>(index);
        if (runtime.Attach(plugin) != 0u) {
            continue;
        }
        if (plugin == RenderWarePlugin::PrtStd) {
            return 0u;
        }
        LogFailure(runtime, kSteps[index]);
        if (plugin != RenderWarePlugin::UserData) {
            return 0u;
        }
    }
    runtime.AppendLog(runtime.InitializeLogger(), kSuccessFormat, kSourceFile,
                      0x152u);
    return 1u;
}

}  // namespace pal4
