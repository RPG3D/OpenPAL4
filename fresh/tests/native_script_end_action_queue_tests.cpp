#include "pal4/native_script_end_action_queue.h"
#include "pal4/native_script_frame_runner.h"

#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void Append(pal4::NativeScriptEndActionQueue& queue, unsigned actor) {
    auto task = std::make_unique<pal4::PlayerEndActionWaitState>();
    task->actor = actor;
    queue.Append(std::move(task));
}

void QueueCases() {
    pal4::NativeScriptEndActionQueue queue;
    for (unsigned actor = 1; actor <= 3; ++actor) Append(queue, actor);
    auto complete = [&](unsigned mask) {
        return queue.Process([&](auto& task) {
            if (mask & (1u << task.actor)) queue.Close(task);
        });
    };
    Check(!complete(1u << 2) && queue.Size() == 2 && queue.BlocksScript(),
          "closing a middle wait retains the unfinished tail and execution flag");
    Check(complete(1u << 3) && queue.Size() == 1 && !queue.BlocksScript(),
          "closed tail resumes despite unfinished hidden first actor");
    Check(!complete(0) && queue.Size() == 1,
          "retained hidden wait does not repeatedly resume later script waits");
    unsigned retained{};
    queue.Visit([&](const auto& task) { retained = task.actor; });
    Check(retained == 1, "unfinished original wait is retained rather than marked complete");
    Append(queue, 4);
    Check(!complete(1u << 1) && queue.Size() == 1 && queue.BlocksScript(),
          "old wait completion cannot release a newly appended blocking tail");
    Check(complete(1u << 4) && queue.Empty() && !queue.BlocksScript(),
          "new tail completion releases the new wait");
    Append(queue, 5);
    queue.Clear();
    Check(queue.Empty() && !queue.BlocksScript(), "context cleanup retires all retained waits");
    Append(queue, 6);
    queue.AppendCompletedHandler(0x849544u, 1, "101-Player-Movment");
    queue.AppendCompletedHandler(0x849544u, 1, "101-Player-Movment");
    Check(queue.Size() == 2, "shared registration replaces only the same marker and name");
    queue.AppendCompletedHandler(0x849550u, 2, "101-Player-Movment");
    Check(queue.BlocksScript() && complete(0) && queue.Size() == 1 &&
              !queue.BlocksScript(),
          "actual closed position/angle tail releases the hidden end wait without completing it");
    Check(!complete(0) && queue.Size() == 1,
          "retained hidden NPC does not repeatedly resume subsequent waits");
    queue.AppendCompletedHandler(0x849544u, 1, "101-Player-Movment");
    Append(queue, 7);
    Check(!complete(0) && queue.Size() == 2 && queue.BlocksScript(),
          "closed position preceding a new blocking tail cannot release that tail");
    Check(complete(1u << 7) && queue.Size() == 1 && !queue.BlocksScript(),
          "later blocking tail still controls resumption");
    queue.Clear();
}

void ObservedCameraCases() {
    pal4::NativeScriptEndActionQueue queue;
    int camera{}, newer{};
    bool closed{};
    unsigned polls{};
    auto update = [](auto&) { throw std::runtime_error("no end action in camera fixture"); };
    queue.AppendObservedHandler(&camera, 0x8495b8u, 1u, [&] { ++polls; return closed; });
    queue.AppendCompletedHandler(0x849550u, 2, "105-Player-Movment");
    Check(queue.BlocksScript() && queue.Process(update) && !queue.BlocksScript() &&
              queue.Size() == 1 && polls == 1 && !closed,
          "closed rotation tail releases earlier unfinished camera without completing it");
    Check(!queue.Process(update) && queue.Size() == 1,
          "retained camera cannot repeatedly release later script instructions");
    queue.AppendCompletedHandler(0x849550u, 2, "105-Player-Movment");
    queue.AppendObservedHandler(&newer, 0x8495b8u, 1u, [] { return false; });
    closed = true;
    Check(!queue.Process(update) && queue.BlocksScript() && queue.Size() == 1,
          "old camera completion and preceding rotation cannot release new blocking tail");
    queue.RemoveObservedHandler(&newer);
    Check(queue.Empty() && !queue.BlocksScript(),
          "creating owner cleanup removes observed callback and its final pause flag");
    queue.RemoveObservedHandler(&camera);
    queue.AppendObservedHandler(&camera, 0x8495b8u, 0u, [] { return false; });
    Check(!queue.BlocksScript() && !queue.Process(update),
          "nonblocking camera observation retains ownership without pausing script");
    queue.Clear();
    Check(queue.Empty(), "context clear retires observed callbacks");
}

