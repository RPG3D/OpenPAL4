#pragma once
#include "pal4/portable/game_camera.h"
#include "pal4/portable/script_stack.h"
#include <optional>

namespace pal4::portable {
using CameraMemberReader=std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)>;
struct CameraTrackCompletion {ScriptToken token;std::string error;};
class CameraTracks {
public:
    CameraTracks(GameCameraSystem& cameras,CameraMemberReader read,bool retain_source_for_diagnostics=false);
    ~CameraTracks();
    CameraTracks(const CameraTracks&)=delete;
    CameraTracks& operator=(const CameraTracks&)=delete;
    ScriptResponse Prepare(std::string_view world,std::string_view scene,std::string_view name);
    ScriptResponse RunSingle(ScriptToken token,std::string_view name,bool wait);
    ScriptResponse WaitCurrent(ScriptToken token);
    std::vector<CameraTrackCompletion> Advance(float elapsed);
    bool Playing() const;
    bool Waiting() const;
    void WriteReport(std::ostream& output,bool include_source=true) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
