# bcdec 来源

- 上游：https://github.com/iOrange/bcdec
- 固定提交：80859ed3b7afb1c527a2a99d70c61457bea72d0c
- 版本：头文件标记v0.985。
- 授权：上游双授权，项目选择MIT；完整LICENSE原样保留。
- bcdec.h SHA256：134520764d96f70a27db814173616c89ad85db95fbd3328417c7a8c77e7ca189。
- LICENSE SHA256：2414c782587f6e96661cb0d19d752432ddecfcc1a221b4cb12bc2253a54dc5ac。

上游文件未修改。bcdec.c仅定义BCDEC_IMPLEMENTATION并包含头文件。公共DDS容器读取位于ports/runtime，BC1/BC2/BC3块使用上游解码；不恢复RenderWare或D3DX私有解码器。只解码顶层图像，原mip链不上传。真实914_1.dds与Pillow比较：alpha一致，2139个RGB分量相差1；保留差异，不声称逐字节等价或原SDK数值等价。
