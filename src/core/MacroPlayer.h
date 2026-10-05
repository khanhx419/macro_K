#pragma once

#include "Types.h"
#include "PrecisionTimer.h"
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

namespace MacroK {

class MacroPlayer {
public:
    using ProgressCallback = std::function<void(size_t currentStep, size_t totalSteps, uint32_t currentLoop, uint32_t totalLoops)>;
    using FinishedCallback = std::function<void(bool completedSuccessfully)>;

    MacroPlayer();
    ~MacroPlayer();

    bool play(const std::vector<MacroEvent>& events, const MacroSettings& settings);
    void stop();
    bool isPlaying() const { return m_isPlaying.load(); }

    void setProgressCallback(ProgressCallback cb) { m_progressCallback = cb; }
    void setFinishedCallback(FinishedCallback cb) { m_finishedCallback = cb; }

private:
    void playbackWorker(std::vector<MacroEvent> events, MacroSettings settings);
    void executeEvent(const MacroEvent& evt);

    std::atomic<bool> m_isPlaying{false};
    std::thread m_playbackThread;

    ProgressCallback m_progressCallback;
    FinishedCallback m_finishedCallback;
};

} // namespace MacroK
