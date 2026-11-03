#pragma once

#include <cstdint>

#include "pal4/singleton_collection_move.h"

namespace pal4 {

class ISingletonCollectionSortRuntime :
    public ISingletonCollectionMoveRuntime {
public:
    [[nodiscard]] virtual bool Compare(std::uint32_t left,
                                       std::uint32_t right) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateTemporary() = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseText(
        std::uint32_t record) = 0;
};

class SingletonCollectionSort final {
public:
    static void PartitionLarge(
        std::uint32_t begin, std::uint32_t end,
        ISingletonCollectionSortRuntime& runtime);
    static void SelectMedian(std::uint32_t first, std::uint32_t middle,
                             std::uint32_t last, std::uint32_t destination,
                             ISingletonCollectionSortRuntime& runtime);
    static void InsertSortedElement(
        std::uint32_t destination, std::uint32_t temporary,
        ISingletonCollectionSortRuntime& runtime);
    static void InsertionSort(
        std::uint32_t begin, std::uint32_t end,
        ISingletonCollectionSortRuntime& runtime);
    static void SwapCollectionElements(
        std::uint32_t left, std::uint32_t right,
        ISingletonCollectionSortRuntime& runtime);
};

}  // namespace pal4
