#include "pal4/rt2d_animation.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct AnimationRuntime final : pal4::IRt2dAnimationRuntime {
    std::vector<std::string> calls;
    std::string cached;
    std::size_t allocation_size = 0;
    float near_depth = 0;
    float far_depth = 0;
    std::uint32_t camera = 0;
    bool fail_allocation = false;
    bool cached_null = false;

    char* Allocate(std::size_t size) override {
        calls.emplace_back("allocate");
        allocation_size = size;
        return fail_allocation ? nullptr : new char[size];
    }
    int Release(char* path) override {
        calls.emplace_back("release");
        delete[] path;
        return 73;
    }
    float GameDepth() override {
        calls.emplace_back("depth");
        return 7.5f;
    }
    void CachePath(const char* path) override {
        calls.emplace_back("cache");
        cached_null = path == nullptr;
        if (path != nullptr) cached = path;
    }
    void Open2D(std::uint32_t value) override {
        Check(value == 0, "Rt2dOpen argument");
        calls.emplace_back("open 2d");
    }
    void OpenAnimation() override { calls.emplace_back("open animation"); }
    void SetFlatNear(float value) override {
        calls.emplace_back("near");
        near_depth = value;
    }
    void SetFlatFar(float value) override {
        calls.emplace_back("far");
        far_depth = value;
    }
    int SetCamera(std::uint32_t value) override {
        calls.emplace_back("camera");
        camera = value;
        return -17;
    }
    void CloseAnimation() override {
        calls.emplace_back("close animation");
    }
    int Close2D() override {
        calls.emplace_back("close 2d");
        return 41;
    }
};

struct MaestroRuntime final : pal4::IRt2dMaestroRuntime {
    std::vector<std::string> calls;
    std::uint32_t dictionary = 0;
    std::uint32_t destroyed = 0;
    std::uint32_t InitializeTextureDictionary() override {
        calls.emplace_back("initialize dictionary");
        return 0x5000u;
    }
    void SetCurrentTextureDictionary(std::uint32_t value) override {
        calls.emplace_back("set dictionary");
        dictionary = value;
    }
    void DestroyMaestro(std::uint32_t value) override {
        calls.emplace_back("destroy maestro");
        destroyed = value;
    }
};

void TestPathAndCleanup() {
    AnimationRuntime runtime;
    char* path = pal4::Rt2dAnimation::NormalizePath("a/b//c", runtime);
    Check(path != nullptr && std::string(path) == "a\\b\\\\c",
          "all forward slashes normalized");
    Check(runtime.allocation_size == 7u, "allocate includes terminator");
    Check(pal4::Rt2dAnimation::ReleasePath(path, runtime) == 73,
          "release result forwarded");
    Check(pal4::Rt2dAnimation::ReleasePath(nullptr, runtime) == 0,
          "null release returns zero");
    Check(runtime.calls == std::vector<std::string>({"allocate", "release"}),
          "null release skips allocator");

    runtime.fail_allocation = true;
    Check(pal4::Rt2dAnimation::NormalizePath("x/y", runtime) == nullptr,
          "allocation failure returns null");
}

void TestInitializationAndBridges() {
    AnimationRuntime runtime;
    Check(pal4::Rt2dAnimation::Initialize(runtime) == 73,
          "initialize returns release result");
    Check(runtime.cached == "gamedata\\animation\\font\\",
          "font animation directory cached after normalization");
    Check(runtime.allocation_size == 25u, "font path allocation length");
    Check(std::fabs(runtime.near_depth - 8.5f) < 0.0001f,
          "near depth is game depth plus one");
    Check(runtime.far_depth == 32.0f, "far depth is 32");
    Check(runtime.calls == std::vector<std::string>({
              "open 2d", "open animation", "depth", "near", "far",
              "allocate", "cache", "release"}),
          "initialization order");
    Check(pal4::Rt2dAnimation::SetCamera(0x7777u, runtime) == -17 &&
              runtime.camera == 0x7777u, "camera thunk forwards argument and result");
    Check(pal4::Rt2dAnimation::Close(runtime) == 41,
          "close forwards the 2d close result");
    Check(runtime.calls[runtime.calls.size() - 2] == "close animation" &&
              runtime.calls.back() == "close 2d", "close order");

    AnimationRuntime failed;
    failed.fail_allocation = true;
    Check(pal4::Rt2dAnimation::Initialize(failed) == 0 && failed.cached_null,
          "failed allocation still reaches cache call and skips release");
}

void TestMaestroDestroy() {
    MaestroRuntime runtime;
    pal4::Rt2dMaestroState state{0u, 0x22u, 0x33u};
    pal4::Rt2dMaestroHandle::Destroy(state, runtime);
    Check(runtime.calls.empty() && state.auxiliary == 0x33u,
          "empty maestro does nothing");

    state.maestro = 0x4000u;
    pal4::Rt2dMaestroHandle::Destroy(state, runtime);
    Check(runtime.calls == std::vector<std::string>({
              "initialize dictionary", "set dictionary", "destroy maestro"}),
          "dictionary precedes maestro destruction");
    Check(runtime.dictionary == 0x5000u && runtime.destroyed == 0x4000u,
          "native handles forwarded");
    Check(state.maestro == 0u && state.auxiliary == 0u &&
              state.reserved == 0x22u, "only first and third words reset");
    pal4::Rt2dMaestroHandle::Destroy(state, runtime);
    Check(runtime.calls.size() == 3u, "repeat destroy is inert");
}

}  // namespace

int main() {
    try {
        TestPathAndCleanup();
        TestInitializationAndBridges();
        TestMaestroDestroy();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
