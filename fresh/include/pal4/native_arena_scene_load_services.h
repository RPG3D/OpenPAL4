#pragma once

#include <string_view>

namespace pal4 {
// Shared by Loading-state transitions and archive scene restoration. The host
// supplies the real resource, scene-object and player initialization services.
class INativeArenaSceneLoadServices {
public:
    virtual ~INativeArenaSceneLoadServices() = default;
    virtual void LoadSceneResources(std::string_view world,
                                    std::string_view area) = 0;
    virtual void LoadSceneRuntime(std::string_view resources) = 0;
    virtual void InitializePlayerScene() = 0;
};
}  // namespace pal4
