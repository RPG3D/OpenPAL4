#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class UtilitiesBindingSchema {
    IdString,
    PreviewInfo,
    Introduction,
    Scene,
    Window,
    StringId,
    World,
};

class IUtilitiesTableBindingRuntime {
public:
    virtual ~IUtilitiesTableBindingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void RegisterFields(std::uint32_t binding,
                                UtilitiesBindingSchema schema) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
};

class UtilitiesTableBindingRefresh final {
public:
    // IDA 0x508710: refresh the ID/string field binding.
    static void IdString(std::uint32_t object,
                         IUtilitiesTableBindingRuntime& runtime);
    // IDA 0x508940: refresh the PreviewInfo field binding.
    static void PreviewInfo(std::uint32_t object,
                            IUtilitiesTableBindingRuntime& runtime);
    // IDA 0x508b70: refresh the Introduction field binding.
    static void Introduction(std::uint32_t object,
                             IUtilitiesTableBindingRuntime& runtime);
    // IDA 0x508da0: refresh the Scene field binding.
    static void Scene(std::uint32_t object,
                      IUtilitiesTableBindingRuntime& runtime);
    // IDA 0x508fd0: refresh the Window field binding.
    static void Window(std::uint32_t object,
                       IUtilitiesTableBindingRuntime& runtime);
    // IDA 0x509200: refresh the StringId field binding.
    static void StringId(std::uint32_t object,
                         IUtilitiesTableBindingRuntime& runtime);
    // IDA 0x509430: refresh the World field binding.
    static void World(std::uint32_t object,
                      IUtilitiesTableBindingRuntime& runtime);
};

}  // namespace pal4
