# Unicode CP936 映射

来源：[Unicode CP936.TXT](https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP936.TXT)，表版本 2.01，日期 2000-01-07。

原文件 SHA256：`b86f601c575e9ab457380b6f7abef03c75499cc6075bdc8b4b27f3f2de74bf6a`。

`cp936_table.inc` 由 `fresh/ports/tools/generate_gbk_table.py` 生成，包含 21791 个有效双字节映射；生成器检查源文件哈希、映射数量与合法范围。ASCII 和 0x80 欧元符号由解码器处理。码表用于公共 C++ 运行时，将原 CSB 字符串解码为 UTF-8，不依赖平台字符集 API。

授权见同目录 `LICENSE.txt`（Unicode License V3），来自 <https://www.unicode.org/license.txt>。码表是通用字符映射，不含游戏资产。

生成命令（仓库根目录）：

```powershell
python fresh/ports/tools/generate_gbk_table.py H:/PAL4/objs/pal4-android/unicode-cp936/CP936.TXT fresh/third_party/unicode-cp936/cp936_table.inc
```

全表参考对照：`H:/PAL4/objs/pal4-android/unicode-cp936/verification.json`。此对照与解码器单元测试不代表完整 CSB 指令或中文对白界面已实现。
