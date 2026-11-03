#pragma once

#include "pal4/game_object_animation_event_data.h"
#include "pal4/animation_string_table_collection.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

// Reads original .amf events and optional .amd parameters from PALActor.cpk.
class NativeActorAnimationEvents final {
public:
    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view model,
                            std::string_view action_resource);
    [[nodiscard]] const std::vector<AnimationEventData>& Events() const noexcept {
        return events_;
    }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }
    [[nodiscard]] bool SidecarLoaded() const noexcept { return sidecar_loaded_; }
    [[nodiscard]] const std::vector<AnimationStringTableData>& SidecarTables() const noexcept {
        return sidecar_tables_;
    }

private:
    std::vector<AnimationEventData> events_;
    std::vector<AnimationStringTableData> sidecar_tables_;
    bool sidecar_loaded_ = false;
    std::string last_error_;
};

}  // namespace pal4
