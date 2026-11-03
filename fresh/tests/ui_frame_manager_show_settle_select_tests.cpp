#include "pal4/ui_frame_manager_show_settle_select.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShowSettleSelectRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x4000u;
    bool throw_insert = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "handler allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    void InsertOrReplace(std::uint32_t tree,
                         std::uint32_t key,
                         std::uint32_t handler) override {
        Check(tree == 0x6104u &&
                  key == pal4::UiFrameManagerShowSettleSelect::kKey &&
                  handler == allocation,
              "insert receives manager tree, show-settle key and handler");
        calls.emplace_back("insert");
        if (throw_insert) throw std::runtime_error("insert");
    }
    void ReleaseHandler(std::uint32_t handler,
                        std::uint32_t flags) override {
        Check(handler == allocation && flags == 1u,
              "temporary callback owner is released with deleting flag");
        calls.emplace_back("release");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::UiFrameManagerShowSettleSelect::GetHandler(runtime) ==
              pal4::UiFrameManagerShowSettleSelect::kHandlerStorage,
          "thunk returns static handler storage");
    Check(runtime.calls == std::vector<std::string>{
                               "allocate", "write:16388:4709040",
                               "write:16384:8664540", "manager",
                               "insert", "release"},
          "registration preserves allocation, field and release order");

    Runtime empty;
    empty.allocation = 0u;
    Check(pal4::UiFrameManagerShowSettleSelect::RegisterHandler(empty) ==
              pal4::UiFrameManagerShowSettleSelect::kHandlerStorage &&
              empty.calls ==
                  std::vector<std::string>{"allocate", "manager", "insert"},
          "null allocation still inserts null handler without release");

    Runtime failed;
    failed.throw_insert = true;
    bool caught = false;
    try {
        (void)pal4::UiFrameManagerShowSettleSelect::RegisterHandler(failed);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed.calls.back() == "release",
          "failed tree insertion releases temporary handler");
}
