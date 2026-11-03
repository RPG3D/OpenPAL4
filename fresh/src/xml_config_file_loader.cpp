#include "pal4/xml_config_file_loader.h"

namespace pal4 {
namespace {

class StreamLifetime final {
public:
    explicit StreamLifetime(IXmlConfigFileLoadRuntime& runtime)
        : runtime_(runtime) {
        runtime_.ConstructStream();
    }
    ~StreamLifetime() { runtime_.DestroyStream(); }

private:
    IXmlConfigFileLoadRuntime& runtime_;
};

}  // namespace

std::uint8_t XmlConfigFileLoader::Load(
    const std::uint32_t filename, const std::uint32_t config,
    IXmlConfigFileLoadRuntime& runtime) {
    if (config == 0u) return 0u;
    StreamLifetime stream(runtime);
    if (!runtime.OpenInput(filename, 33u)) runtime.SetStreamError(2u);
    if (runtime.StreamFailed()) return 0u;

    const auto major = runtime.ReadVersionWord();
    const auto minor = runtime.ReadVersionWord();
    if (major > 1 || (major == 1 && minor > 1)) return 0u;

    const auto result = runtime.ReadNodeData(0u, config);
    if (runtime.CloseInput())
        runtime.ResetFileBuffer();
    else
        runtime.SetStreamError(2u);
    return result;
}

}  // namespace pal4
