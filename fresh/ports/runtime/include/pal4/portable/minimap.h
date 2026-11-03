#pragma once
#include "pal4/portable/scene.h"
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
namespace pal4::portable {
using MinimapMemberReader=std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)>;
struct MinimapObject {
    std::string name;
    std::array<float,3> position{};
    std::uint32_t marker{},visible{},uses{};
};
struct MinimapIcon {
    std::string name,image;
    std::int32_t x{},y{};
    std::uint32_t category{};
};
struct MinimapPlayer {
    bool valid{};
    std::int32_t grid_x{},grid_y{},left{},top{};
    std::array<std::uint32_t,3> direction_bits{};
};
// 直接读取用户scenedata.cpk中的MiniMap.df与PNG；拥有CPU图层与图标。
// GPU/HUD只消费这些状态，不介入脚本或原图标筛选规则。
class SceneMinimap {
public:
    SceneMinimap(std::string world,std::string scene,MinimapMemberReader read,MinimapMemberReader icons={});
    ~SceneMinimap();
    SceneMinimap(const SceneMinimap&)=delete;
    SceneMinimap& operator=(const SceneMinimap&)=delete;
    void RefreshObjects(std::span<const MinimapObject> objects);
    // 单独探索用于工具；游戏250/66通过UpdatePlayer执行原坐标/朝向/探索链。
    bool RevealAt(std::array<float,3> position);
    void UpdatePlayer(std::array<float,3> position,std::array<float,3> rotation);
    // CPU viewport uses the recovered MinimapTextureState crop/clear/refresh flow.
    void ConfigureViewport(unsigned width,unsigned height);
    void SetSceneType(std::uint32_t type);
    std::uint32_t DisplayMode() const;
    bool ViewportHasAlpha() const;
    const Texture* ViewportImage() const;
    std::array<std::int32_t,4> ViewportRegion() const;
    std::uint64_t ViewportRevision() const;
    const MinimapPlayer& Player() const;
    const Texture* PlayerImage() const;
    void SelectLayer(std::size_t index);
    const Texture* ActiveImage() const;
    std::span<const MinimapIcon> Icons() const;
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
