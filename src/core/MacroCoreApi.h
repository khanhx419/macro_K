#pragma once

#include <cstdint>

#ifdef MACRO_EXPORTS
#define MACRO_API __declspec(dllexport)
#else
#define MACRO_API
#endif

extern "C" {
    MACRO_API bool Macro_Init();
    MACRO_API void Macro_Shutdown();

    MACRO_API bool Macro_StartRecord(bool recordMouseMoves, uint32_t mouseIntervalMs);
    MACRO_API void Macro_StopRecord();
    MACRO_API bool Macro_IsRecording();

    MACRO_API bool Macro_PlayCurrent(double speedMultiplier, uint32_t loopCount, uint32_t loopDelayMs);
    MACRO_API bool Macro_PlayFile(const char* filePath, double speedMultiplier, uint32_t loopCount, uint32_t loopDelayMs);
    MACRO_API void Macro_StopPlay();
    MACRO_API bool Macro_IsPlaying();

    MACRO_API bool Macro_SaveFile(const char* filePath);
    MACRO_API bool Macro_LoadFile(const char* filePath);
    MACRO_API uint32_t Macro_GetEventCount();
    MACRO_API void Macro_ClearEvents();
}
