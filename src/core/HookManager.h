#pragma once

#include "Types.h"
#include "PrecisionTimer.h"
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

namespace MacroK {

class HookManager {
public:
    using EventCallback = std::function<void(const MacroEvent&)>;
    using HotkeyCallback = std::function<void(uint32_t vkCode)>;

    HookManager();
    ~HookManager();

    bool init();
    void shutdown();

    bool startRecording(const MacroSettings& settings = MacroSettings{});
    void stopRecording();
    bool isRecording() const { return m_isRecording.load(); }

    const std::vector<MacroEvent>& getRecordedEvents() const;
    void setRecordedEvents(const std::vector<MacroEvent>& events);
    bool removeEvent(size_t index);
    bool removeEvents(std::vector<size_t> indices);
    void clearEvents();

    void setEventCallback(EventCallback cb) { m_eventCallback = cb; }
    void setHotkeyCallback(HotkeyCallback cb) { m_hotkeyCallback = cb; }

    void setSettings(const MacroSettings& settings);
    MacroSettings getSettings() const;

    void addIgnoredKey(uint32_t vkCode);
    void setIgnoredKeys(const std::vector<uint32_t>& keys);
    void clearIgnoredKeys();

private:
    void hookThreadFunc(std::atomic<bool>& ready);
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    void handleMouseEvent(WPARAM wParam, const MSLLHOOKSTRUCT* pMouse);
    void handleKeyboardEvent(WPARAM wParam, const KBDLLHOOKSTRUCT* pKey);

    static HookManager* s_instance;

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_isRecording{false};
    std::atomic<bool> m_ignoreInitialMouseUp{false};
    std::thread m_hookThread;
    DWORD m_hookThreadId{0};
    HHOOK m_mouseHook{nullptr};
    HHOOK m_keyboardHook{nullptr};

    PrecisionTimer m_timer;
    uint64_t m_lastEventTimeMs{0};
    int32_t m_lastRecordedX{-1};
    int32_t m_lastRecordedY{-1};
    uint64_t m_lastMouseMoveTimeMs{0};

    MacroSettings m_settings;
    std::vector<MacroEvent> m_events;
    mutable std::mutex m_eventMutex;

    std::vector<uint32_t> m_ignoredKeys;
    std::mutex m_ignoredKeysMutex;

    EventCallback m_eventCallback;
    HotkeyCallback m_hotkeyCallback;
};

} // namespace MacroK
