# HandmadeMath

上游：https://github.com/HandmadeMath/HandmadeMath

版本：v2.0.0，提交 `422bc588e9e8ae580f472f05e47c01a646acff38`。

`HandmadeMath.h`原样保存，SHA256：`489a6b0db28c2ef7441691147fc3c0664b1ab78f9314f62d9f76a0c58390ad01`。授权声明在头文件LICENSE段：公共领域，并提供不认可该声明地区的永久复制、分发及修改授权。

公共镜头模块只使用公开的向量归一化和矩阵转四元数接口；定义 `HANDMADE_MATH_NO_SSE`，同一份标量代码用于主机、Android和Switch。原游戏的方向查表归一化、四元数转欧拉角、插值和任务时钟继续复用已有重建代码。这里不重写RenderWare私有实现，也不链接原x86 SDK到ARM64。
