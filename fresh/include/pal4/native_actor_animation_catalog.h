#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pal4 {

class NativePackageArchive;

// Reads the actor's original .tab action list from PALActor.cpk.
class NativeActorAnimationCatalog final {
public:
    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view model);
    [[nodiscard]] bool Load(NativePackageArchive& archive,
                            std::string_view directory,
                            std::string_view model);
    [[nodiscard]] const std::vector<std::pair<std::string, std::string>>&
    Entries() const noexcept { return entries_; }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    std::vector<std::pair<std::string, std::string>> entries_;
    std::string last_error_;
};

}  // namespace pal4
