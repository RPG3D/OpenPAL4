#pragma once
#include "pal4/portable/monster_behavior_definition.h"
#include <functional>
#include <memory>
#include <span>
#include <optional>

namespace pal4::portable {
class MonsterObjectMembers;
// 具体移动/网格辅助对象必须实现完整生命周期，队列没有空操作后备。
// 辅助对象析构须释放其剩余资源；Release仅按原任务路径调用。
class IMonsterBehaviorAction {
public:
    virtual ~IMonsterBehaviorAction()=default;
    // 失败时必须回滚外部注册，保持未接纳状态。
    virtual void Enter()=0;
    virtual bool Active() const=0;
    virtual void SetActive(bool)=0;
    virtual void Update(float seconds)=0;
    virtual bool Complete() const=0;
    virtual void Release() noexcept=0;
};
class IMonsterBehaviorActionFactory {
public:
    virtual ~IMonsterBehaviorActionFactory()=default;
    virtual bool Supports(std::int32_t kind) const=0;
    virtual std::unique_ptr<IMonsterBehaviorAction> Create(std::int32_t kind)=0;
};
class MonsterBehaviorTasks {
public:
    // 单游戏线程使用，不可重入。借用factory；必须晚于队列销毁。
    // 随机值由原游戏RNG边界提供。
    MonsterBehaviorTasks(IMonsterBehaviorActionFactory&,std::function<std::uint32_t()> random);
    ~MonsterBehaviorTasks();
    MonsterBehaviorTasks(const MonsterBehaviorTasks&)=delete;
    MonsterBehaviorTasks& operator=(const MonsterBehaviorTasks&)=delete;
    // 绑定实际管理器+688的4-word视图；只改写构造器写入的有效字节。
    // 视图须存活到解绑，节点身份仍由此队列解析。
    std::uint64_t BindContainerImage(std::span<std::uint32_t,4>,std::uint32_t logical_address);
    void UnbindContainerImage(std::uint64_t binding);
    bool OwnsContainerImage(std::uint64_t binding) const;
    std::size_t RegisteredFactoryCount() const;
    std::optional<std::uint32_t> FactoryAddress(std::int32_t class_id) const;
    bool Start(const MonsterBehaviorDefinition&);
    std::uint32_t Advance(float seconds);
    void Cancel();
    bool Ready() const;
    std::size_t TaskCount() const;
    std::size_t HelperCount() const;
    void WriteReport(std::ostream&) const;
private:
    friend class MonsterObjectMembers;
    MonsterBehaviorTasks(IMonsterBehaviorActionFactory&,std::function<std::uint32_t()>,bool register_factories);
    void RegisterFactory(std::uint32_t class_id,std::uint32_t factory);
    void ClearFactories();
    struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace pal4::portable
