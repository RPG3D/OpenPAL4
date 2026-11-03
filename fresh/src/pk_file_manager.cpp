#include "pal4/pk_file_manager.h"

#include <array>
#include <utility>

namespace pal4 {
namespace {

void DrainPagedArray(const std::uint32_t object, const std::uint32_t base,
                     const std::uint32_t element_bytes,
                     const std::uint32_t page_bytes,
                     IScriptManagerCoreRuntime& runtime) {
    auto count = runtime.ReadWord(object + base + 40u);
    while (count != 0) {
        const auto next = runtime.ReadWord(object + base + 8u) + element_bytes;
        runtime.WriteWord(object + base + 8u, next);
        runtime.WriteWord(object + base + 40u, --count);
        if (count != 0 && next != runtime.ReadWord(object + base + 4u))
            continue;

        auto cursor = runtime.ReadWord(object + base + 12u);
        const auto page = runtime.ReadWord(cursor);
        cursor += 4u;
        runtime.WriteWord(object + base + 12u, cursor);
        (void)runtime.ReleaseMemory(page);
        if (count != 0) {
            const auto next_page = runtime.ReadWord(cursor);
            runtime.WriteWord(object + base, next_page);
            runtime.WriteWord(object + base + 4u, next_page + page_bytes);
            runtime.WriteWord(object + base + 8u, next_page);
            continue;
        }

        for (std::uint32_t offset = 0; offset <= 28u; offset += 4u)
            runtime.WriteWord(object + base + offset, 0u);
        const auto page_list = runtime.ReadWord(object + base + 32u);
        (void)runtime.ReleaseMemory(page_list);
    }
}

void ClearBackPagedArray(const std::uint32_t object,
                         const std::uint32_t base,
                         const std::uint32_t element_bytes,
                         const std::uint32_t page_bytes,
                         IScriptManagerCoreRuntime& memory) {
    auto count = memory.ReadWord(object + base + 40u);
    while (count != 0) {
        if (memory.ReadWord(object + base + 24u) ==
            memory.ReadWord(object + base + 16u)) {
            const auto old_cursor = memory.ReadWord(object + base + 28u);
            const auto cursor = old_cursor - 4u;
            memory.WriteWord(object + base + 28u, cursor);
            (void)memory.ReleaseMemory(memory.ReadWord(old_cursor));
            const auto page = memory.ReadWord(cursor);
            memory.WriteWord(object + base + 16u, page);
            memory.WriteWord(object + base + 20u, page + page_bytes);
            memory.WriteWord(object + base + 24u, page + page_bytes);
        }

        memory.WriteWord(object + base + 24u,
                         memory.ReadWord(object + base + 24u) - element_bytes);
        memory.WriteWord(object + base + 40u, --count);
        if (count != 0) continue;

        if (base == 32u) {
            (void)PkFileManager::RemoveCallbackPage(object + 28u, memory);
            return;
        }

        const auto old_cursor = memory.ReadWord(object + base + 28u);
        const auto cursor = old_cursor - 4u;
        memory.WriteWord(object + base + 28u, cursor);
        (void)memory.ReleaseMemory(memory.ReadWord(old_cursor));
        const auto front_cursor = memory.ReadWord(object + base + 12u);
        if (front_cursor == cursor)
            (void)memory.ReleaseMemory(memory.ReadWord(front_cursor));
        for (std::uint32_t offset = 0; offset <= 28u; offset += 4u)
            memory.WriteWord(object + base + offset, 0u);
        (void)memory.ReleaseMemory(memory.ReadWord(object + base + 32u));
    }
}

}  // namespace

std::uint32_t PkFileManager::Init() {
    if (initialized_) return manager_;
    initialized_ = true;
    const auto singleton = runtime_.GetSingletonManager();
    manager_ = runtime_.RegisterSingleton(
        singleton, "pkFileMgr", 30u,
        [this] { return Create(); },
        [this](const std::uint32_t object) { return Destroy(object); });
    return manager_;
}

std::uint32_t PkFileManager::Create() {
    auto& memory = runtime_.Scripts();
    const auto object = memory.AllocateMemory(kObjectBytes);
    if (object == 0) return 0;
    if (Construct(object) != 0) return object;
    (void)memory.ReleaseMemory(object);
    return 0;
}

std::uint32_t PkFileManager::Destroy(const std::uint32_t object) {
    if (object == 0) return 0;
    (void)Cleanup(object);
    return runtime_.Scripts().ReleaseMemory(object);
}

std::uint32_t PkFileManager::Construct(const std::uint32_t object) {
    auto& memory = runtime_.Scripts();
    memory.WriteByte(object + 28u, 0u);
    memory.WriteByte(object + 76u, 0u);
    for (const auto base : {32u, 80u}) {
        (void)ResetPageIterator(object + base, memory);
        (void)ResetPageIterator(object + base + 16u, memory);
        for (std::uint32_t offset = 32u; offset <= 40u; offset += 4u)
            memory.WriteWord(object + base + offset, 0u);
    }

    for (std::uint32_t index = 0; index < kScriptCount; ++index) {
        const auto child = object + kFirstScriptOffset + index * kScriptStride;
        if (ScriptManagerCore::Construct(
                child, runtime_.TreeGlobals(), memory) != 0)
            continue;
        while (index != 0) {
            --index;
            (void)ScriptManagerCore::Destruct(
                object + kFirstScriptOffset + index * kScriptStride,
                runtime_.TreeGlobals(), memory);
        }
        return 0;
    }
    memory.WriteWord(object, 0u);
    return object;
}

std::uint32_t PkFileManager::Cleanup(const std::uint32_t object) {
    auto& memory = runtime_.Scripts();
    for (std::uint32_t index = kScriptCount; index != 0;) {
        --index;
        (void)ScriptManagerCore::Destruct(
            object + kFirstScriptOffset + index * kScriptStride,
            runtime_.TreeGlobals(), memory);
    }
    DrainPagedArray(object, 80u, 4u, 4096u, memory);
    DrainPagedArray(object, 32u, 24u, 4080u, memory);
    return 0;
}

std::uint32_t PkFileManager::ProcessFile(
    const std::uint32_t object, const std::uint32_t file) {
    if (file == 0) return 0;
    auto& memory = runtime_.Scripts();
    memory.WriteWord(object, file);
    for (const auto [destination, source] : {
             std::pair{4u, 40u}, std::pair{8u, 44u},
             std::pair{12u, 48u}, std::pair{16u, 68u},
             std::pair{20u, 56u}, std::pair{24u, 76u}})
        memory.WriteWord(object + destination, memory.ReadWord(file + source));

    ClearBackPagedArray(object, 32u, 24u, 4080u, memory);
    return 1;
}

std::uint32_t PkFileManager::LoadAllScripts(
    const std::uint32_t object) {
    auto& memory = runtime_.Scripts();
    if (memory.ReadWord(object) == 0) return 0;
    struct Package {
        const char* path;
        std::uint32_t loader;
    };
    static constexpr std::array<Package, 9> packages{{
        {"gamedata\\PALActor.cpk", 0x670100u},
        {"gamedata\\palobject.cpk", 0x6702e0u},
        {"gamedata\\Script.cpk", 0x670c80u},
        {"gamedata\\scenedata.cpk", 0x6708c0u},
        {"gamedata\\palweapon.cpk", 0x671220u},
        {"gamedata\\ui.cpk", 0x671400u},
        {"gamedata\\Effect.cpk", 0x6715e0u},
        {"gamedata\\MatFX.cpk", 0x6717c0u},
        {"gamedata\\2D.cpk", 0x6719a0u},
    }};
    std::uint32_t result = 0;
    for (std::uint32_t index = 0; index < packages.size(); ++index)
        result = ScriptManagerCore::Load(
            object + kFirstScriptOffset + index * kScriptStride,
            packages[index].path, packages[index].loader, memory);
    return result;
}

std::uint32_t PkFileManager::CleanupAndProcess(
    const std::uint32_t object) {
    auto& memory = runtime_.Scripts();
    memory.WriteWord(object, 0u);
    ClearBackPagedArray(object, 32u, 24u, 4080u, memory);
    ClearBackPagedArray(object, 80u, 4u, 4096u, memory);

    static constexpr std::array<std::uint32_t, 9> order{
        0u, 1u, 2u, 3u, 4u, 9u, 6u, 7u, 8u};
    std::uint32_t result = 0;
    for (const auto index : order)
        result = ScriptManagerCore::CleanupPackage(
            object + kFirstScriptOffset + index * kScriptStride, memory);
    return result;
}

std::uint32_t PkFileManager::GetLastScriptData(
    const std::uint32_t object) {
    auto& memory = runtime_.Scripts();
    if (memory.ReadWord(object) == 0 ||
        memory.ReadWord(object + 120u) == 0)
        return 0;
    const auto back = memory.ReadWord(object + 104u);
    const auto start = memory.ReadWord(object + 96u);
    if (back != start) return memory.ReadWord(back - 4u);
    const auto previous_page =
        memory.ReadWord(memory.ReadWord(object + 108u) - 4u);
    return memory.ReadWord(previous_page + 4096u - 4u);
}

std::uint32_t PkFileManager::LoadScript(
    const char* path, const std::uint32_t loader) {
    auto& memory = runtime_.Scripts();
    const auto object = memory.AllocateMemory(kScriptStride);
    if (object == 0) return 0;
    if (ScriptManagerCore::Construct(
            object, runtime_.TreeGlobals(), memory) == 0) {
        (void)memory.ReleaseMemory(object);
        return 0;
    }
    if (ScriptManagerCore::Load(object, path, loader, memory) != 0)
        return object;
    (void)ScriptManagerCore::Destruct(
        object, runtime_.TreeGlobals(), memory);
    (void)memory.ReleaseMemory(object);
    return 0;
}

std::uint32_t PkFileManager::DestroyScript(
    const std::uint32_t script) {
    if (script == 0) return 0;
    auto& memory = runtime_.Scripts();
    (void)ScriptManagerCore::CleanupPackage(script, memory);
    (void)ScriptManagerCore::Destruct(
        script, runtime_.TreeGlobals(), memory);
    return memory.ReleaseMemory(script);
}

std::uint32_t PkFileManager::AddAndProcessScript(
    const std::uint32_t value) {
    const auto first = Init();
    if (first == 0) return 0;
    (void)AddScriptData(first, value);
    const auto second = Init();
    return second == 0 ? 0 : InstallScriptCallbacks(second);
}

std::uint32_t PkFileManager::AddPrimaryScriptAndProcess() {
    return AddEmbeddedScriptAndProcess(0u);
}

std::uint32_t PkFileManager::AddObjectScriptAndProcess() {
    return AddEmbeddedScriptAndProcess(1u);
}

std::uint32_t PkFileManager::AddGlobalScriptAndProcess() {
    return AddEmbeddedScriptAndProcess(kGlobalScriptIndex);
}

std::uint32_t PkFileManager::AddSceneScriptAndProcess() {
    return AddEmbeddedScriptAndProcess(3u);
}

std::uint32_t PkFileManager::AddWeaponScriptAndProcess() {
    return AddEmbeddedScriptAndProcess(4u);
}

std::uint32_t PkFileManager::AddUiScriptAndProcess() {
    return AddEmbeddedScriptAndProcess(5u);
}

std::uint32_t PkFileManager::AddEffectScriptAndProcess() {
    return AddEmbeddedScriptAndProcess(6u);
}

std::uint32_t PkFileManager::AddMatFxScriptAndProcess() {
    return AddEmbeddedScriptAndProcess(7u);
}

std::uint32_t PkFileManager::Add2dScriptAndProcess() {
    return AddEmbeddedScriptAndProcess(8u);
}

std::uint32_t PkFileManager::AddEmbeddedScriptAndProcess(
    const std::uint32_t index) {
    const auto first = Init();
    if (first == 0) return 0;
    (void)AddScriptData(
        first, first + kFirstScriptOffset + index * kScriptStride);
    const auto second = Init();
    return second == 0 ? 0 : InstallScriptCallbacks(second);
}

std::uint32_t PkFileManager::CleanupAllScripts() {
    const auto first = Init();
    if (first == 0) return 0;
    (void)RemoveScriptData(first);
    const auto second = Init();
    return second == 0 ? 0 : PopFileCallbacks(second);
}

std::uint32_t PkFileManager::GetScriptSize(
    const std::uint32_t file) {
    const auto manager = Init();
    if (manager == 0) return 0;
    const auto script = GetLastScriptData(manager);
    if (script != 0) {
        const auto handle = ScriptManagerCore::GetScript(
            script, file, runtime_.Scripts());
        return handle == 0 ? 0 : runtime_.Scripts().ReadWord(handle + 36u);
    }
    const auto position = runtime_.NativeFilePosition(file);
    runtime_.NativeFileSeek(file, 0, 2);
    const auto length = runtime_.NativeFilePosition(file);
    runtime_.NativeFileSeek(file, static_cast<std::int32_t>(position), 0);
    return length;
}

}  // namespace pal4
