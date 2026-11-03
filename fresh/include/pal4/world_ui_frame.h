#pragma once
#include <cstdint>
namespace pal4 {
// Shared58FFA0 renderer used by Save9 and ModalFrame10.
class IWorldUiFrameRuntime {
public:
    virtual ~IWorldUiFrameRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint32_t ArenaManager()=0;
    virtual void ProcessWorldFrame(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t UIFrameManager()=0;
    virtual std::uint32_t RenderUI(std::uint32_t,std::uint32_t)=0;
};
}
