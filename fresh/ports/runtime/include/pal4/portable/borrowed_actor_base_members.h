#pragma once
#include "pal4/base_object_member_runtime.h"
#include "pal4/portable/object_memory.h"
#include <memory>
namespace pal4::portable {
// 原基础成员生命周期；模型/动作由外部拥有，关闭播放器并恢复动画图像后才销毁。
// 仅支持无UV字典的资源及空原动画记录；不是动态资源或动画TAB装载器。
// 资源绑定仍须通过检查原包的BindConstructedActorResource。存储必须存活更久。
class BorrowedActorBaseMembers final {
public:
    BorrowedActorBaseMembers(ObjectMemory&,BaseObjectMemberRuntime::AddressResolver);
    ~BorrowedActorBaseMembers();
    BorrowedActorBaseMembers(const BorrowedActorBaseMembers&)=delete;
    BorrowedActorBaseMembers& operator=(const BorrowedActorBaseMembers&)=delete;
    std::uint32_t Construct(BaseObjectState&,std::uint32_t logical_address);
    // 先验证未交还的动画/UV数据；失败不进入原析构，不改写图像或释放存储。
    void Destroy(BaseObjectState&);
    void ValidateDestroy(BaseObjectState&);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
