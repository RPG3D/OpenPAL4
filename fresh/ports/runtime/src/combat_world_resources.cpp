#include "pal4/portable/combat_world_resources.h"

#include "pal4/portable/bsp_scene.h"
#include "pal4/portable/image_texture.h"
#include "pal4/portable/package.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace pal4::portable {
namespace {
constexpr std::size_t kMaxCombatWorldPackageBytes = 256u * 1024u * 1024u;

bool SafeArea(std::string_view area) {
    return !area.empty() && area.size() <= 64u &&
        area.find_first_not_of(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") ==
            std::string_view::npos;
}

std::optional<std::vector<std::uint8_t>> ReadTextureBytes(
    PackageReader& package, std::string_view area, std::string_view name) {
    if (name.empty() || name.size() > 255u ||
        name.find_first_of("\\/:") != std::string_view::npos ||
        name.find("..") != std::string_view::npos)
        throw std::runtime_error("invalid CombatWorld texture name");

    const std::string prefix = std::string(area) + "\\" + std::string(name);
    for (const auto suffix : {".dds", ".png", ""}) {
        if (auto bytes = package.ReadIfPresent(prefix + suffix, 32u * 1024u * 1024u))
            return bytes;
    }
    return {};
}

Texture DecodeTexture(std::span<const std::uint8_t> bytes) {
    const bool dds = bytes.size() >= 4u && bytes[0] == 'D' &&
        bytes[1] == 'D' && bytes[2] == 'S' && bytes[3] == ' ';
    return DecodeOriginalTexture(bytes, dds);
}
} // namespace

struct CombatWorldResources::State final {
    std::string area;
    std::filesystem::path package_path;
    std::array<std::uint8_t, 16> lightmap_config{};
    Scene world;
};

struct CombatWorldResources::Control final {
    std::unique_ptr<State> state;
    std::size_t leases{};
};

CombatWorldResources::Lease::Lease(std::shared_ptr<Control> control)
    : control_(std::move(control)) {
    if (!control_ || !control_->state)
        throw std::logic_error("CombatWorld lease requires loaded resources");
    ++control_->leases;
}

CombatWorldResources::Lease::~Lease() { Release(); }

CombatWorldResources::Lease::Lease(Lease&& other) noexcept
    : control_(std::move(other.control_)) {}

CombatWorldResources::Lease& CombatWorldResources::Lease::operator=(
    Lease&& other) noexcept {
    if (this != &other) {
        Release();
        control_ = std::move(other.control_);
    }
    return *this;
}

CombatWorldResources::Lease::operator bool() const noexcept {
    return control_ && control_->state;
}

const Scene& CombatWorldResources::Lease::World() const {
    if (!*this) throw std::logic_error("CombatWorld lease is empty");
    return control_->state->world;
}

std::string_view CombatWorldResources::Lease::Area() const {
    if (!*this) throw std::logic_error("CombatWorld lease is empty");
    return control_->state->area;
}

const std::array<std::uint8_t, 16>&
CombatWorldResources::Lease::LightmapConfig() const {
    if (!*this) throw std::logic_error("CombatWorld lease is empty");
    return control_->state->lightmap_config;
}

void CombatWorldResources::Lease::Release() noexcept {
    if (!control_) return;
    if (control_->leases) --control_->leases;
    control_.reset();
}

CombatWorldResources::CombatWorldResources(std::filesystem::path game_data)
    : game_data_(std::move(game_data)), control_(std::make_shared<Control>()) {
    if (game_data_.empty())
        throw std::invalid_argument("CombatWorld resources require a game-data path");
}

CombatWorldResources::~CombatWorldResources() = default;

void CombatWorldResources::Load(std::string_view area) {
    if (!SafeArea(area)) throw std::invalid_argument("invalid CombatWorld area");
    if (control_->state) {
        if (control_->state->area == area) return;
        throw std::logic_error("CombatWorld must be reset before area replacement");
    }

    auto next = std::make_unique<State>();
    next->area = area;
    next->package_path = game_data_ / "PALWorld" / "CombatWorld" /
        (std::string(area) + ".cpk");
    std::ifstream input(next->package_path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open original CombatWorld package: " +
                                 next->package_path.string());

    PackageReader package(input);
    const auto prefix = std::string(area) + "\\";
    const auto bsp = package.Read(prefix + std::string(area) + ".bsp",
                                  kMaxCombatWorldPackageBytes);
    const auto config = package.Read(prefix + std::string(area) + "_ltMap.cfg",
                                    64u * 1024u);
    if (config.size() < next->lightmap_config.size())
        throw std::runtime_error("original CombatWorld lightmap config is truncated");
    std::copy_n(config.begin(), next->lightmap_config.size(),
                next->lightmap_config.begin());

    const auto load_required = [&](std::string_view name) {
        auto bytes = ReadTextureBytes(package, area, name);
        if (!bytes)
            throw std::runtime_error("original CombatWorld texture is missing: " +
                                     std::string(name));
        return DecodeTexture(*bytes);
    };
    const auto load_lightmap = [&](std::string_view name) -> std::optional<Texture> {
        auto bytes = ReadTextureBytes(package, area, name);
        if (!bytes) return {};
        return DecodeTexture(*bytes);
    };
    next->world = ReadBspScene(bsp, std::span(next->lightmap_config),
                               load_required, load_lightmap);
    control_->state = std::move(next);
}

void CombatWorldResources::Reset() {
    if (control_->leases)
        throw std::logic_error("CombatWorld reset while scene leases are active");
    control_->state.reset();
}

bool CombatWorldResources::Loaded() const noexcept {
    return control_ && bool(control_->state);
}

CombatWorldResources::Lease CombatWorldResources::Borrow() {
    return Lease(control_);
}

} // namespace pal4::portable
