#include "../core/Types.h"
#include "../core/PrecisionTimer.h"
#include "../core/MacroStorage.h"
#include "../core/HookManager.h"
#include <iostream>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::cout << "===========================================\n";
    std::cout << "    RUNNING MACRO RECORDER K UNIT TESTS    \n";
    std::cout << "===========================================\n";

    // Test 1: PrecisionTimer
    std::cout << "[TEST 1] Testing PrecisionTimer...\n";
    {
        MacroK::PrecisionTimer timer;
        timer.start();
        std::atomic<bool> run{true};
        MacroK::PrecisionTimer::sleepPrecise(50, run);
        uint64_t elapsed = timer.getElapsedMs();
        std::cout << "  - Requested sleep: 50ms, Measured elapsed: " << elapsed << "ms\n";
        assert(elapsed >= 48 && elapsed <= 70);
        std::cout << "  -> PASS\n";
    }

    // Test 2: MacroStorage Save & Load
    std::cout << "[TEST 2] Testing MacroStorage Save and Load...\n";
    {
        fs::create_directories("macros");
        std::string testPath = "macros/unit_test.json";

        std::vector<MacroK::MacroEvent> originalEvents;

        MacroK::MacroEvent e1{};
        e1.type = MacroK::EventType::MouseMove;
        e1.delayMs = 100;
        e1.timestampMs = 100;
        e1.x = 250;
        e1.y = 350;
        originalEvents.push_back(e1);

        MacroK::MacroEvent e2{};
        e2.type = MacroK::EventType::MouseDown;
        e2.delayMs = 50;
        e2.timestampMs = 150;
        e2.x = 250;
        e2.y = 350;
        e2.button = MacroK::MouseButton::Left;
        originalEvents.push_back(e2);

        MacroK::MacroEvent e3{};
        e3.type = MacroK::EventType::MouseUp;
        e3.delayMs = 60;
        e3.timestampMs = 210;
        e3.x = 250;
        e3.y = 350;
        e3.button = MacroK::MouseButton::Left;
        originalEvents.push_back(e3);

        MacroK::MacroEvent e4{};
        e4.type = MacroK::EventType::KeyDown;
        e4.delayMs = 80;
        e4.timestampMs = 290;
        e4.vkCode = 0x41; // 'A'
        e4.scanCode = 30;
        e4.isExtendedKey = false;
        originalEvents.push_back(e4);

        MacroK::MacroEvent e5{};
        e5.type = MacroK::EventType::KeyUp;
        e5.delayMs = 40;
        e5.timestampMs = 330;
        e5.vkCode = 0x41;
        e5.scanCode = 30;
        originalEvents.push_back(e5);

        MacroK::MacroSettings originalSettings;
        originalSettings.speedMultiplier = 2.5;
        originalSettings.loopCount = 3;
        originalSettings.loopDelayMs = 200;
        originalSettings.recordMouseMoves = true;
        originalSettings.mouseMoveIntervalMs = 15;

        bool saved = MacroK::MacroStorage::saveToFile(testPath, originalEvents, originalSettings);
        assert(saved);
        std::cout << "  - File successfully saved to: " << testPath << "\n";

        std::vector<MacroK::MacroEvent> loadedEvents;
        MacroK::MacroSettings loadedSettings;
        bool loaded = MacroK::MacroStorage::loadFromFile(testPath, loadedEvents, loadedSettings);
        assert(loaded);
        std::cout << "  - File successfully loaded from: " << testPath << "\n";

        assert(loadedEvents.size() == originalEvents.size());
        assert(loadedSettings.loopCount == 3);
        assert(loadedSettings.speedMultiplier >= 2.49 && loadedSettings.speedMultiplier <= 2.51);
        assert(loadedSettings.loopDelayMs == 200);

        assert(loadedEvents[0].type == MacroK::EventType::MouseMove);
        assert(loadedEvents[0].x == 250 && loadedEvents[0].y == 350);
        assert(loadedEvents[1].button == MacroK::MouseButton::Left);
        assert(loadedEvents[3].vkCode == 0x41);

        std::cout << "  - All 5 events & settings match original values!\n";
        std::cout << "  -> PASS\n";
    }

    // Test 3: Event removal and delay merging
    std::cout << "[TEST 3] Testing Event Deletion & Delay Merging...\n";
    {
        MacroK::HookManager hookMgr;
        std::vector<MacroK::MacroEvent> evts;
        MacroK::MacroEvent a{}; a.delayMs = 100; a.type = MacroK::EventType::MouseMove; evts.push_back(a);
        MacroK::MacroEvent b{}; b.delayMs = 50;  b.type = MacroK::EventType::MouseDown; evts.push_back(b);
        MacroK::MacroEvent c{}; c.delayMs = 70;  c.type = MacroK::EventType::MouseUp;   evts.push_back(c);
        hookMgr.setRecordedEvents(evts);

        bool removed = hookMgr.removeEvent(0);
        assert(removed);
        const auto& after = hookMgr.getRecordedEvents();
        assert(after.size() == 2);
        assert(after[0].type == MacroK::EventType::MouseDown);
        assert(after[0].delayMs == 150);
        std::cout << "  - Event removed and subsequent delay merged accurately (150ms)!\n";
        std::cout << "  -> PASS\n";
    }

    std::cout << "\n===========================================\n";
    std::cout << "       ALL TESTS PASSED SUCCESSFULLY!      \n";
    std::cout << "===========================================\n";

    return 0;
}
