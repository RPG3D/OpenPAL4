#include "pal4/effect_system.h"

#include <cctype>
#include <string>

namespace pal4 {
namespace {

class ScriptScope final {
public:
    explicit ScriptScope(IEffectSystemCreateRuntime& runtime)
        : runtime_(runtime) { runtime_.BeginScriptScope(); }
    ~ScriptScope() { runtime_.EndScriptScope(); }

private:
    IEffectSystemCreateRuntime& runtime_;
};

class TemporaryHandle final {
public:
    TemporaryHandle(IEffectSystemCreateRuntime& runtime,
                    std::uint32_t id, std::uint32_t system)
        : runtime_(runtime), value_(runtime.CopyHandle(id, system)) {}
    ~TemporaryHandle() { runtime_.ResetTemporaryHandle(value_); }
    [[nodiscard]] const EffectSystemHandle& Value() const { return value_; }

private:
    IEffectSystemCreateRuntime& runtime_;
    EffectSystemHandle value_;
};

void SetName(std::uint32_t xml, std::string_view name,
             IEffectSystemCreateRuntime& runtime) {
    const auto attribute = runtime.FindXmlAttribute(xml, "NAME");
    if (attribute != 0u)
        runtime.SetXmlString(attribute, name);
    else
        runtime.AddXmlString(xml, "NAME", name);
}

void SetId(std::uint32_t xml, std::uint32_t id,
           IEffectSystemCreateRuntime& runtime) {
    const auto attribute = runtime.FindXmlAttribute(xml, "ID");
    if (attribute != 0u)
        runtime.SetXmlInt(attribute, id);
    else
        runtime.AddXmlInt(xml, "ID", id);
}

EffectSystemHandle Create(std::uint32_t system, std::string_view name,
                          std::uint32_t position, std::uint32_t repeat_count,
                          std::uint8_t flag, bool pause,
                          IEffectSystemCreateRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return {};

    ScriptScope script(runtime);
    std::string normalized(name);
    if (runtime.UppercaseNames()) {
        for (auto& ch : normalized)
            ch = static_cast<char>(std::toupper(
                static_cast<unsigned char>(ch)));
    }
    const auto effect = runtime.AcquireEffect(normalized);
    if (effect == 0u) return {};

    const auto xml = effect + 20u;
    SetName(xml, normalized, runtime);
    runtime.WriteWord(effect + 12u, system);
    runtime.SetPosition(effect, position, 0u);
    runtime.WriteByte(effect + 83u, flag);
    if (!runtime.StartPlayback(effect)) {
        runtime.ReturnEffect(effect, 1u);
        return {};
    }
    if (pause && !runtime.PausePlayback(effect)) {
        runtime.ReturnEffect(effect, 1u);
        return {};
    }

    const auto id = runtime.ReadWord(system + 8u);
    runtime.WriteWord(system + 8u, id + 1u);
    runtime.InsertEffect(system + 12u, id, effect, repeat_count);

    if (pause) SetId(xml, id, runtime);
    TemporaryHandle result(runtime, id, system);
    const auto output = result.Value();
    if (!pause) SetId(xml, id, runtime);
    {
        TemporaryHandle callback(runtime, id, system);
        runtime.NotifyCreated(system, callback.Value(), position,
                              repeat_count);
    }
    return output;
}

}  // namespace

EffectSystemHandle EffectSystem::CreateEffectObject(
    std::uint32_t system, std::string_view name,
    std::uint32_t position, std::uint32_t repeat_count,
    std::uint8_t flag, IEffectSystemCreateRuntime& runtime) {
    return Create(system, name, position, repeat_count, flag, true, runtime);
}

EffectSystemHandle EffectSystem::CreateEffectObjectWithoutPause(
    std::uint32_t system, std::string_view name,
    std::uint32_t position, std::uint32_t repeat_count,
    std::uint8_t flag, IEffectSystemCreateRuntime& runtime) {
    return Create(system, name, position, repeat_count, flag, false, runtime);
}

}  // namespace pal4
