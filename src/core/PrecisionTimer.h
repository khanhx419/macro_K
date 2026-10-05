#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <cstdint>
#include <atomic>

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

namespace MacroK {

class PrecisionTimer {
public:
    PrecisionTimer() {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        m_frequency = freq.QuadPart;
        timeBeginPeriod(1); // Set Windows system timer resolution to 1ms
    }

    ~PrecisionTimer() {
        timeEndPeriod(1);
    }

    void start() {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        m_startTime = counter.QuadPart;
    }

    // Elapsed time in milliseconds since start() was called
    uint64_t getElapsedMs() const {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return static_cast<uint64_t>((counter.QuadPart - m_startTime) * 1000 / m_frequency);
    }

    // Elapsed time in microseconds
    uint64_t getElapsedUs() const {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return static_cast<uint64_t>((counter.QuadPart - m_startTime) * 1000000 / m_frequency);
    }

    // High precision sleep that allows cancellation
    static bool sleepPrecise(uint32_t milliseconds, const std::atomic<bool>& shouldContinue) {
        if (milliseconds == 0) return shouldContinue.load();

        LARGE_INTEGER freq, startCounter, currentCounter;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&startCounter);

        int64_t targetTicks = (static_cast<int64_t>(milliseconds) * freq.QuadPart) / 1000;

        // If sleep is long enough, do coarse sleep first to avoid 100% CPU usage
        if (milliseconds > 5) {
            uint32_t coarseSleep = milliseconds - 4;
            // Sleep in small increments of 10ms so we can respond quickly to stop hotkeys
            while (coarseSleep > 0 && shouldContinue.load()) {
                uint32_t step = (coarseSleep > 10) ? 10 : coarseSleep;
                Sleep(step);
                coarseSleep -= step;
            }
        }

        // Spin-wait the remaining fraction of time for sub-millisecond accuracy
        while (shouldContinue.load()) {
            QueryPerformanceCounter(&currentCounter);
            if ((currentCounter.QuadPart - startCounter.QuadPart) >= targetTicks) {
                break;
            }
            // Yield CPU slice to other threads while spinning if more than 1ms left
            int64_t remainingTicks = targetTicks - (currentCounter.QuadPart - startCounter.QuadPart);
            if ((remainingTicks * 1000 / freq.QuadPart) > 1) {
                Sleep(0);
            }
        }

        return shouldContinue.load();
    }

private:
    int64_t m_frequency{0};
    int64_t m_startTime{0};
};

} // namespace MacroK
