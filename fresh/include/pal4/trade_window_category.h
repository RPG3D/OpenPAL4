#pragma once
#include <cstdint>
namespace pal4 {
class ITradeWindowCategoryRuntime {
public:
    virtual ~ITradeWindowCategoryRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t AdjacentIndex(std::uint32_t group,std::uint32_t current,bool next)=0;
    virtual void UpdatePage(std::uint32_t frame,std::uint32_t group,std::uint32_t page,std::uint32_t forward)=0;
};
}
