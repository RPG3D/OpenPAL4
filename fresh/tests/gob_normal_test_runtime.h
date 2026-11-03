#pragma once

#include "pal4/gob_normal.h"
#include "pal4/gob_snapshot_runtime.h"

#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct NormalSnapshotRuntime final : pal4::IGobSnapshotRuntime {
    static constexpr std::uint32_t snapshot = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = snapshot;
    bool fail_name_init = false;
    bool xml_valid = true;
    std::string name;
    std::string xml_name;
    std::int32_t xml_visible = 0;

    std::uint32_t Allocate(std::uint32_t size) override {
        if (size != 0x18u) throw std::runtime_error("snapshot size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        if (address != snapshot) throw std::runtime_error("snapshot free");
        calls.emplace_back("free");
    }
    void InitializeNameRecord(std::uint32_t record) override {
        if (record != snapshot + 4u) throw std::runtime_error("name offset");
        calls.emplace_back("name init");
        if (fail_name_init) throw std::runtime_error("name init failed");
        name.clear();
    }
    std::uint32_t ReleaseNameRecord(std::uint32_t record) override {
        if (record != snapshot + 4u) throw std::runtime_error("name offset");
        calls.emplace_back("name release");
        name.clear();
        return 0x55u;
    }
    std::string ReadNameRecord(std::uint32_t record) override {
        if (record != snapshot + 4u) throw std::runtime_error("name offset");
        calls.emplace_back("name read");
        return name;
    }
    void AssignNameRecord(std::uint32_t record,
                          std::string_view value) override {
        if (record != snapshot + 4u) throw std::runtime_error("name offset");
        calls.emplace_back("name assign");
        name = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read word");
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write word");
        words[address] = value;
    }
    std::uint32_t CreateXmlDocument(std::string_view root) override {
        if (root != "soGOBNormal") throw std::runtime_error("XML root");
        calls.emplace_back("create XML");
        return 0x8000u;
    }
    void WriteXmlString(std::uint32_t document, std::string_view key,
                        std::string_view value) override {
        if (document != 0x8000u || key != "valName")
            throw std::runtime_error("XML string key");
        calls.emplace_back("write XML name");
        xml_name = value;
    }
    void WriteXmlInt(std::uint32_t document, std::string_view key,
                     std::int32_t value) override {
        if (document != 0x8000u || key != "valVisible")
            throw std::runtime_error("XML integer key");
        calls.emplace_back("write XML visible");
        xml_visible = value;
    }
    void FlushXml(std::uint32_t document, std::uint32_t stream) override {
        if (document != 0x8000u || stream != 0x9000u)
            throw std::runtime_error("XML flush");
        calls.emplace_back("flush XML");
    }
    bool ReadXml(std::uint32_t document, std::uint32_t stream) override {
        if (document != 0x8000u || stream != 0x9000u)
            throw std::runtime_error("XML read");
        calls.emplace_back("read XML");
        return xml_valid;
    }
    std::string ReadXmlString(std::uint32_t document, std::string_view key,
                              std::string_view fallback) override {
        if (document != 0x8000u || key != "valName")
            throw std::runtime_error("XML read string key");
        calls.emplace_back("read XML name");
        return xml_name.empty() ? std::string(fallback) : xml_name;
    }
    std::int32_t ReadXmlInt(std::uint32_t document, std::string_view key,
                            std::int32_t) override {
        if (document != 0x8000u || key != "valVisible")
            throw std::runtime_error("XML read integer key");
        calls.emplace_back("read XML visible");
        return xml_visible;
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t document) override {
        if (document != 0x8000u) throw std::runtime_error("XML destroy");
        calls.emplace_back("destroy XML");
        return 0x88u;
    }
};
