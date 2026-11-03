#pragma once

#include "pal4/rw_geometry_plugin.h"

namespace pal4 {

class NativeRwPlugin8449Lifecycle final : public IRwGeometryPluginLifecycle {
public:
    [[nodiscard]] static bool Attach();
    [[nodiscard]] static const RwGeometryPluginState& State();

private:
    static NativeRwPlugin8449Lifecycle& Instance();

    std::int32_t RegisterEnginePlugin(std::uint32_t bytes,
                                      std::uint32_t plugin_id) override;
    std::int32_t RegisterGeometryPlugin(std::uint32_t bytes,
                                        std::uint32_t plugin_id) override;
    std::int32_t RegisterGeometryStream(std::uint32_t plugin_id) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;

    static void* Identity(void* object, int, int);
    static void* AtomicConstruct(void* object, int, int);
    static void* AtomicDestruct(void* object, int, int);
    static void* AtomicCopy(void* destination, const void* source, int, int);

    RwGeometryPluginState state_;
};

}  // namespace pal4
