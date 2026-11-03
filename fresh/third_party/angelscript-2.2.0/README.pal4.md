# AngelScript 2.2.0 来源与边界

本目录保存官方 2.2.0 SDK 引擎及 scriptstring 插件：
https://www.angelcode.com/angelscript/sdk/files/angelscript_2.2.0.zip

原始许可与版权声明保留原文。本地兼容修改：

- `source/as_config.h` 的旧 `vsnprintf` 别名仅用于 MSVC1900 之前版本，避免与现代 UCRT 冲突。
- `scriptstring/scriptstring.cpp` 使用显式比较包装函数，避免现代 MSVC 无法解析历史运算符地址。

原 PAL4 可执行文件的回调记录为32字节，本 SDK 在 VS2022 x86 下的 `asUPtr` 为24字节。通过本库自身头文件调用有效，但未经 ABI 适配验证，不能把本库对象交给原可执行文件内部函数。

当前安卓移植只把此版本用于 Windows32位执行校验；64位运行时使用官方2.38版本。
