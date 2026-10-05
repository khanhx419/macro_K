#include "MacroPlayer.h"
#include <iostream>
#include <cmath>

namespace MacroK {

MacroPlayer::MacroPlayer() = default;

MacroPlayer::~MacroPlayer() {
    stop();
}

bool MacroPlayer::play(const std::vector<MacroEvent>& events, const MacroSettings& settings) {
    if (events.empty() || m_isPlaying.load()) {
        return false;
    }

    m_isPlaying.store(true);
    m_playbackThread = std::thread(&MacroPlayer::playbackWorker, this, events, settings);
    return true;
}

void MacroPlayer::stop() {
    if (!m_isPlaying.load()) {
        return;
    }

    m_isPlaying.store(false);
    if (m_playbackThread.joinable()) {
        m_playbackThread.join();
    }
}

void MacroPlayer::playbackWorker(std::vector<MacroEvent> events, MacroSettings settings) {
    uint32_t currentLoop = 1;
    bool completedAll = true;

    while (m_isPlaying.load()) {
        if (settings.loopCount > 0 && currentLoop > settings.loopCount) {
            break;
        }

        size_t total = events.size();
        for (size_t i = 0; i < total; ++i) {
            if (!m_isPlaying.load()) {
                completedAll = false;
                break;
            }

            const auto& evt = events[i];

            // Filter out events that the user has disabled in settings
            bool isMouseAction = (evt.type == EventType::MouseMove ||
                                  evt.type == EventType::MouseDown ||
                                  evt.type == EventType::MouseUp ||
                                  evt.type == EventType::MouseWheel ||
                                  evt.type == EventType::MouseHWheel);

            if (!settings.recordMouse && isMouseAction) {
                // If user disabled mouse, skip all mouse playback
                continue;
            }
            if (!settings.recordMouseMoves && evt.type == EventType::MouseMove) {
                // If user disabled mouse movement, skip trajectory playback
                continue;
            }
            bool isKeyboardAction = (evt.type == EventType::KeyDown || evt.type == EventType::KeyUp);
            if (!settings.recordKeyboard && isKeyboardAction) {
                // If user disabled keyboard, skip keyboard playback
                continue;
            }

            // Calculate scaled delay
            uint32_t scaledDelay = evt.delayMs;
            if (settings.speedMultiplier > 0.001) {
                scaledDelay = static_cast<uint32_t>(std::round(evt.delayMs / settings.speedMultiplier));
            }

            // Sleep with precision
            if (!PrecisionTimer::sleepPrecise(scaledDelay, m_isPlaying)) {
                completedAll = false;
                break;
            }

            // Execute the action
            executeEvent(evt, settings);

            if (m_progressCallback) {
                m_progressCallback(i + 1, total, currentLoop, settings.loopCount);
            }
        }

        if (!m_isPlaying.load()) {
            break;
        }

        currentLoop++;
        if (settings.loopCount == 0 || currentLoop <= settings.loopCount) {
            if (!PrecisionTimer::sleepPrecise(settings.loopDelayMs, m_isPlaying)) {
                completedAll = false;
                break;
            }
        }
    }

    m_isPlaying.store(false);

    if (m_finishedCallback) {
        m_finishedCallback(completedAll);
    }
}

void MacroPlayer::executeEvent(const MacroEvent& evt, const MacroSettings& settings) {
    INPUT input{};

    int vLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (vWidth <= 1) vWidth = GetSystemMetrics(SM_CXSCREEN);
    if (vHeight <= 1) vHeight = GetSystemMetrics(SM_CYSCREEN);

    auto toNormalizedX = [&](int32_t x) -> LONG {
        return static_cast<LONG>((static_cast<double>(x - vLeft) * 65535.0) / (vWidth > 1 ? vWidth - 1 : 1));
    };

    auto toNormalizedY = [&](int32_t y) -> LONG {
        return static_cast<LONG>((static_cast<double>(y - vTop) * 65535.0) / (vHeight > 1 ? vHeight - 1 : 1));
    };

    switch (evt.type) {
        case EventType::MouseMove: {
            if (!settings.recordMouse || !settings.recordMouseMoves) return;
            SetCursorPos(evt.x, evt.y);
            input.type = INPUT_MOUSE;
            input.mi.dx = toNormalizedX(evt.x);
            input.mi.dy = toNormalizedY(evt.y);
            input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
            SendInput(1, &input, sizeof(INPUT));
            break;
        }
        case EventType::MouseDown:
        case EventType::MouseUp: {
            if (!settings.recordMouse) return;

            input.type = INPUT_MOUSE;
            if (settings.recordMouseMoves) {
                SetCursorPos(evt.x, evt.y);
                input.mi.dx = toNormalizedX(evt.x);
                input.mi.dy = toNormalizedY(evt.y);
            }

            DWORD baseFlag = (evt.type == EventType::MouseDown) ? 0 : 1; // 0=Down, 1=Up

            switch (evt.button) {
                case MouseButton::Left:
                    input.mi.dwFlags = (baseFlag == 0) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
                    break;
                case MouseButton::Right:
                    input.mi.dwFlags = (baseFlag == 0) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
                    break;
                case MouseButton::Middle:
                    input.mi.dwFlags = (baseFlag == 0) ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
                    break;
                case MouseButton::XButton1:
                    input.mi.dwFlags = (baseFlag == 0) ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
                    input.mi.mouseData = XBUTTON1;
                    break;
                case MouseButton::XButton2:
                    input.mi.dwFlags = (baseFlag == 0) ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
                    input.mi.mouseData = XBUTTON2;
                    break;
                default:
                    return;
            }

            if (settings.recordMouseMoves) {
                input.mi.dwFlags |= MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
            }
            SendInput(1, &input, sizeof(INPUT));
            break;
        }
        case EventType::MouseWheel: {
            if (!settings.recordMouse) return;
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = static_cast<DWORD>(evt.wheelDelta);
            SendInput(1, &input, sizeof(INPUT));
            break;
        }
        case EventType::MouseHWheel: {
            if (!settings.recordMouse) return;
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
            input.mi.mouseData = static_cast<DWORD>(evt.wheelDelta);
            SendInput(1, &input, sizeof(INPUT));
            break;
        }
        case EventType::KeyDown:
        case EventType::KeyUp: {
            if (!settings.recordKeyboard) return;
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = static_cast<WORD>(evt.vkCode);
            input.ki.wScan = static_cast<WORD>(evt.scanCode);
            input.ki.dwFlags = 0;

            if (evt.isExtendedKey) {
                input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
            }
            if (evt.type == EventType::KeyUp) {
                input.ki.dwFlags |= KEYEVENTF_KEYUP;
            }

            SendInput(1, &input, sizeof(INPUT));
            break;
        }
    }
}

} // namespace MacroK
