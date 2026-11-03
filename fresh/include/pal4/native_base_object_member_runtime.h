#pragma once
#include "pal4/base_object_member_runtime.h"
namespace pal4 {
// Win32入口保留实际地址；成员构造与64位平台共用同一适配器。
class NativeBaseObjectMemberRuntime final:public BaseObjectMemberRuntime {
public:
    NativeBaseObjectMemberRuntime(IGameConfigMemory&,IRwDictSchemaRuntime&,
        IGameObjectEmbeddedAnimationRuntime&,IStringBufferStorageRuntime&,ID3dFacePathRuntime&);
};
}
