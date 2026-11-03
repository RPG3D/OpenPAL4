# miniLZO 2.09

用于读取原始 CPK/SMP 的 LZO1X 压缩成员，不包含游戏资源。

- 作者：Markus F. X. J. Oberhumer。
- 上游：https://www.oberhumer.com/opensource/lzo/
- 来源：本机既有 `H:/PAL4/pal4-pack-tool/minilzo`，四份源码原样复制。
- 版本：`MINILZO_VERSION = 0x2090`。
- 许可：GPL-2.0-or-later；保留各文件版权头，完整许可见 `COPYING`。
- 运行时调用 `lzo1x_decompress_safe`，校验错误码和实际解压长度。

SHA256：

| 文件 | SHA256 |
| --- | --- |
| minilzo.c | 9930c1ed636b41304f48087b75437d0fb8853258f62f3ff0b83378eeabead0f3 |
| minilzo.h | 4d74063fd42acbad95ab9a4cbca67ca91e6293b4bc3a03972a1ea5cf381de98c |
| lzoconf.h | db90ae188e46ee7cb82e22683e942859b011174be9c305817f6de77ba0f55aca |
| lzodefs.h | 113ed5cd3272ee07949ce8b9a10e94d3f898796b42b91b131137ab7b86e33873 |
