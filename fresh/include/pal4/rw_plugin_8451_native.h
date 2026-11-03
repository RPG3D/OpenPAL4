#pragma once

#include "pal4/rw_plugin_8451.h"

namespace pal4 {

class NativeRwPlugin8451Lifecycle final : public IRwPlugin8451Lifecycle {
public:
    [[nodiscard]] static bool Attach();
    [[nodiscard]] static const RwPlugin8451State& State();

private:
    static NativeRwPlugin8451Lifecycle& Instance();

    std::int32_t RegisterEngine(std::uint32_t size, std::uint32_t id,
                                RwPlugin8451Callback construct,
                                RwPlugin8451Callback destruct) override;
    std::int32_t RegisterGeometry(std::uint32_t size, std::uint32_t id,
                                  RwPlugin8451Callback construct,
                                  RwPlugin8451Callback destruct,
                                  RwPlugin8451Callback copy) override;
    std::int32_t RegisterGeometryStream(std::uint32_t id,
                                        RwPlugin8451Callback read,
                                        RwPlugin8451Callback write,
                                        RwPlugin8451Callback size) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t ReadWord(std::uint32_t address) override;

    static void* Identity(void* object, int, int);
    static void* AtomicConstruct(void* object, int, int);
    static void* AtomicDestruct(void* object, int, int);
    static void* AtomicCopy(void* destination, const void* source, int, int);

    RwPlugin8451State state_;
};

}  // namespace pal4
