# AngelScript 2.38.0 来源

官方压缩包：https://www.angelcode.com/angelscript/sdk/files/angelscript_2.38.0.zip

压缩包 SHA256：`b33b5dbcda10317ef67d628353d83246984ce6fcac102d4dc2aed121eba52e6f`。

仅保留引擎源文件/头文件、scriptstdstring 插件和官方许可；引擎源码未修改。

scriptstdstring 有一处明确标注的本地修复：恢复 locale 的代码使用与创建 locale 相同的 Android/Vita 条件，修复上游条件范围不匹配导致的安卓编译错误，未改变数值解析算法。

构建使用 `AS_MAX_PORTABILITY` 和通用游戏回调。官方许可与源文件原始版权声明保留原文。
