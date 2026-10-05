#define MACRO_EXPORTS
#include "MacroCoreApi.h"
#include "HookManager.h"
#include "MacroPlayer.h"
#include "MacroStorage.h"

static MacroK::HookManager g_hookManager;
static MacroK::MacroPlayer g_player;

extern "C" {

MACRO_API bool Macro_Init() {
    return g_hookManager.init();
}

MACRO_API void Macro_Shutdown() {
    g_player.stop();
    g_hookManager.shutdown();
}

MACRO_API bool Macro_StartRecord(bool recordMouseMoves, uint32_t mouseIntervalMs) {
    MacroK::MacroSettings settings;
    settings.recordMouseMoves = recordMouseMoves;
    settings.mouseMoveIntervalMs = mouseIntervalMs;
    return g_hookManager.startRecording(settings);
}

MACRO_API void Macro_StopRecord() {
    g_hookManager.stopRecording();
}

MACRO_API bool Macro_IsRecording() {
    return g_hookManager.isRecording();
}

MACRO_API bool Macro_PlayCurrent(double speedMultiplier, uint32_t loopCount, uint32_t loopDelayMs) {
    if (g_hookManager.isRecording()) return false;
    MacroK::MacroSettings settings;
    settings.speedMultiplier = speedMultiplier;
    settings.loopCount = loopCount;
    settings.loopDelayMs = loopDelayMs;
    return g_player.play(g_hookManager.getRecordedEvents(), settings);
}

MACRO_API bool Macro_PlayFile(const char* filePath, double speedMultiplier, uint32_t loopCount, uint32_t loopDelayMs) {
    if (!filePath || g_hookManager.isRecording()) return false;
    std::vector<MacroK::MacroEvent> events;
    MacroK::MacroSettings fileSettings;
    if (!MacroK::MacroStorage::loadFromFile(filePath, events, fileSettings)) {
        return false;
    }
    if (speedMultiplier > 0.0) fileSettings.speedMultiplier = speedMultiplier;
    if (loopCount > 0) fileSettings.loopCount = loopCount;
    if (loopDelayMs > 0) fileSettings.loopDelayMs = loopDelayMs;
    return g_player.play(events, fileSettings);
}

MACRO_API void Macro_StopPlay() {
    g_player.stop();
}

MACRO_API bool Macro_IsPlaying() {
    return g_player.isPlaying();
}

MACRO_API bool Macro_SaveFile(const char* filePath) {
    if (!filePath) return false;
    return MacroK::MacroStorage::saveToFile(filePath, g_hookManager.getRecordedEvents());
}

MACRO_API bool Macro_LoadFile(const char* filePath) {
    if (!filePath) return false;
    std::vector<MacroK::MacroEvent> events;
    MacroK::MacroSettings settings;
    if (MacroK::MacroStorage::loadFromFile(filePath, events, settings)) {
        g_hookManager.setRecordedEvents(events);
        return true;
    }
    return false;
}

MACRO_API uint32_t Macro_GetEventCount() {
    return static_cast<uint32_t>(g_hookManager.getRecordedEvents().size());
}

MACRO_API void Macro_ClearEvents() {
    g_hookManager.clearEvents();
}

}
