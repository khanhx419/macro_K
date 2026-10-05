#include "../core/Types.h"
#include "../core/HookManager.h"
#include "../core/MacroPlayer.h"
#include "../core/MacroStorage.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <filesystem>
#include <conio.h>

namespace fs = std::filesystem;

class MacroCliApp {
public:
    MacroCliApp() {
        // Set console codepage to UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // Create macros directory if not existing
        fs::create_directories("macros");

        // Ignore control hotkeys from being recorded into macro stream
        updateIgnoredKeys();

        // Bind global hotkey callback
        m_hookManager.setHotkeyCallback([this](uint32_t vkCode) {
            this->onGlobalHotkey(vkCode);
        });

        // Bind playback progress callback
        m_player.setProgressCallback([](size_t current, size_t total, uint32_t currentLoop, uint32_t totalLoops) {
            if (current % 10 == 0 || current == total) {
                std::cout << "\r[Playback] Loop " << currentLoop;
                if (totalLoops > 0) std::cout << "/" << totalLoops;
                std::cout << " - Step " << current << "/" << total << "   " << std::flush;
            }
        });

        m_player.setFinishedCallback([](bool ok) {
            std::cout << "\n[Playback] " << (ok ? "Completed successfully!" : "Stopped by user.") << "\n";
        });
    }

    void updateIgnoredKeys() {
        m_hookManager.setIgnoredKeys({m_settings.hotkeyRecord, m_settings.hotkeyPlay, m_settings.hotkeyStop});
    }

    void run() {
        if (!m_hookManager.init()) {
            std::cerr << "Loi: Khong the khoi tao Windows Low-Level Hook!\n";
            return;
        }

        printBanner();

        bool running = true;
        while (running) {
            printMenu();

            std::cout << "\nChon chuc nang (1-7): ";
            char choice = _getch();
            std::cout << choice << "\n\n";

            switch (choice) {
                case '1':
                    toggleRecording();
                    break;
                case '2':
                    togglePlayback();
                    break;
                case '3':
                    showRecordedEvents();
                    break;
                case '4':
                    saveMacro();
                    break;
                case '5':
                    loadMacro();
                    break;
                case '6':
                    configureSettings();
                    break;
                case '8':
                    deleteEventByIndex();
                    break;
                case '7':
                case 'q':
                case 'Q':
                    running = false;
                    break;
                default:
                    std::cout << "Lua chon khong hop le!\n";
                    break;
            }
        }

        m_player.stop();
        m_hookManager.shutdown();
        std::cout << "Tam biet!\n";
    }

private:
    void printBanner() {
        std::cout << "=========================================================\n";
        std::cout << "        MACRO RECORDER K (Win32 High-Precision Core)     \n";
        std::cout << "=========================================================\n";
        std::cout << " Phim tat toan cuc (Global Hotkeys):\n";
        std::cout << "   [Ghi/Dung] : " << MacroK::MacroStorage::getKeyName(m_settings.hotkeyRecord, 0, false) << "\n";
        std::cout << "   [Phat/Dung]: " << MacroK::MacroStorage::getKeyName(m_settings.hotkeyPlay, 0, false) << "\n";
        std::cout << "   [Dung ESC] : " << MacroK::MacroStorage::getKeyName(m_settings.hotkeyStop, 0, false) << " (Huy lap vo han)\n";
        std::cout << "=========================================================\n";
    }

