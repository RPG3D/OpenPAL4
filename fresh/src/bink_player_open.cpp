#include "pal4/bink_player.h"

#include <cstring>
#include <string>
#include <string_view>

namespace pal4 {
namespace {

std::string ResolveArchivePath(
    const std::string_view filename,
    IBinkPlayerOpenRuntime& runtime) {
    std::string path = "gamedata\\";
    path += filename;
    if (runtime.FileExists(path))
        return path;
    const auto root = runtime.InstallRoot();
    path = root + "PAL4\\gamedata\\" + std::string(filename);
    while (!runtime.FileExists(path))
        runtime.PromptForDisk(root.empty() ? '?' : root[0]);
    return path;
}

bool OpenBinkInState(
    std::byte* object, const std::uint32_t source,
    const float volume, const bool use_io_table,
    IBinkPlayerOpenRuntime& runtime) {
    BinkVideoState state{};
    std::memcpy(&state, object + BinkPlayer::kVideoOffset, sizeof(state));
    const auto opened = BinkVideo::Open(
        state, source, volume, use_io_table, runtime);
    std::memcpy(object + BinkPlayer::kVideoOffset, &state, sizeof(state));
    return opened;
}

}  // namespace

bool BinkPlayer::OpenVideoFile(
    std::byte* object, const std::uint32_t source,
    const bool center, IBinkPlayerOpenRuntime& runtime) {
    const auto volume = runtime.VideoMasterVolume();
    if (!runtime.UseVideoPackages()) {
        const auto opened = OpenBinkInState(
            object, source, volume, false, runtime);
        if (opened)
            (void)SetVideoParameters(object, center, runtime);
        return opened;
    }

    const auto video_name = runtime.ScriptVideoName(source);
    auto* first = object + kFirstPackageOffset;
    auto* second = object + kSecondPackageOffset;
    if (object[kFirstPackageOpenFlag] != std::byte{0})
        (void)runtime.ClosePackage(first);
    if (object[kSecondPackageOpenFlag] != std::byte{0})
        (void)runtime.ClosePackage(second);
    runtime.ResetPackageGlobalPointer(first, 1u);
    runtime.ResetPackageGlobalPointer(second, 1u);

    if (!runtime.OpenPackage(
            first, ResolveArchivePath("VideoA.cpk", runtime)))
        return false;
    const auto first_file = runtime.OpenPackageFile(first, video_name);
    if (first_file != 0u) {
        runtime.SeekPackageFile(first, first_file, 0u, 0u);
        const auto opened = OpenBinkInState(
            object, runtime.PackageBinkSource(first), volume, true,
            runtime);
        runtime.ClosePackageFile(first, first_file);
        if (opened)
            (void)SetVideoParameters(object, center, runtime);
        return opened;
    }

    (void)runtime.ClosePackage(first);
    if (!runtime.OpenPackage(
            second, ResolveArchivePath("VideoB.cpk", runtime)))
        return false;
    const auto second_file = runtime.OpenPackageFile(second, video_name);
    if (second_file == 0u) {
        (void)SetVideoParameters(object, center, runtime);
        return true;
    }
    runtime.SeekPackageFile(second, second_file, 0u, 0u);
    const auto opened = OpenBinkInState(
        object, runtime.PackageBinkSource(second), volume, true,
        runtime);
    runtime.ClosePackageFile(second, second_file);
    if (opened)
        (void)SetVideoParameters(object, center, runtime);
    return opened;
}

}  // namespace pal4
