#pragma once
#include <memory>
namespace pal4 {
// Owns the two original66CF20 lights and the SDK active-light binding.
// Create after engine start; destroy before engine stop. Lights stay outside worlds.
class NativeNormalMapLights final {
public:
    NativeNormalMapLights();
    ~NativeNormalMapLights();
    NativeNormalMapLights(const NativeNormalMapLights&)=delete;
    NativeNormalMapLights& operator=(const NativeNormalMapLights&)=delete;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
