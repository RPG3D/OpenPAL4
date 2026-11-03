# 原始资源进度

## Android 媒体临时缓冲

正式媒体通道从用户的 `Music/*.smp` 等原包读取成员，复制到 Java `byte[]` 后交由平台播放器。此前 JNI 使用 `std::vector::clear()` 清空 native 媒体字节，但 `clear()` 保留 vector 容量，长时间运行会继续占用曾经读入的最大一份媒体缓冲空间。

现在成功复制后通过交换空 vector 释放容量。源数据超过 JNI `jsize` 上限时释放 native 缓冲并抛出 `IllegalStateException`；`NewByteArray` 失败或 `SetByteArrayRegion` 留下的 pending exception 同样保留。Java 仅在媒体字节转移调用点捕获 `OutOfMemoryError` 和该路径的 `IllegalStateException`，用 native 保存的命令 generation/id 向媒体或音效状态机报告失败；其他异常继续传播。媒体命令用 `MediaServices::Complete(generation,id,...)` 完成匹配的等待令牌，音效则用 generation/handle 更新为 failed。媒体和音效命令在 GL 线程顺序取出并立即取字节；无字节命令不创建待转移令牌，共享状态类会拒绝重叠的非空转移。捕获后保留原异常文本用于失败原因。Android Session 与主机测试共用 [media_byte_transfer.h](include/pal4/portable/media_byte_transfer.h)。

验证结果：

- 主机 MSVC 构建 `pal4_portable_package_tests`、`pal4_portable_arena_tests` 和 `pal4_portable_actor_playback_tests` 通过。
- 主机 CTest：包、场景、动画合同 3 项全部通过。
- 新增 `pal4_media_byte_transfer_contract`，使用生产 transfer state 与真实媒体/音效服务验证成功消费、注入失败后的状态机完成、旧 generation 拒绝、空数据不建 ticket 和重叠拒绝。
- 本次 Android x86_64 无资产 APK 构建通过，Java 与 JNI native 代码均完成编译。
- 未安装设备或模拟器；未测量 native heap 峰值、垃圾回收时序或完整剧情中的媒体切换。

主机故障注入从共享状态服务入口模拟字节数组分配失败，并验证真实媒体/音效状态机响应；它没有执行 Android `NewByteArray`/`SetByteArrayRegion`，也不能证明 JVM pending exception 与 GL 回调边界。设备上尚未注入 JNI 数组失败。OOM 恢复回报本身仍依赖 `Throwable.toString()`、JNI 字符串传递和 native 错误记录可用的内存；极端内存耗尽时无法保证回报成功或待处理令牌完成。native 只有在取得错误文本后才清除待转移标记。相关总览见 [RAW_RESOURCES.md](RAW_RESOURCES.md)。

## 媒体与场景 CPK 磁盘写入审计

Android Bink 通过 `PackageMemberStream` 接入 FFmpeg AVIO；原样成员分块读取，压缩或加密成员在内存解码。Switch 视频使用 libmpv stream callback，音乐和音效由包读取器解到内存；场景 `ArenaSource` 正式原包模式只从用户 CPK 读取，并拒绝 `.p4*` 转换缓存请求。旧 Switch 0.2.5 媒体临时提取已不在播放路径；检出的 `native_bink_package_source.cpp` 使用 Windows API，且仅由 Win32 原生 Bink 目标编译，不属于 Android/Switch 正式运行时。

LZO CPK 成员解码会同时持有压缩输入和解压输出缓冲；`PackageReader::Read` 的现有限额定义为成员解码大小，并非总瞬时内存预算，压缩输入另受同值上限约束。两份缓冲因此可能使瞬时占用高于单个读取限额。当前正式路径没有独立峰值预算配置；改变现有限额语义会拒绝过去允许的原始成员，需先按资源实测与调用点确定独立策略。本次不修改读取行为。

生命周期静态审计：Android `BinkPlayer` 在 stop、解码异常或 EOF 后离开 try-with-resources 时关闭 JNI decoder；若 stop 发生在一次同步 FFmpeg `next()` 内，归档与解码器资源会等本次调用返回后释放，尚无外部中断钩子。Switch mpv callback 的 cancel 只设原子标记，文件、PackageReader 和 PackageMemberStream 在 mpv 调用 close callback 后随 Reader 一并析构；open 失败由局部 unique_ptr 回收。章节归档释放及延迟动画保留由 `archive-lifetime-original` 原包探针覆盖释放、重开和数据一致。包读取现有合同通过主机 CTest；未在设备验证取消关闭时序或测量实际峰值。
