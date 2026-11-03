#include "pal4/script_process_and_cleanup.h"

namespace pal4 {

std::uint32_t ScriptProcessRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
std::uint32_t ScriptProcessRuntimeStub::EmptyStringAddress() {
    return 0u;
}
std::int32_t ScriptProcessRuntimeStub::CompareCaseInsensitive(
    std::uint32_t, std::uint32_t) { return 0; }
void ScriptProcessRuntimeStub::CopyName(char*, std::uint32_t,
                                        std::uint32_t) {}
void ScriptProcessRuntimeStub::ResetObject(std::uint32_t) {}
void ScriptProcessRuntimeStub::AddAndProcessScript() {}
void ScriptProcessRuntimeStub::InitializeObject(
    std::uint32_t, const char*, std::uint32_t, std::uint32_t) {}
void ScriptProcessRuntimeStub::CleanupAllScripts() {}
void ScriptProcessRuntimeStub::ApplyFirst(
    std::uint32_t, const std::array<std::uint32_t, 3>&,
    std::uint32_t) {}
std::int32_t ScriptProcessRuntimeStub::ApplySecond(
    std::uint32_t, const std::array<std::uint32_t, 3>&,
    std::uint32_t) { return 0; }

bool ScriptProcessAndCleanup::NamesDiffer(
    std::uint32_t object, IScriptProcessRuntime& runtime) {
    auto script_name = runtime.ReadDword(object + 0xa1cu);
    if (script_name == 0u)
        script_name = runtime.EmptyStringAddress();
    return runtime.CompareCaseInsensitive(
        runtime.ReadDword(object + 0xcu), script_name) != 0;
}

std::int32_t ScriptProcessAndCleanup::Process(
    std::uint32_t object, IScriptProcessRuntime& runtime) {
    if (!NamesDiffer(object, runtime))
        return 0;

    std::array<char, 0x100u> name{};
    runtime.CopyName(name.data(), runtime.ReadDword(object + 4u),
                     static_cast<std::uint32_t>(name.size()));
    std::array<std::uint32_t, 3> second{};
    std::array<std::uint32_t, 3> first{};
    for (std::uint32_t index = 0u; index < 3u; ++index)
        second[index] = runtime.ReadDword(object + 0xacu + index * 4u);
    for (std::uint32_t index = 0u; index < 3u; ++index)
        first[index] = runtime.ReadDword(object + 0xa0u + index * 4u);

    runtime.ResetObject(object);
    runtime.AddAndProcessScript();
    auto script_name = runtime.ReadDword(object + 0xa1cu);
    if (script_name == 0u)
        script_name = runtime.EmptyStringAddress();
    runtime.InitializeObject(object, name.data(), script_name, 1u);
    runtime.CleanupAllScripts();
    runtime.ApplyFirst(object, second, 0u);
    return runtime.ApplySecond(object, first, 0u);
}

}  // namespace pal4
