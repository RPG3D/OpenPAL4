#include "pal4/renderware_plugin_initializer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Log {
    std::string format;
    std::string source;
    std::uint32_t line;
};

class Runtime final : public pal4::IRenderWarePluginRuntime {
public:
    std::uint32_t Attach(const pal4::RenderWarePlugin plugin) override {
        events.emplace_back("attach");
        attached.push_back(plugin);
        return plugin == fail_at || plugin == second_fail_at ? 0u : 2u;
    }
    std::uint32_t InitializeLogger() override {
        events.emplace_back("logger-init");
        return 0x1234u;
    }
    void AppendLog(const std::uint32_t logger, const std::string_view format,
                   const std::string_view source_file,
                   const std::uint32_t source_line) override {
        events.emplace_back("logger-append");
        Check(logger == 0x1234u, "log uses returned logger object");
        logs.push_back({std::string(format), std::string(source_file),
                        source_line});
    }

    pal4::RenderWarePlugin fail_at = pal4::RenderWarePlugin::Count;
    pal4::RenderWarePlugin second_fail_at = pal4::RenderWarePlugin::Count;
    std::vector<pal4::RenderWarePlugin> attached;
    std::vector<std::string> events;
    std::vector<Log> logs;
};

constexpr std::array<std::string_view, 22> kFailureNames{{
    "RpMipmapKLPluginAttach", "RpWorldPluginAttach", "RpMorphPluginAttach",
    "RpCollisionPluginAttach", "RpUserDataPluginAttach",
    "RpHAnimPluginAttach", "RtAnimInitialize", "RpSkinPluginAttach",
    "RpNormMapPluginAttach", "RpMatFXPluginAttach",
    "RpUVAnimPluginAttach", "RpLODAtomicPluginAttach", "RpPVSPluginAttach",
    "RpRandomPluginAttach", "RpLtMapPluginAttach", "RpPTankPluginAttach",
    "", "RpSplinePluginAttach", "RpAtomicAASorterCBPluginAttach",
    "RtkDistOptPluginAttach", "RtkHookAtomicPluginAttach",
    "RtkVISAtomicPluginAttach",
}};
constexpr std::array<std::uint32_t, 22> kFailureLines{{
    0xa4u, 0xacu, 0xb3u, 0xc2u, 0xcau, 0xd1u, 0xd7u, 0xddu,
    0xe3u, 0xe9u, 0xf0u, 0xf8u, 0x100u, 0x108u, 0x110u, 0x11bu,
    0u, 0x127u, 0x12eu, 0x135u, 0x13cu, 0x143u,
}};

void CheckLog(const Log& log, const std::string_view expected_format,
              const std::uint32_t expected_line) {
    Check(log.source ==
              R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)" &&
              log.line == expected_line && log.format == expected_format,
          "exact log format, original source path, and line");
}

void AllSuccess() {
    Runtime runtime;
    pal4::RenderWarePluginInitializer initializer;
    Check(initializer.Initialize(runtime) == 1u,
          "all attach calls succeed");
    Check(runtime.attached.size() == kFailureNames.size(),
          "all 22 plugin operations are called");
    for (std::size_t index = 0u; index < runtime.attached.size(); ++index) {
        Check(runtime.attached[index] ==
                  static_cast<pal4::RenderWarePlugin>(index),
              "plugin attach order follows IDA");
    }
    Check(runtime.logs.size() == 1u, "success logs once");
    CheckLog(runtime.logs[0],
             "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCAttachPlugins success\n\n\n",
             0x152u);
    Check(runtime.events[runtime.events.size() - 2u] == "logger-init" &&
              runtime.events.back() == "logger-append",
          "success log follows final attach");
}

void EveryFailureBranch() {
    pal4::RenderWarePluginInitializer initializer;
    for (std::size_t failed = 0u; failed < kFailureNames.size(); ++failed) {
        Runtime runtime;
        runtime.fail_at = static_cast<pal4::RenderWarePlugin>(failed);
        const bool continues = failed == 4u;
        Check(initializer.Initialize(runtime) == (continues ? 1u : 0u),
              "failure branch return value");
        Check(runtime.attached.size() ==
                  (continues ? kFailureNames.size() : failed + 1u),
              "failure branch stops at the observed operation");
        for (std::size_t index = 0u; index < runtime.attached.size(); ++index) {
            Check(runtime.attached[index] ==
                      static_cast<pal4::RenderWarePlugin>(index),
                  "failure path call order");
        }
        if (failed == 16u) {
            Check(runtime.logs.empty() &&
                      runtime.events.back() == "attach",
                  "PrtStd failure returns without logging");
            continue;
        }
        Check(runtime.logs.size() == (continues ? 2u : 1u),
              "failure logs once, UserData also reaches success log");
        std::string format =
            "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCAttachPlugins failed <";
        format += kFailureNames[failed];
        format += ">]\n\n\n";
        CheckLog(runtime.logs[0], format, kFailureLines[failed]);
        if (continues) {
            CheckLog(runtime.logs[1],
                     "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCAttachPlugins success\n\n\n",
                     0x152u);
        }
    }
}

void UserDataThenLaterFailure() {
    Runtime runtime;
    runtime.fail_at = pal4::RenderWarePlugin::UserData;
    runtime.second_fail_at = pal4::RenderWarePlugin::GamePlugin8450;
    pal4::RenderWarePluginInitializer initializer;
    Check(initializer.Initialize(runtime) == 0u,
          "later plugin failure stops after UserData warning");
    Check(runtime.attached.size() == 20u && runtime.logs.size() == 2u,
          "both failure logs emitted without success log");
    CheckLog(runtime.logs[0],
             "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCAttachPlugins failed <RpUserDataPluginAttach>]\n\n\n",
             0xcau);
    CheckLog(runtime.logs[1],
             "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCAttachPlugins failed <RtkDistOptPluginAttach>]\n\n\n",
             0x135u);
}

}  // namespace

int main() {
    AllSuccess();
    EveryFailureBranch();
    UserDataThenLaterFailure();
    std::cout << "24 RenderWare plugin path tests passed\n";
}