class Batch final : public pal4::IGameScriptCallbackRuntime,
                    public pal4::INativeScriptFrameWaitRuntime {
public:
    pal4::NativeScriptEndActionQueue queue;
    bool immediate{};
    unsigned registered{}, complete_mask{}, after{};
    void Invoke(int id, asIScriptGeneric&) override {
        if (id == -55) immediate = true;
        else if (id == -56) immediate = false;
        else if (id == -273) {
            Append(queue, ++registered);
            Check(asGetActiveContext()->Suspend() >= 0, "real SDK callback suspend");
        } else if (id == -57) ++after;
        else if (id == -250) queue.AppendCompletedHandler(0x849544u, 1, "101-Player-Movment");
        else if (id == -253) queue.AppendCompletedHandler(0x849550u, 2, "101-Player-Movment");
        else throw std::runtime_error("unexpected batch callback");
    }
    bool BlocksScript() const override { return queue.BlocksScript(); }
    bool Tick(float) override {
        return queue.Process([&](auto& task) {
            if (complete_mask & (1u << task.actor)) queue.Close(task);
        });
    }
};

void SdkBatchCases() {
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[] = "void Batch() { giIMMBegin(); "
        "giNpcEndAction(\"hidden\", false); giNpcEndAction(\"visibleA\", false); "
        "giNpcEndAction(\"visibleB\", false); giIMMEnd(); giNewGame(); }";
    Check(engine.Raw()->AddScriptSection("batch", "batch.cs", source, sizeof(source)-1) >= 0 &&
          engine.Raw()->Build("batch") >= 0, "compile actual IMM and NPC callback signatures");
    Batch batch;
    engine.SetCallbacks(&batch);
    pal4::NativeScriptFrameRunner runner(engine);
    runner.SetImmediateModeProvider([&] { return batch.immediate; });
    runner.Queue("batch", "Batch");
    using Status = pal4::NativeScriptFrameStatus;
    Check(runner.TickUntilBoundary(.25f, &batch, [] { return false; }) == Status::suspended &&
          batch.registered == 3 && batch.after == 0 && !batch.immediate && batch.queue.Size() == 3,
          "IMM registers all three blocking ends before waiting outside the batch");
    batch.complete_mask = (1u << 2) | (1u << 3);
    Check(runner.TickUntilBoundary(.25f, &batch, [] { return false; }) == Status::finished &&
          batch.after == 1 && batch.queue.Size() == 1,
          "visible tail completion resumes actual SDK context with hidden wait retained");
    batch.queue.Clear(); batch.registered = 0; batch.complete_mask = 0; batch.after = 0;
    runner.Queue("batch", "Batch");
    Check(runner.TickUntilBoundary(.25f, &batch, [&] { return batch.registered == 1; }) ==
              Status::suspended && batch.registered == 1 && batch.after == 0,
          "state boundary still stops IMM before later callbacks");
    runner.ClearExecution(); batch.queue.Clear();
    constexpr char rest[] = "void Rest() { giIMMBegin(); "
        "giNpcEndAction(\"hidden\", true); giPlayerCurrentSetPos(-16.4f,-0.2f,-17.5f); "
        "giPlayerCurrentSetAng(0.0f); giIMMEnd(); giNewGame(); }";
    Check(engine.Raw()->AddScriptSection("rest", "rest.cs", rest, sizeof(rest)-1) >= 0 &&
          engine.Raw()->Build("rest") >= 0, "compile actual rest tail signatures");
    batch.registered = 0; batch.complete_mask = 0; batch.after = 0;
    runner.Queue("rest", "Rest");
    Check(runner.TickUntilBoundary(.25f, &batch, [] { return false; }) == Status::suspended &&
          batch.queue.Size() == 3 && batch.after == 0,
          "rest IMM retains all three registrations in source order");
    Check(runner.TickUntilBoundary(.25f, &batch, [] { return false; }) == Status::finished &&
          batch.queue.Size() == 1 && batch.after == 1,
          "real SDK rest continuation resumes from closed sibling tail with hidden end unfinished");
    batch.queue.Clear();
    engine.SetCallbacks(nullptr);
}
}

int main() {
    try {
        QueueCases(); ObservedCameraCases(); SdkBatchCases();
        std::cout << "PASS: shared end-action dispatch and SDK IMM batch\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
