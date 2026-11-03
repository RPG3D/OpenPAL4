# TinyXML 2.5.3

复用仓库既有源码研究目录 `refcode/source_library_research/tinyxml_2_5_3` 中的公开源码。下载地址、原压缩包 SHA256 及逐文件哈希见 `provenance.json`；源码保持原样，授权见各源码头及 `readme.txt`。

公共 UI 资源层通过 TinyXML 的公开 API 解析用户 `ui.cpk` 中的布局、字体描述及 imageset，使用 `TIXML_USE_STL` 和现代标准库。不链接原 x86 库，不重建 TinyXML 私有算法，不包含游戏资源。
