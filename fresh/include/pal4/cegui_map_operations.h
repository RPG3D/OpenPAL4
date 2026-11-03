#pragma once

#include <cstdint>

#include "pal4/cegui_system_initializer.h"
#include "pal4/cegui_lifecycle.h"

namespace pal4 {

class ICeguiMapRuntime {
public:
    virtual ~ICeguiMapRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Predecessor(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t InsertNode(
        std::uint32_t map, std::uint32_t child,
        std::uint32_t parent, CeguiKeyMapping mapping) = 0;
};

class ICeguiMapEraseRuntime {
public:
    virtual ~ICeguiMapEraseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Successor(
        std::uint32_t node) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
};

class ICeguiMapCleanupRuntime {
public:
    virtual ~ICeguiMapCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
};

class ICeguiMapInsertRuntime {
public:
    virtual ~ICeguiMapInsertRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
};

class ICeguiMapCursorRuntime {
public:
    virtual ~ICeguiMapCursorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
};

struct CeguiMapInsertionResult {
    std::uint32_t node = 0u;
    bool inserted = false;
};

class CeguiMapOperations final {
public:
    [[nodiscard]] static std::uint32_t ValueSlot(
        std::uint32_t map, std::uint32_t key,
        const CeguiTreeGlobals& globals, ICeguiMapRuntime& runtime);
    [[nodiscard]] static CeguiMapInsertionResult Insert(
        std::uint32_t map, CeguiKeyMapping mapping,
        const CeguiTreeGlobals& globals, ICeguiMapRuntime& runtime);
    [[nodiscard]] static std::uint32_t Erase(
        std::uint32_t map, std::uint32_t node,
        const CeguiTreeGlobals& globals, ICeguiMapEraseRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupSubtree(
        std::uint32_t root, const CeguiTreeGlobals& globals,
        ICeguiMapCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t map, std::uint32_t child,
        std::uint32_t parent, CeguiKeyMapping mapping,
        const CeguiTreeGlobals& globals,
        ICeguiMapInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t map, std::uint32_t key,
        const CeguiTreeGlobals& globals, ICeguiMapRuntime& runtime);
    [[nodiscard]] static std::uint32_t PreviousCursor(
        std::uint32_t& cursor, const CeguiTreeGlobals& globals,
        ICeguiMapCursorRuntime& runtime);
    [[nodiscard]] static std::uint32_t NextCursor(
        std::uint32_t& cursor, const CeguiTreeGlobals& globals,
        ICeguiMapCursorRuntime& runtime);
};

}  // namespace pal4
