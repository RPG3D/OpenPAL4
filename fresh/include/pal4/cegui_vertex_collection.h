#pragma once

#include "pal4/cegui_renderer.h"

#include <array>
#include <cstdint>
#include <map>

namespace pal4 {

using CeguiVertexRecord = std::array<std::uint8_t, 28>;

class ICeguiVertexCollectionRuntime {
public:
    virtual ~ICeguiVertexCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void WriteRecord(std::uint32_t destination,
                             const CeguiVertexRecord& value) = 0;
};

class CeguiVertexCollectionStub : public ICeguiVertexCollectionRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override;
    void WriteRecord(std::uint32_t destination,
                     const CeguiVertexRecord& value) override;
    [[nodiscard]] const CeguiVertexRecord& Record(
        std::uint32_t address) const;

private:
    std::uint32_t next_address_ = 0x10000u;
    std::map<std::uint32_t, CeguiVertexRecord> records_;
};

class CeguiVertexCollection final {
public:
    // 0x415740, called with the 28-byte collection at renderer +200.
    [[nodiscard]] static std::uint32_t Insert(
        CeguiRendererState& state, std::uint32_t position,
        std::uint32_t count, const CeguiVertexRecord& value,
        ICeguiVertexCollectionRuntime& runtime);
    // 0x415a60: forward copy [source, old_end) to destination.
    [[nodiscard]] static std::uint32_t MoveRange(
        CeguiRendererState& state, std::uint32_t destination,
        std::uint32_t source, ICeguiVertexCollectionRuntime& runtime);
};

}  // namespace pal4
