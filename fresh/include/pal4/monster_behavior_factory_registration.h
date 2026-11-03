#pragma once
#include <cstdint>
namespace pal4 {
class IMonsterBehaviorFactoryRegistrationRuntime {
public:
    virtual ~IMonsterBehaviorFactoryRegistrationRuntime()=default;
    virtual void InsertBehavior(std::uint32_t registry,std::uint32_t class_id,std::uint32_t factory)=0;
};
// 复用原4312A0的四项注册顺序；注册容器由平台拥有。
void RegisterMonsterBehaviorFactories(std::uint32_t registry,IMonsterBehaviorFactoryRegistrationRuntime&);
}