    void printMenu() {
        std::cout << "\n---------------------------------------------------------\n";
        std::cout << "TRANG THAI HIEN TAI:\n";
        std::cout << " - Ghi am: " << (m_hookManager.isRecording() ? "[DANG GHI...]" : "[DUNG]") << "\n";
        std::cout << " - Phat lai: " << (m_player.isPlaying() ? "[DANG PHAT...]" : "[DUNG]") << "\n";
        std::cout << " - So luong su kien trong bo nho: " << m_hookManager.getRecordedEvents().size() << "\n";
        std::cout << " - Toc do phat: " << m_settings.speedMultiplier << "x | So vong lap: " 
                  << (m_settings.loopCount == 0 ? "Vo han (0)" : std::to_string(m_settings.loopCount)) << "\n";
        std::cout << "---------------------------------------------------------\n";
        std::cout << " [1] Bat / Tat Ghi (Toggle Record)\n";
        std::cout << " [2] Phat lai Macro (Play Macro)\n";
        std::cout << " [3] Xem danh sach su kien vua ghi (View Events)\n";
        std::cout << " [4] Luu Macro ra file JSON (Save)\n";
        std::cout << " [5] Tai Macro tu file JSON (Load)\n";
        std::cout << " [6] Cai dat toc do, vong lap, thu phim/chuot (Settings)\n";
        std::cout << " [8] Xoa 1 su kien theo STT (Delete Action)\n";
        std::cout << " [7] Thoat (Exit)\n";
    }

    void onGlobalHotkey(uint32_t vkCode) {
        if (vkCode == m_settings.hotkeyRecord) {
            toggleRecording();
        } else if (vkCode == m_settings.hotkeyPlay) {
            togglePlayback();
        } else if (vkCode == m_settings.hotkeyStop) {
            emergencyStop();
        }
    }

    void toggleRecording() {
        if (m_player.isPlaying()) {
            std::cout << "\n[!] Dang phat macro, vui long dung phat truoc khi ghi.\n";
            return;
        }

        if (!m_hookManager.isRecording()) {
            std::cout << "\n>>> BAT DAU GHI MACRO (Nhan phim Ghi hoac 1 de dung)... <<<\n";
            m_hookManager.startRecording(m_settings);
        } else {
            m_hookManager.stopRecording();
            std::cout << "\n>>> DA DUNG GHI! Tong so su kien: " 
                      << m_hookManager.getRecordedEvents().size() << " <<<\n";
        }
    }

    void togglePlayback() {
        if (m_hookManager.isRecording()) {
            std::cout << "\n[!] Dang ghi am, vui long dung ghi truoc khi phat.\n";
            return;
        }

        if (m_player.isPlaying()) {
            std::cout << "\n>>> DUNG PHAT LAI MACRO <<<\n";
            m_player.stop();
        } else {
            const auto& events = m_hookManager.getRecordedEvents();
            if (events.empty()) {
                std::cout << "\n[!] Khong co su kien nao trong bo nho de phat!\n";
                return;
            }

            std::cout << "\n>>> BAT DAU PHAT LAI (" << events.size() << " su kien, toc do " 
                      << m_settings.speedMultiplier << "x, lap " 
                      << (m_settings.loopCount == 0 ? "vo han" : std::to_string(m_settings.loopCount)) 
                      << " lan)... <<<\n";
            std::cout << ">>> Nhan phim Dung hoac ESC de dung bat ky luc nao! <<<\n";
            m_player.play(events, m_settings);
        }
    }

    void emergencyStop() {
        if (m_hookManager.isRecording()) {
            m_hookManager.stopRecording();
            std::cout << "\n[!] Da dung ghi am!\n";
        }
        if (m_player.isPlaying()) {
            m_player.stop();
            std::cout << "\n[!] Da dung phat lai!\n";
        }
    }

