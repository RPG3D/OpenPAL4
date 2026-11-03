#pragma once

#include "pal4/binary_xml_config.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pal4 {

class NativePackageArchive;

struct NativeEffectTextureAsset final {
    std::string member;
    // An empty payload preserves the original particle loader's null texture.
    std::vector<std::uint8_t> bytes;
};

// Owns one parsed .eft document from the original Effect.cpk.
class NativeEffectScriptAsset final {
public:
    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view name);
    [[nodiscard]] bool Load(NativePackageArchive& archive,
                            std::string_view name);
    void Clear() noexcept;
    [[nodiscard]] const BinaryXmlConfig& Document() const noexcept {
        return document_;
    }
    [[nodiscard]] const std::string& Name() const noexcept { return name_; }
    [[nodiscard]] const NativeEffectTextureAsset* FindTexture(
        std::string_view logical_path) const;
    [[nodiscard]] std::size_t TextureCount() const noexcept {
        return textures_.size();
    }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    BinaryXmlConfig document_;
    std::unordered_map<std::string, NativeEffectTextureAsset> textures_;
    std::string name_;
    std::string last_error_;
};

}  // namespace pal4
