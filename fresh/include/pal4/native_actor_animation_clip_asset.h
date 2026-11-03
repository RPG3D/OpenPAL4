#pragma once

#include <filesystem>
#include <string>
#include <string_view>

struct RtAnimAnimation;

namespace pal4 {

class NativePackageArchive;

// Owns one RenderWare animation streamed from an original CPK. The explicit
// archive/directory overload also supports nested Effect clump directories.
class NativeActorAnimationClipAsset final {
public:
    NativeActorAnimationClipAsset() = default;
    ~NativeActorAnimationClipAsset();
    NativeActorAnimationClipAsset(const NativeActorAnimationClipAsset&) = delete;
    NativeActorAnimationClipAsset& operator=(
        const NativeActorAnimationClipAsset&) = delete;

    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view model,
                            std::string_view filename);
    [[nodiscard]] bool Load(NativePackageArchive& archive,
                            std::string_view model,
                            std::string_view filename);
    [[nodiscard]] bool Load(NativePackageArchive& archive,
                            std::string_view directory,
                            std::string_view model,
                            std::string_view filename);
    void Unload() noexcept;
    [[nodiscard]] RtAnimAnimation* Animation() const noexcept {
        return animation_;
    }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    RtAnimAnimation* animation_ = nullptr;
    std::string last_error_;
};

}  // namespace pal4
