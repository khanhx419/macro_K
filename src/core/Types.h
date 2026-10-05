#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

namespace MacroK {

enum class EventType {
    MouseMove,
    MouseDown,
    MouseUp,
    MouseWheel,
    MouseHWheel,
    KeyDown,
    KeyUp
};

enum class MouseButton {
    None,
    Left,
    Right,
    Middle,
    XButton1,
    XButton2
};

struct MacroEvent {
    EventType type;
    uint32_t delayMs;        // Delay before executing this event (relative to previous event)
    uint64_t timestampMs;    // Absolute timestamp from recording start

    // Mouse attributes
    int32_t x{0};
    int32_t y{0};
    MouseButton button{MouseButton::None};
    int32_t wheelDelta{0};

    // Keyboard attributes
    uint32_t vkCode{0};
    uint32_t scanCode{0};
    bool isExtendedKey{false};
};

struct MacroSettings {
    double speedMultiplier{1.0};      // 1.0 = normal, 2.0 = 2x faster, 0.5 = 2x slower
    uint32_t loopCount{0};            // 0 = infinite loop (stop on ESC), 1..N = repeat count
    uint32_t loopDelayMs{500};        // Delay between loops
    bool recordKeyboard{true};        // Record keyboard events (keys)
    bool recordMouse{true};           // Record mouse events (clicks, scroll, moves)
    bool recordMouseMoves{true};      // Record continuous mouse trajectories
    uint32_t mouseMoveIntervalMs{15}; // Minimum interval between recorded mouse moves
    int32_t mouseMoveThresholdPx{3};  // Minimum movement distance in px to filter stationary jitter

    // Customizable Global Hotkeys (Virtual Key codes)
    uint32_t hotkeyRecord{VK_F8};
    uint32_t hotkeyPlay{VK_F9};
    uint32_t hotkeyStop{VK_ESCAPE};
};

} // namespace MacroK
