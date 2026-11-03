#pragma once
#include "pal4/string_buffer.h"
#include "pal4/game_config_state.h"
#include "pal4/d3d_face_path.h"
#include <cstddef>
#include <memory>
namespace pal4::portable {
struct ObjectMemoryLimits {
    std::uint32_t first=0x10000000,limit=0x70000000;
    std::size_t bytes=64u*1024u*1024u,allocations=100000;
};
// 单线程对象存储：逻辑身份不复用，数据由标准容器拥有；不是原堆算法重写。
// 外部对象身份必须与分配区间分离。借用Path服务的对象必须先于本存储销毁。
class ObjectMemory final:public IStringBufferStorageRuntime,public IGameConfigMemory {
public:
    explicit ObjectMemory(ObjectMemoryLimits limits={});
    ~ObjectMemory();
    ObjectMemory(const ObjectMemory&)=delete;
    ObjectMemory& operator=(const ObjectMemory&)=delete;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t Free(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address,std::uint32_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) const;
    void WriteByte(std::uint32_t address,std::uint8_t value) override;
    std::size_t AllocationCount() const;
    std::size_t AllocatedBytes() const;
    ID3dFacePathRuntime& Path();
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
