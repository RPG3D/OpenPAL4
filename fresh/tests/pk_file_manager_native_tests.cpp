#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_csb_buffer_runtime.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t file_cursor = 0u;
std::array<std::int32_t, 4> file_call_trace{};
std::size_t file_call_count = 0u;

std::uint32_t Position(const std::uint32_t file) {
    Check(file == 0x1234u, "loose file handle is forwarded");
    file_call_trace[file_call_count++] = 1;
    return file_cursor;
}

void Seek(const std::uint32_t file, const std::int32_t offset,
          const std::int32_t origin) {
    Check(file == 0x1234u, "loose seek handle is forwarded");
    file_call_trace[file_call_count++] = origin == 2 ? 2 : 3;
    file_cursor = origin == 2 ? 101u : static_cast<std::uint32_t>(offset);
}

}  // namespace

int main(const int argc, char** argv) {
    bool incomplete_api_rejected = false;
    try {
        pal4::NativePkFileManagerRuntime incomplete({&Position, nullptr});
    } catch (const std::invalid_argument&) {
        incomplete_api_rejected = true;
    }
    Check(incomplete_api_rejected, "incomplete native file API is rejected");
    pal4::NativePkFileManagerRuntime loose_runtime({&Position, &Seek});
    file_cursor = 7u;
    Check(loose_runtime.GetScriptSize(0x1234u) == 101u &&
              loose_runtime.ManagerAddress() != 0u &&
              file_cursor == 7u && file_call_count == 4u &&
              file_call_trace == std::array<std::int32_t, 4>{1, 2, 1, 3},
          "loose file size follows tell, seek-end, tell, restore");
    loose_runtime.Shutdown();
    pal4::NativePkFileManagerRuntime runtime;
    const auto manager = runtime.InitializePackFileManager();
    Check(manager != 0u && manager == runtime.ManagerAddress() &&
          runtime.InitializePackFileManager() == manager,
          "native PK singleton creates and caches the manager");
    bool missing_file_api_rejected = false;
    try {
        (void)runtime.GetScriptSize(0x1234u);
    } catch (const std::logic_error&) {
        missing_file_api_rejected = true;
    }
    Check(missing_file_api_rejected,
          "unbound loose file API fails explicitly");
    Check(runtime.ScriptInstanceCount() == pal4::PkFileManager::kScriptCount &&
          runtime.ArchiveCount() == pal4::PkFileManager::kScriptCount,
          "native PK manager constructs eleven embedded package objects");

    std::array<std::uint32_t, 20> file{};
    file[10] = 0x1010u;
    file[11] = 0x1111u;
    file[12] = 0x1212u;
    file[17] = 0x1717u;
    file[14] = 0x1414u;
    file[19] = 0x1919u;
    const auto file_address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(file.data()));
    runtime.ProcessPackFiles(manager, file_address);
    Check(runtime.BoundFileSystem() == file_address,
          "native PK manager binds the selected filesystem");
    const auto* copied = reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(manager));
    Check(copied[1] == file[10] && copied[2] == file[11] &&
          copied[3] == file[12] && copied[4] == file[17] &&
          copied[5] == file[14] && copied[6] == file[19],
          "native PK manager stores original filesystem callbacks");
    Check(runtime.AddPrimaryScriptAndProcess() == file_address &&
              copied[18] == 1u && copied[30] == 1u,
          "primary script scope installs one callback and script record");
    Check(file[10] != 0u && file[10] != 0x66e9a0u &&
              file[11] != 0u && file[11] != 0x66e9e0u &&
              file[12] != 0u && file[12] != 0x66ea10u &&
              file[17] != 0u && file[17] != 0x66ea40u &&
              file[14] != 0u && file[14] != 0x66ea60u &&
              file[19] != 0u && file[19] != 0x66ead0u,
          "primary scope installs callable native script callbacks");
    const auto script_callbacks=file;
    runtime.PushFilesystemAccess();
    Check(runtime.FilesystemAccessDepth()==1u && copied[18]==2u && copied[30]==1u &&
              file[10]==0x1010u && file[19]==0x1919u,
        "native filesystem scope restores original callbacks without retiring the active script");
    runtime.PushFilesystemAccess();
    Check(runtime.FilesystemAccessDepth()==2u && copied[18]==3u,"native filesystem scopes support nesting");
    bool scope_cleanup_rejected=false;
    try{(void)runtime.CleanupAllScripts();}catch(const std::logic_error&){scope_cleanup_rejected=true;}
    Check(scope_cleanup_rejected && copied[18]==3u && copied[30]==1u,"script cleanup cannot pop a borrowed filesystem scope");
    runtime.PopFilesystemAccess();runtime.PopFilesystemAccess();
    Check(!runtime.FilesystemAccessDepth() && copied[18]==1u && file==script_callbacks,
        "native filesystem pop restores all actual script callbacks and original stack depth");
    bool unmatched_pop_rejected=false;
    try{runtime.PopFilesystemAccess();}catch(const std::logic_error&){unmatched_pop_rejected=true;}
    Check(unmatched_pop_rejected && file==script_callbacks,"unmatched filesystem pop preserves active script callbacks");
    Check(runtime.CleanupAllScripts() != 0u &&
              copied[18] == 0u && copied[30] == 0u &&
              file[10] == 0x1010u && file[11] == 0x1111u &&
              file[12] == 0x1212u && file[17] == 0x1717u &&
              file[14] == 0x1414u && file[19] == 0x1919u,
          "primary scope restores all six file callbacks and releases records");
    Check(runtime.AddSceneScriptAndProcess() == file_address &&
              copied[18] == 1u && copied[30] == 1u &&
              file[10] != 0x1010u,
          "scene script scope installs native callbacks");
    Check(runtime.CleanupAllScripts() != 0u &&
              copied[18] == 0u && copied[30] == 0u &&
              file[10] == 0x1010u,
          "scene script scope restores native callbacks");
    if (argc == 2) {
        const auto original_directory = std::filesystem::current_path();
        std::filesystem::current_path(argv[1]);
        Check(runtime.LoadAllScripts() == 1u &&
              runtime.LoadedPackageCount() == 9u,
              "nine game script packages open from original installation");
        Check(runtime.LoadAllScripts() == 1u &&
              runtime.LoadedPackageCount() == 9u,
              "reloading the same script paths reuses open packages");
        Check(runtime.AddPrimaryScriptAndProcess() == file_address,
              "primary PALActor script scope opens on the original packages");
        using LoadCallback = std::uint32_t (__cdecl *)(
            std::uint32_t, std::uint32_t, const char*);
        using ExistsCallback = std::uint32_t (__cdecl *)(
            std::uint32_t, const char*);
        using ReadCallback = std::uint32_t (__cdecl *)(
            std::uint32_t, void*, std::uint32_t);
        using StateCallback = std::uint32_t (__cdecl *)(std::uint32_t);
        using SeekCallback = std::uint64_t (__cdecl *)(
            std::uint32_t, std::int32_t, std::uint32_t);
        const auto load = reinterpret_cast<LoadCallback>(file[10]);
        const auto exists = reinterpret_cast<ExistsCallback>(file[19]);
        const auto read = reinterpret_cast<ReadCallback>(file[12]);
        const auto loaded = reinterpret_cast<StateCallback>(file[17]);
        const auto unload = reinterpret_cast<StateCallback>(file[11]);
        const auto seek = reinterpret_cast<SeekCallback>(file[14]);
        constexpr auto actor_path = "gamedata\\PALActor\\101\\C01.anm";
        constexpr std::uint32_t actor_key = 0x81010001u;
        const auto actor_exists = exists(0u, actor_path);
        const auto actor_load = load(0u, actor_key, actor_path);
        const auto actor_loaded = loaded(actor_key);
        if (actor_exists != 1u || actor_load != 1u || actor_loaded != 0u)
            std::fprintf(stderr, "actor callback: exists=%u load=%u loaded=%u\n",
                         actor_exists, actor_load, actor_loaded);
        Check(actor_exists == 1u && actor_load == 1u && actor_loaded == 0u,
              "native callbacks find and register an original actor clip");
        std::array<std::uint8_t, 12> actor_prefix{};
        Check(read(actor_key, actor_prefix.data(), actor_prefix.size()) ==
                  actor_prefix.size() &&
                  seek(actor_key, 0, 1u) == 0u &&
                  read(actor_key, actor_prefix.data(), actor_prefix.size()) ==
                      actor_prefix.size() &&
                  seek(actor_key, 0, 3u) > actor_prefix.size() &&
                  loaded(actor_key) == 1u && unload(actor_key) != 0u &&
                  loaded(actor_key) == 0u,
              "native callbacks read, seek and close the original clip");
        Check(runtime.CleanupAllScripts() != 0u &&
                  file[10] == 0x1010u && file[19] == 0x1919u,
              "primary scope restores callbacks after original asset use");
        Check(runtime.AddSceneScriptAndProcess() == file_address,
              "scene script scope opens on original scene data package");
        constexpr auto floor_path =
            "gamedata\\editData\\scene\\Q01\\N01\\N01_floor.dff";
        constexpr std::uint32_t floor_key = 0x81010002u;
        Check(exists(0u, floor_path) == 1u &&
                  load(0u, floor_key, floor_path) == 1u,
              "scene callbacks resolve original Q01/N01 floor clump");
        std::array<std::uint8_t, 12> floor_header{};
        Check(read(floor_key, floor_header.data(), floor_header.size()) ==
                  floor_header.size(),
              "scene callback reads original floor clump header");
        std::uint32_t chunk_type{};
        std::memcpy(&chunk_type, floor_header.data(), sizeof(chunk_type));
        Check(chunk_type == 0x10u &&
                  seek(floor_key, 0, 1u) == 0u &&
                  unload(floor_key) != 0u,
              "scene callback reads a RenderWare clump and closes it");
        Check(runtime.CleanupAllScripts() != 0u &&
                  file[10] == 0x1010u && file[19] == 0x1919u,
              "scene scope restores callbacks after original asset use");
        const auto script = manager + pal4::PkFileManager::kFirstScriptOffset +
                            2u * pal4::PkFileManager::kScriptStride +
                            pal4::ScriptManagerCore::kPackageOffset;
        const auto ui = manager + pal4::PkFileManager::kFirstScriptOffset +
                        5u * pal4::PkFileManager::kScriptStride +
                        pal4::ScriptManagerCore::kPackageOffset;
        Check(*reinterpret_cast<const std::uint32_t*>(
                  static_cast<std::uintptr_t>(script + 40u)) == 74u &&
              *reinterpret_cast<const std::uint32_t*>(
                  static_cast<std::uintptr_t>(ui + 40u)) == 547u,
              "embedded script and UI packages hold original indexes");
        const auto global_script = runtime.OpenGlobalScriptPath(
            "gamedata\\editData\\script\\script.csb");
        Check(global_script != 0u &&
                  runtime.Scripts().ReadWord(global_script + 36u) == 0x280u,
              "game startup CSB path resolves to original script package");
        std::uint32_t declared_bytes = 0u;
        runtime.Scripts().ReadPackageData(
            &declared_bytes, sizeof(declared_bytes), global_script);
        Check(declared_bytes == 0x27cu &&
                  runtime.Scripts().CloseScriptHandle(script, global_script),
              "global CSB opens, reads and closes through PK manager");
        {
            pal4::NativeCsbBufferRuntime csb_runtime(runtime);
            std::uint32_t payload_size = 99u;
            std::uint32_t payload = 77u;
            Check(!pal4::CsbBufferReader::Read(
                      "gamedata\\editData\\script\\missing.csb",
                      payload_size, payload, csb_runtime) &&
                      payload_size == 99u && payload == 77u,
                  "missing global CSB preserves callback outputs");
            Check(pal4::CsbBufferReader::Read(
                      "gamedata\\editData\\script\\script.csb",
                      payload_size, payload, csb_runtime) &&
                      payload_size == 0x27cu && payload != 0u,
                  "native CSB reader opens original package payload");
            std::uint32_t first_payload_word = 1u;
            std::memcpy(&first_payload_word,
                        reinterpret_cast<const void*>(
                            static_cast<std::uintptr_t>(payload)),
                        sizeof(first_payload_word));
            const std::string_view payload_text(
                reinterpret_cast<const char*>(
                    static_cast<std::uintptr_t>(payload)), payload_size);
            Check(payload_text.find("NewGame") != std::string_view::npos &&
                      payload_text.find("1A.bik") != std::string_view::npos &&
                      payload_text.find("Pal4A.bik") != std::string_view::npos &&
                      payload_text.find("Q01") != std::string_view::npos &&
                      payload_text.find("N01") != std::string_view::npos,
                  "original NewGame payload contains movie and scene names");
            const auto reopened = runtime.OpenGlobalScriptPath(
                "gamedata\\editData\\script\\script.csb");
            Check(first_payload_word == 0u && reopened != 0u &&
                      runtime.CloseGlobalScriptMember(reopened),
                  "native CSB reader skips length prefix and closes member");
            pal4::NativeCsbBufferRuntime::FreePayload(payload);
        }
        const auto handle = runtime.OpenScriptMember(
            5u, "fonts\\fontPuzzle_0.imageset");
        Check(handle != 0u, "open original UI member through PK script package");
        auto& scripts = runtime.Scripts();
        const auto member_size = scripts.ReadWord(handle + 36u);
        Check(member_size > 0u && member_size < 0x100000u,
              "PK script member reports original decoded size");
        std::vector<char> member(member_size);
        scripts.ReadPackageData(member.data(), member_size, handle);
        const std::string_view xml(member.data(), member.size());
        Check(xml.find("<Imageset") != std::string_view::npos &&
              scripts.ReadWord(handle + 40u) == member_size,
              "PK script member reads original XML and advances cursor");
        Check(scripts.SeekPackage(ui, handle, 0, 0) == 0,
              "PK script member seeks to start");
        char prefix[8]{};
        scripts.ReadPackageData(prefix, sizeof(prefix), handle);
        Check(std::string_view(prefix, sizeof(prefix)) == xml.substr(0, 8) &&
              scripts.SeekPackage(ui, handle, 4, 2) ==
                  static_cast<std::int32_t>(member_size - 4u),
              "PK script member read and original end-origin seek");
        const auto ui_script = manager +
            pal4::PkFileManager::kFirstScriptOffset +
            5u * pal4::PkFileManager::kScriptStride;
        constexpr std::uint32_t key = 0x80000001u;
        const auto inserted = pal4::ScriptManagerCore::InsertScript(
            ui_script, key, handle, scripts);
        Check(inserted != 0u && scripts.ReadWord(inserted) != 0u &&
              pal4::ScriptManagerCore::GetScript(ui_script, key, scripts) ==
                  handle && (handle & 0xffffff00u) != 0u,
              "script tree retains the complete native member handle");
        Check(pal4::ScriptManagerCore::SeekScript(
                  ui_script, key, 0, 0, scripts) == 0 &&
              pal4::ScriptManagerCore::ReadScript(
                  ui_script, key, prefix, sizeof(prefix), scripts) ==
                  sizeof(prefix) &&
              std::string_view(prefix, sizeof(prefix)) == xml.substr(0, 8),
              "script manager reads original XML through keyed tree lookup");
        for (const auto extra_key : {1u, 3u, 2u, 4u})
            Check(pal4::ScriptManagerCore::InsertScript(
                      ui_script, extra_key, handle, scripts) != 0u &&
                  pal4::ScriptManagerCore::GetScript(
                      ui_script, extra_key, scripts) == handle,
                  "native script tree balances multiple unsigned keys");
        const auto removed = pal4::ScriptManagerCore::RemoveScript(
            ui_script, key, scripts);
        Check(removed != 0u &&
              pal4::ScriptManagerCore::GetScript(ui_script, key, scripts) == 0u &&
              pal4::ScriptManagerCore::GetScript(ui_script, 2u, scripts) == handle,
              "script manager removes a keyed handle without losing peers");
        for (const auto extra_key : {1u, 3u, 2u, 4u})
            Check(pal4::ScriptManagerCore::RemoveScript(
                      ui_script, extra_key, scripts) != 0u &&
                  pal4::ScriptManagerCore::GetScript(
                      ui_script, extra_key, scripts) == 0u,
                  "native script tree deletes every remaining key");
        Check(scripts.CloseScriptHandle(ui, handle) == 1u &&
              scripts.CloseScriptHandle(ui, handle) == 0u,
              "PK script member closes once and invalidates its handle");
        const auto second_handle = runtime.OpenScriptMember(
            5u, "fonts\\fontPuzzle_0.imageset");
        Check(second_handle != 0u &&
              pal4::ScriptManagerCore::InsertScript(
                  ui_script, 2u, second_handle, scripts) != 0u &&
              pal4::ScriptManagerCore::GetScript(ui_script, 2u, scripts) ==
                  second_handle &&
              pal4::ScriptManagerCore::CloseScript(
                  ui_script, 2u, scripts) == 1u,
              "native script handle closes through script manager facade");
        std::filesystem::current_path(original_directory);
    }
    Check(runtime.AddPrimaryScriptAndProcess() == file_address &&
              file[10] != 0x1010u,
          "shutdown fixture has an active primary script scope");
    runtime.Shutdown();
    Check(runtime.ManagerAddress() == 0u &&
          runtime.ScriptInstanceCount() == 0u &&
          runtime.ArchiveCount() == 0u &&
          runtime.AllocationCount() == 0u &&
          runtime.LoadedPackageCount() == 0u &&
          file[10] == 0x1010u && file[19] == 0x1919u,
          "native PK shutdown destroys children and shared tree nil");
    Check(runtime.InitializePackFileManager() != 0u &&
          runtime.ScriptInstanceCount() == pal4::PkFileManager::kScriptCount,
          "native PK manager can restart after shutdown");
    runtime.Shutdown();
    Check(runtime.AllocationCount() == 0u && runtime.ArchiveCount() == 0u,
          "restarted PK manager releases its owned allocations");
    return EXIT_SUCCESS;
}
