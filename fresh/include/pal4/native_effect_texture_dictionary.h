#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace pal4 {
struct RwTextureDictionaryState;
// Owns the effect-system dictionary. All material/particle users must be
// destroyed before this owner and before RenderWare shutdown.
class NativeEffectTextureDictionary final {
public:
    explicit NativeEffectTextureDictionary(const std::filesystem::path& game_root);
    // Constructs in zeroed caller storage (the singleton's +40). The storage
    // must outlive this owner. Initialize at the effect-system enable stage.
    NativeEffectTextureDictionary(const std::filesystem::path& game_root,
                                  RwTextureDictionaryState& storage);
    ~NativeEffectTextureDictionary();
    NativeEffectTextureDictionary(const NativeEffectTextureDictionary&)=delete;
    NativeEffectTextureDictionary& operator=(const NativeEffectTextureDictionary&)=delete;
    // Reinitialization/cleanup require all material and particle users gone.
    void Initialize();
    void Cleanup();
    [[nodiscard]] std::uint32_t Find(std::string_view path);
    [[nodiscard]] std::uint32_t Load(std::string_view path);
    void Release(std::uint32_t texture);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