    void showRecordedEvents() {
        const auto& events = m_hookManager.getRecordedEvents();
        if (events.empty()) {
            std::cout << "\n[!] Chua co su kien nao duoc ghi!\n";
            return;
        }

        std::cout << "\n=== DANH SACH SU KIEN (" << events.size() << " su kien) ===\n";
        std::cout << std::left << std::setw(6) << "STT"
                  << std::setw(14) << "Loai"
                  << std::setw(10) << "Delay(ms)"
                  << std::setw(24) << "Chi tiet" << "\n";
        std::cout << "---------------------------------------------------------\n";

        size_t count = events.size();
        size_t limit = count > 30 ? 30 : count;

        for (size_t i = 0; i < limit; ++i) {
            const auto& e = events[i];
            std::cout << std::left << std::setw(6) << (i + 1)
                      << std::setw(14) << MacroK::MacroStorage::eventTypeToString(e.type)
                      << std::setw(10) << e.delayMs;

            std::string detail;
            if (e.type == MacroK::EventType::MouseMove) {
                detail = "X=" + std::to_string(e.x) + " Y=" + std::to_string(e.y);
            } else if (e.type == MacroK::EventType::MouseDown || e.type == MacroK::EventType::MouseUp) {
                detail = MacroK::MacroStorage::mouseButtonToString(e.button) + " (" + std::to_string(e.x) + ", " + std::to_string(e.y) + ")";
            } else if (e.type == MacroK::EventType::KeyDown || e.type == MacroK::EventType::KeyUp) {
                detail = "Key: " + MacroK::MacroStorage::getKeyName(e.vkCode, e.scanCode, e.isExtendedKey) + " (VK=" + std::to_string(e.vkCode) + ")";
            } else if (e.type == MacroK::EventType::MouseWheel) {
                detail = "Delta: " + std::to_string(e.wheelDelta);
            }
            std::cout << detail << "\n";
        }

        if (count > limit) {
            std::cout << "... va " << (count - limit) << " su kien nua.\n";
        }
    }

    void saveMacro() {
        const auto& events = m_hookManager.getRecordedEvents();
        if (events.empty()) {
            std::cout << "\n[!] Bo nho trong, khong co su kien de luu!\n";
            return;
        }

        std::string filename;
        std::cout << "\nNhap ten file de luu (vi du: macro1): ";
        std::cin >> filename;
        if (filename.find(".json") == std::string::npos) {
            filename += ".json";
        }

        std::string path = "macros/" + filename;
        if (MacroK::MacroStorage::saveToFile(path, events, m_settings)) {
            std::cout << ">>> Da luu thanh cong: " << path << " (" << events.size() << " su kien) <<<\n";
        } else {
            std::cout << "Loi: Khong the ghi file!\n";
        }
    }

    void loadMacro() {
        std::cout << "\nDanh sach macro co san trong thu muc 'macros':\n";
        std::vector<std::string> fileList;
        for (const auto& entry : fs::directory_iterator("macros")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                fileList.push_back(entry.path().filename().string());
                std::cout << "  - " << fileList.back() << "\n";
            }
        }

        if (fileList.empty()) {
            std::cout << "  (Khong co file .json nao)\n";
        }

        std::string filename;
        std::cout << "\nNhap ten file muon mo: ";
        std::cin >> filename;
        if (filename.find(".json") == std::string::npos) {
            filename += ".json";
        }

        std::string path = "macros/" + filename;
        std::vector<MacroK::MacroEvent> loadedEvents;
        MacroK::MacroSettings loadedSettings;

