#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

// Whole system renderer boundary used by the native state dispatcher. Its
// implementation owns all576220 phases; it does not supply Entry/update.
class ICombatSystemFrameRenderer {
public:
    virtual ~ICombatSystemFrameRenderer()=default;
    virtual void Render(std::uint32_t frame)=0;
};
// Debug presentation is shared independently of the unrelated5761B0 sound callback.
class ICombatSystemDebugRuntime {
public:
    virtual ~ICombatSystemDebugRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint8_t ReadByte(std::uint32_t)=0;
    virtual std::uint32_t GetGameState()=0;
    virtual std::uint32_t CountEffects(std::uint32_t)=0;
    virtual std::string GetBgmName()=0;
    virtual std::string ReadCString(std::uint32_t)=0;
    virtual void DrawText(std::string_view,std::uint32_t left,std::uint32_t top,
        std::uint32_t right,std::uint32_t bottom,std::uint32_t font_index,
        std::uint32_t color,std::uint32_t draw_flags)=0;
};
class ICombatSystemFrameServices {
public:
    virtual ~ICombatSystemFrameServices()=default;
    virtual bool Inactive()=0;
    virtual void World(std::uint32_t frame)=0;
    virtual void GameObject(std::uint32_t frame)=0;
    virtual void Actors(std::uint32_t frame)=0;
    virtual void Ui(std::uint32_t frame)=0;
    virtual void Console(std::uint32_t frame)=0;
    virtual void Debug(std::uint32_t frame)=0;
};
class ICombatSystemRenderRuntime : public ICombatSystemDebugRuntime {
public:
    virtual void PlaySound(std::uint32_t,std::uint32_t,std::uint32_t)=0;
    virtual void RenderWorld(std::uint32_t)=0;
    virtual void RenderGameObject(std::uint32_t,std::uint32_t)=0;
    virtual void RenderActors(std::uint32_t)=0;
    virtual void RenderUi(std::uint32_t)=0;
    virtual void RenderConsole(std::uint32_t)=0;
};

class CombatSystemRender final {
public:
    // IDA 0x5761b0.
    [[nodiscard]] static std::uint32_t PlaySoundEffect(
        std::uint32_t sound, std::uint32_t second,
        std::uint32_t third, ICombatSystemRenderRuntime& runtime);
    // IDA 0x576220, reconstructed from assembly because Hex-Rays failed.
    static void Render(std::uint32_t object, std::uint32_t frame,
                       ICombatSystemRenderRuntime& runtime);
    static void RenderFrame(std::uint32_t state,std::uint32_t frame,ICombatSystemFrameServices&);
    // IDA 0x5762c0.
    static void DrawDebugInfo(std::uint32_t object,
                              std::uint32_t frame,
                              ICombatSystemDebugRuntime& runtime);
};

}  // namespace pal4
