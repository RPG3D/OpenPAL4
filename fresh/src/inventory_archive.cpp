#include "pal4/inventory_archive.h"

#include <exception>

namespace pal4 {

std::uint8_t InventoryArchive::Serialize(
    const std::uint32_t object, const std::uint32_t stream,
    IInventoryArchiveRuntime& runtime) {
    const auto flag = runtime.CreateFlag(kFlag);
    std::uint32_t document = 0u;
    bool document_created = false;
    bool document_destroyed = false;
    bool flag_released = false;
    try {
        runtime.WriteFlag(stream, flag);
        document = runtime.CreateDocument(kDocument);
        document_created = true;
        const auto attribute = runtime.FindAttribute(document, kMoney);
        if (attribute != 0u)
            runtime.SetAttributeFromField(attribute, object + 0x134u);
        else
            runtime.AddAttributeFromField(document, kMoney,
                                          object + 0x134u);
        runtime.WriteDocument(document, stream);
        (void)InventorySlotArray::SerializeAll(object + 0x104u, stream,
                                               runtime.ArrayRuntime());
        runtime.SerializeSlots(object + 0x7cu, stream);
        const auto result = runtime.SerializeBase(object, stream);
        document_destroyed = true;
        runtime.DestroyDocument(document);
        flag_released = true;
        runtime.ReleaseFlag(flag);
        return result;
    } catch (...) {
        const auto error = std::current_exception();
        if (document_created && !document_destroyed) {
            document_destroyed = true;
            try {
                runtime.DestroyDocument(document);
            } catch (...) {
            }
        }
        if (!flag_released) {
            try {
                runtime.ReleaseFlag(flag);
            } catch (...) {
            }
        }
        std::rethrow_exception(error);
    }
}

std::uint8_t InventoryArchive::Deserialize(
    const std::uint32_t object, const std::uint32_t stream,
    IInventoryArchiveRuntime& runtime) {
    const auto flag = runtime.CreateFlag("");
    std::uint32_t document = 0u;
    bool document_created = false;
    bool document_destroyed = false;
    bool flag_released = false;
    try {
        if (!runtime.ReadFlag(stream, flag, kFlag)) {
            runtime.MarkFlagError(flag);
            flag_released = true;
            runtime.ReleaseFlag(flag);
            return 0u;
        }
        (void)InventorySubsystem::Reset(object, runtime);
        document = runtime.CreateDocument(kDocument);
        document_created = true;
        if (!runtime.ReadDocument(document, stream)) {
            document_destroyed = true;
            runtime.DestroyDocument(document);
            flag_released = true;
            runtime.ReleaseFlag(flag);
            return 0u;
        }
        const auto attribute = runtime.FindAttribute(document, kMoney);
        runtime.WriteWord(object + 0x134u,
                          attribute == 0u ? 0u :
                          runtime.ReadAttributeInt(attribute));
        (void)InventorySlotArray::DeserializeAll(object + 0x104u, stream,
                                                 runtime.ArrayRuntime());
        runtime.DeserializeSlots(object + 0x7cu, stream);
        const auto result = runtime.DeserializeBase(object, stream);
        document_destroyed = true;
        runtime.DestroyDocument(document);
        flag_released = true;
        runtime.ReleaseFlag(flag);
        return result;
    } catch (...) {
        const auto error = std::current_exception();
        if (document_created && !document_destroyed) {
            document_destroyed = true;
            try {
                runtime.DestroyDocument(document);
            } catch (...) {
            }
        }
        if (!flag_released) {
            try {
                runtime.ReleaseFlag(flag);
            } catch (...) {
            }
        }
        std::rethrow_exception(error);
    }
}

}  // namespace pal4
