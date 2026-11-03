# 怪物资源逐帧更新

ActorResourceFrame通过原PackageReader检查实际模型与伴随资源，再调用原GameResourceManager::UpdateAndRenderClump（667620）和RwDictSchema::Update（6659D0）。它用于替换已确认无动态资源怪物的空资源回调，不是完整资源装载器。

## 原包证明与限制

按原装载路径检查model\model.dff、model\model.uva、model\model_shadow.dff。原DFF必须存在；UVA或独立阴影存在时明确拒绝，等待对应资源拥有者接入。不能根据P4AR缓存没有元数据就推断原资源不存在。

当前原包中的401、402、405、407、408均无这两种伴随资源。原LoadDict失败后dict/list为0，原资源更新自然不调用UV服务；没有阴影模型则不提交阴影场景。这是原资源对应的真实分支，不是通用空成功后备。角色101存在阴影，其拒绝路径另经原包测试；玩家阴影和其他动态资源仍未完成。

组件借用实例及实际管理器状态，要求主体已装载。每次检查schema/阴影字段，拒绝未绑定的动态状态；不能用它代替管理器初始化。按原loaded、视锥模式/可见位门控返回更新或跳过，保留返回值和调用统计。异常后禁止继续，错误图像和非法时间输入不会被当成成功。

## 共用接口

原667620实现从render文件移到独立update文件，分支未改。接口缩到IGameResourceFrameRuntime与IRwDictSchemaUpdateRuntime；现有完整资源runtime继承小接口，仍能传入真实UV/阴影服务。主工程和native/package源列表同步；本轮未重新验证完整Win32目标。

## 验证

主机4项回归与安卓独立合同通过。共用原核心覆盖loaded/视锥门、UV按时间倍率推进后应用、动态列表尾重新读取、UV先于阴影提交，以及空列表/不完整阴影门控。UV和阴影分支使用显式受控回调，不等于移动端GPU实现已完成。

原M01/1八只730帧、M02/1十二只1066帧追逐循环，在新探针中把资源空回调替换为本组件。两端完整报告逐字节一致，分别754和1102次资源调用，合计包含40次门控跳过。还检查错误图像、非法时间、动态状态注入后失败、缺失原模型和真实阴影资源拒绝。

旧资源帧探针的原管理器初始图像、单怪物世界、玩家/随机/内存、世界碰撞与交互/组动作/战斗服务仍受控。未验证真实管理器初始化、完整动态资源、GPU、CSB或第一章；未替换已安装APK/NRO。旧探针保留原范围，不据此改称已使用真实资源服务。

证据：H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/actor-resource-frame-probe。

主资源前92个word已在专用探针改用原构造和初始化，见[主资源初始化](ACTOR_RESOURCE_INITIALIZATION.md)。这不扩大本文旧探针的验证范围，完整管理器仍待接。
