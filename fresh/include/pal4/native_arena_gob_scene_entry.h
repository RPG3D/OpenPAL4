#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace pal4 {

class NativeSceneMovementAsset;
class NativeSceneGobAssets;
class NativeGobObjectImage;
class NativeCameraManagerOwner;

// Executes the original animated-object enter callback against native scene owners.
class NativeArenaGobSceneEntry final {
public:
    static void Enter(std::uint32_t object, std::uint32_t arena,
                      NativeSceneMovementAsset& movement,
                      NativeCameraManagerOwner* cameras = nullptr,
                      NativeGobObjectImage* image = nullptr);
    static void Leave(NativeGobObjectImage& object, std::uint32_t arena,
                      NativeSceneMovementAsset& movement,
                      std::function<void()> refresh_object_sprites = {},
                      NativeCameraManagerOwner* cameras = nullptr);
    static void RemoveAnimated(NativeGobObjectImage& object,
                               std::uint32_t arena,
                               NativeSceneMovementAsset& movement,
                               std::function<void()> refresh_object_sprites = {},
                               NativeCameraManagerOwner* cameras = nullptr);
    static void AddAnimated(NativeGobObjectImage& object,
                            std::uint32_t arena,
                            NativeSceneMovementAsset& movement,
                            std::function<void()> refresh_object_sprites = {},
                            NativeCameraManagerOwner* cameras = nullptr);
    [[nodiscard]] static std::uint32_t EnterAll(
        NativeSceneGobAssets& assets, std::uint32_t arena,
        NativeSceneMovementAsset& movement,
        NativeCameraManagerOwner* cameras = nullptr);
    // Retires every owned object through the original leave callback, including
    // objects attached later by scripts rather than the initial entry session.
    [[nodiscard]] static std::uint32_t LeaveAll(
        NativeSceneGobAssets& assets, std::uint32_t arena,
        NativeSceneMovementAsset& movement,
        std::function<void()> refresh_object_sprites = {},
        NativeCameraManagerOwner* cameras = nullptr);
};

class NativeArenaGobSceneSession final {
public:
    NativeArenaGobSceneSession();
    ~NativeArenaGobSceneSession();
    NativeArenaGobSceneSession(const NativeArenaGobSceneSession&) = delete;
    NativeArenaGobSceneSession& operator=(const NativeArenaGobSceneSession&) = delete;

    [[nodiscard]] std::uint32_t BindAll(
        NativeSceneGobAssets& assets, std::uint32_t arena,
        NativeSceneMovementAsset& movement,
        std::function<void()> refresh_object_sprites,
        NativeCameraManagerOwner* cameras = nullptr);
    void Clear();
    [[nodiscard]] std::size_t Count() const noexcept;

private:
    struct EntryState;
    std::vector<std::unique_ptr<EntryState>> entries_;
};

}  // namespace pal4
