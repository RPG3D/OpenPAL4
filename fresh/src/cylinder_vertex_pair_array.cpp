#include "pal4/cylinder_vertex_pair_array.h"
#include "record_array_common.h"

namespace pal4 {
namespace {

class RawRecordRuntime final {
public:
    explicit RawRecordRuntime(ICylinderVertexPairArrayRuntime& runtime)
        : runtime_(runtime) {}

    std::uint32_t ReadWord(std::uint32_t address) {
        return runtime_.ReadWord(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) {
        runtime_.WriteWord(address, value);
    }
    std::uint32_t Allocate(std::uint32_t bytes) {
        return runtime_.Allocate(bytes);
    }
    void CopyRecord(std::uint32_t destination, std::uint32_t source) {
        runtime_.Copy40(destination, source);
    }
    void AssignRecord(std::uint32_t destination, std::uint32_t source) {
        runtime_.Copy40(destination, source);
    }
    void DestroyRecord(std::uint32_t) {}
    void Free(std::uint32_t address) { runtime_.Free(address); }

private:
    ICylinderVertexPairArrayRuntime& runtime_;
};

}  // namespace

void CylinderVertexPairArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    ICylinderVertexPairArrayRuntime& runtime) {
    RawRecordRuntime records(runtime);
    detail::InsertRecordCopies<kRecordBytes>(
        array, position, count, source, records);
}

}  // namespace pal4
