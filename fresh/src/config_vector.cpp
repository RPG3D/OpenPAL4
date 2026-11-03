#include "pal4/config_vector.h"

#include <string>

namespace pal4 {
namespace {

bool MissingNode(const std::uint32_t config, const std::uint32_t node,
                 IConfigVectorRuntime& runtime) {
    return node == runtime.ReadWord(config + 24u) || node == 0xffffffe4u;
}

void AddFloatAttribute(const std::uint32_t config,
                       const std::string_view key, const float value,
                       IConfigVectorRuntime& runtime) {
    const auto attribute = runtime.CreateFloatAttribute(key, value);
    try {
        runtime.AddAttribute(config, attribute);
    } catch (...) {
        runtime.DestroyAttribute(attribute);
        throw;
    }
    runtime.DestroyAttribute(attribute);
}

}  // namespace

std::uint8_t ConfigVector::Set(
    const std::uint32_t config, const std::string_view name,
    const ConfigVector3& value, IConfigVectorRuntime& runtime) {
    const std::string base(name);
    try {
        const auto x_key = base + "_X";
        const auto x_attribute = runtime.GetConfigValue(config, x_key);
        if (x_attribute != 0u)
            runtime.SetFloat(x_attribute, value.x);
        else
            AddFloatAttribute(config, x_key, value.x, runtime);

        const auto y_key = base + "_Y";
        const auto y_node = runtime.FindAttributeNode(config, y_key);
        if (MissingNode(config, y_node, runtime))
            AddFloatAttribute(config, y_key, value.y, runtime);
        else
            runtime.SetFloat(y_node + 28u, value.y);

        const auto z_key = base + "_Z";
        const auto z_node = runtime.FindAttributeNode(config, z_key);
        if (MissingNode(config, z_node, runtime))
            AddFloatAttribute(config, z_key, value.z, runtime);
        else
            runtime.SetFloat(z_node + 28u, value.z);
        return runtime.ReleaseNameBuffer();
    } catch (...) {
        (void)runtime.ReleaseNameBuffer();
        throw;
    }
}

ConfigVector3& ConfigVector::Get(
    ConfigVector3& output, const std::uint32_t config,
    const std::string_view name, IConfigVectorRuntime& runtime) {
    const std::string base(name);
    try {
        const auto x_attribute = runtime.GetConfigValue(config, base + "_X");
        output.x = x_attribute == 0u ? 0.0f :
                   runtime.ToFloat(x_attribute, 0.0f);

        const auto y_node = runtime.FindAttributeNode(config, base + "_Y");
        output.y = MissingNode(config, y_node, runtime) ? 0.0f :
                   runtime.ToFloat(y_node + 28u, 0.0f);

        const auto z_node = runtime.FindAttributeNode(config, base + "_Z");
        output.z = MissingNode(config, z_node, runtime) ? 0.0f :
                   runtime.ToFloat(z_node + 28u, 0.0f);
        (void)runtime.ReleaseNameBuffer();
        return output;
    } catch (...) {
        (void)runtime.ReleaseNameBuffer();
        throw;
    }
}

}  // namespace pal4
