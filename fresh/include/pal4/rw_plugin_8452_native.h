#pragma once

#include "pal4/rw_plugin_8452.h"

namespace pal4 {

class NativeRwPlugin8452Lifecycle final : public IRwPlugin8452Lifecycle {
public:
    [[nodiscard]] static bool Attach();
    [[nodiscard]] static const RwPlugin8452State& State();

private:
    static NativeRwPlugin8452Lifecycle& Instance();

    std::int32_t RegisterEngine(std::uint32_t size, std::uint32_t id,
                                RwPlugin8452Callback construct,
                                RwPlugin8452Callback destruct) override;
    std::int32_t RegisterGeometry(std::uint32_t size, std::uint32_t id,
                                  RwPlugin8452Callback construct,
                                  RwPlugin8452Callback destruct,
                                  RwPlugin8452Callback copy) override;
    std::int32_t RegisterGeometryStream(std::uint32_t id,
                                        RwPlugin8452Callback read,
                                        RwPlugin8452Callback write,
                                        RwPlugin8452Callback size) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;

    static void* Identity(void* object, int, int);
    static void* AtomicConstruct(void* object, int, int);
    static void* AtomicDestruct(void* object, int, int);
    static void* AtomicCopy(void* destination, const void* source, int, int);

    RwPlugin8452State state_;
};

}  // namespace pal4
