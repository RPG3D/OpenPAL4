#pragma once

#include <cstdint>
#include <array>
#include <new>

#include "pal4/xml_node_data_reader.h"

namespace pal4::detail {

class XmlTemporary final {
public:
    XmlTemporary(IXmlNodeDataRuntime& runtime, const std::uint32_t size)
        : runtime_(runtime), address_(runtime.AllocateTemporary(size)) {
        if (address_ == 0u) throw std::bad_alloc{};
    }
    ~XmlTemporary() { runtime_.FreeTemporary(address_); }
    XmlTemporary(const XmlTemporary&) = delete;
    XmlTemporary& operator=(const XmlTemporary&) = delete;
    [[nodiscard]] std::uint32_t Address() const { return address_; }

private:
    IXmlNodeDataRuntime& runtime_;
    std::uint32_t address_;
};

class XmlAttribute final {
public:
    XmlAttribute(IXmlNodeDataRuntime& runtime, const std::uint32_t type,
                 const std::uint32_t name)
        : runtime_(runtime), address_(runtime.InitializeAttribute(type, name, storage_)) {
        if (address_ == 0u) throw std::bad_alloc{};
    }
    ~XmlAttribute() { runtime_.DestroyAttribute(address_); }
    XmlAttribute(const XmlAttribute&) = delete;
    XmlAttribute& operator=(const XmlAttribute&) = delete;
    [[nodiscard]] std::uint32_t Address() const { return address_; }

private:
    IXmlNodeDataRuntime& runtime_;
    std::array<std::uint32_t, 7> storage_{};
    std::uint32_t address_;
};

}  // namespace pal4::detail
