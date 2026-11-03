# LodePNG来源

- 上游：https://github.com/lvandeve/lodepng
- 固定提交：`e584a8490db1b76b81bf298c11c09d3b55d9c270`，版本20261006。
- 原样保留 `lodepng.h`、`lodepng.cpp`，许可证位于两文件开头，未修改上游实现。
- SHA256：头文件 `9A1FCA0238C38333BDC8545336961B73FD0B76957BBACF033186991BE9D7CF85`；实现 `6AEC876971EC860527859458D3095220FC5EE1258EC37EB59E1E7DD9CF136A81`。
- 公共运行时仅启用内存PNG解码，关闭文件IO与编码；不依赖Android或RenderWare私有PNG实现。
- 游戏图像从用户原CPK读取，不随源码或发布包附带。
