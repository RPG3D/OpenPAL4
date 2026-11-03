#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

class IRt2dAnimationRuntime {
public:
    virtual ~IRt2dAnimationRuntime() = default;
    [[nodiscard]] virtual char* Allocate(std::size_t size) = 0;
    [[nodiscard]] virtual int Release(char* address) = 0;
    [[nodiscard]] virtual float GameDepth() = 0;
    virtual void CachePath(const char* path) = 0;

    // Rt2d public API boundaries remain injected until native integration.
    virtual void Open2D(std::uint32_t) {}
    virtual void OpenAnimation() {}
    virtual void SetFlatNear(float) {}
    virtual void SetFlatFar(float) {}
    [[nodiscard]] virtual int SetCamera(std::uint32_t) { return 0; }
    virtual void CloseAnimation() {}
    [[nodiscard]] virtual int Close2D() { return 0; }
};

class Rt2dAnimation final {
public:
    // IDA 0x5aa140/0x5aa1b0.
    [[nodiscard]] static char* NormalizePath(
        const char* source, IRt2dAnimationRuntime& runtime);
    [[nodiscard]] static int ReleasePath(
        char* path, IRt2dAnimationRuntime& runtime);
    // IDA 0x5aa1d0/0x5aa220/0x5aa230.
    [[nodiscard]] static int Initialize(IRt2dAnimationRuntime& runtime);
    [[nodiscard]] static int SetCamera(
        std::uint32_t camera, IRt2dAnimationRuntime& runtime);
    [[nodiscard]] static int Close(IRt2dAnimationRuntime& runtime);
};

struct Rt2dMaestroState {
    std::uint32_t maestro = 0;
    std::uint32_t reserved = 0;
    std::uint32_t auxiliary = 0;
};
static_assert(sizeof(Rt2dMaestroState) == 12);

class IRt2dMaestroRuntime {
public:
    virtual ~IRt2dMaestroRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeTextureDictionary() {
        return 0;
    }
    virtual void SetCurrentTextureDictionary(std::uint32_t) {}
    virtual void DestroyMaestro(std::uint32_t) {}
};

class Rt2dMaestroHandle final {
public:
    // IDA 0x5aa240: only the first and third words are reset.
    static void Destroy(Rt2dMaestroState& state,
                        IRt2dMaestroRuntime& runtime);
};

}  // namespace pal4