        if (MacroK::MacroStorage::loadFromFile(path, loadedEvents, loadedSettings)) {
            m_hookManager.setRecordedEvents(loadedEvents);
            m_settings = loadedSettings;
            updateIgnoredKeys();
            std::cout << ">>> Da nap thanh cong: " << path << " (" << loadedEvents.size() << " su kien) <<<\n";
            std::cout << "    Toc do: " << m_settings.speedMultiplier << "x | So vong lap: " << m_settings.loopCount << "\n";
        } else {
            std::cout << "Loi: Khong the doc file hoac file khong dung dinh dang!\n";
        }
    }

    void deleteEventByIndex() {
        const auto& events = m_hookManager.getRecordedEvents();
        if (events.empty()) {
            std::cout << "\n[!] Bo nho trong, khong co su kien nao de xoa!\n";
            return;
        }

        std::cout << "\nDanh sach dang co " << events.size() << " su kien.";
        std::cout << "\nNhap STT su kien muon xoa (1 - " << events.size() << "): ";
        size_t idx;
        if (std::cin >> idx && idx >= 1 && idx <= events.size()) {
            m_hookManager.removeEvent(idx - 1);
            std::cout << ">>> Da xoa thanh cong su kien #" << idx << "! Con lai: " 
                      << m_hookManager.getRecordedEvents().size() << " su kien. <<<\n";
        } else {
            std::cout << "So thu tu khong hop le!\n";
        }
    }

    void configureSettings() {
        std::cout << "\n=== CAI DAT MACRO ===\n";
        std::cout << "1. Toc do phat hien tai: " << m_settings.speedMultiplier << "x\n";
        std::cout << "2. So lan lap: " << (m_settings.loopCount == 0 ? "Vo han (0)" : std::to_string(m_settings.loopCount)) << "\n";
        std::cout << "3. Do tre giua cac vong lap: " << m_settings.loopDelayMs << " ms\n";
        std::cout << "4. Thu ban phim: " << (m_settings.recordKeyboard ? "BAT" : "TAT") << "\n";
        std::cout << "5. Thu chuot (Click & Cuon): " << (m_settings.recordMouse ? "BAT" : "TAT") << "\n";
        std::cout << "6. Thu di chuyen chuot: " << (m_settings.recordMouseMoves ? "BAT" : "TAT") << "\n";
        std::cout << "7. Phim tat Ghi: " << MacroK::MacroStorage::getKeyName(m_settings.hotkeyRecord, 0, false) << "\n";
        std::cout << "8. Phim tat Phat: " << MacroK::MacroStorage::getKeyName(m_settings.hotkeyPlay, 0, false) << "\n";
        std::cout << "9. Phim tat Dung: " << MacroK::MacroStorage::getKeyName(m_settings.hotkeyStop, 0, false) << "\n";

        std::cout << "\nChon muc can doi (1-9, hoac phim khac de quay lai): ";
        char opt = _getch();
        std::cout << opt << "\n";

        if (opt == '1') {
            std::cout << "Nhap toc do moi (vi du: 1.0, 1.5, 2.0, 5.0): ";
            double spd;
            if (std::cin >> spd && spd > 0.05) {
                m_settings.speedMultiplier = spd;
                std::cout << "Da cap nhat toc do: " << spd << "x\n";
            }
        } else if (opt == '2') {
            std::cout << "Nhap so lan lap (0 = vo han, >= 1): ";
            uint32_t count;
            if (std::cin >> count) {
                m_settings.loopCount = count;
                std::cout << "Da cap nhat so lan lap: " << count << "\n";
            }
        } else if (opt == '3') {
            std::cout << "Nhap do tre giua cac vong lap (ms): ";
            uint32_t delay;
            if (std::cin >> delay) {
                m_settings.loopDelayMs = delay;
                std::cout << "Da cap nhat delay: " << delay << " ms\n";
            }
        } else if (opt == '4') {
            m_settings.recordKeyboard = !m_settings.recordKeyboard;
            std::cout << "Da chuyen thu ban phim thanh: " << (m_settings.recordKeyboard ? "BAT" : "TAT") << "\n";
        } else if (opt == '5') {
            m_settings.recordMouse = !m_settings.recordMouse;
            std::cout << "Da chuyen thu chuot thanh: " << (m_settings.recordMouse ? "BAT" : "TAT") << "\n";
        } else if (opt == '6') {
            m_settings.recordMouseMoves = !m_settings.recordMouseMoves;
            std::cout << "Da chuyen thu di chuyen chuot thanh: " << (m_settings.recordMouseMoves ? "BAT" : "TAT") << "\n";
        }
    }

    MacroK::HookManager m_hookManager;
    MacroK::MacroPlayer m_player;
    MacroK::MacroSettings m_settings;
};

int main() {
    MacroCliApp app;
    app.run();
    return 0;
}
