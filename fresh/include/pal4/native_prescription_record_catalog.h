#pragma once
#include "pal4/game_data_parameter_table.h"
#include "pal4/game_data_field_descriptor.h"
#include <memory>
#include <span>
namespace pal4 {
class NativeGameDatabase;
struct NativeGameDbTable;
// Owned original328-byte Prescription records and six legacy texts per record.
// Uses the shared field registry and original signed-key table lookup.
class NativePrescriptionRecordCatalog final {
public:
    explicit NativePrescriptionRecordCatalog(const NativeGameDatabase&);
    // Same validated ownership boundary for an already decoded table snapshot.
    explicit NativePrescriptionRecordCatalog(const NativeGameDbTable&);
    ~NativePrescriptionRecordCatalog();
    NativePrescriptionRecordCatalog(const NativePrescriptionRecordCatalog&)=delete;
    NativePrescriptionRecordCatalog& operator=(const NativePrescriptionRecordCatalog&)=delete;
    std::span<const GameDataParameterRecord> Records()const noexcept;
    std::span<const GameDataFieldDescriptor> Fields()const noexcept;
    std::uint32_t Address(std::uint32_t key)const;
    std::uint32_t TableAddress()const noexcept;
    std::string_view Text(std::uint32_t address)const;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
