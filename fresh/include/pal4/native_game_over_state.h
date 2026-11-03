#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
namespace pal4 {
class NativePkFileManagerRuntime;struct PalScreenEffectState;class NativeEffectRenderRuntime;
class INativeGameOverStateDispatch {
public:
    virtual ~INativeGameOverStateDispatch()=default;
    virtual std::uint32_t Address()const=0;
    virtual bool Enter(std::uint32_t previous,std::uint32_t document)=0;
    virtual void Leave()=0;
    virtual void Render(std::uint32_t frame)=0;
    virtual void Update(float elapsed)=0;
};
// Owns original20-byte state7 and its cached SDK texture. Borrowed package,
// screen/config/render/transition services outlive Close and state registration.
class NativeGameOverState final:public INativeGameOverStateDispatch {
public:
    NativeGameOverState(std::uint32_t game,std::span<const std::uint32_t,2> config,
        NativePkFileManagerRuntime&,PalScreenEffectState&,NativeEffectRenderRuntime&,std::function<void()> logo);
    ~NativeGameOverState();
    std::uint32_t Address()const override;
    bool Enter(std::uint32_t previous,std::uint32_t document)override;
    void Leave()override;
    void Render(std::uint32_t frame)override;
    void Update(float elapsed)override;
    void Close();
    bool Entered()const noexcept;
    float Remaining()const noexcept;
    std::uint32_t Texture()const noexcept;
    std::uint32_t DrawCount()const noexcept;
    std::int32_t LastAlpha()const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
