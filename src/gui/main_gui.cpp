#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <filesystem>

#include "../core/Types.h"
#include "../core/HookManager.h"
#include "../core/MacroPlayer.h"
#include "../core/MacroStorage.h"

#pragma comment(lib, "comctl32.lib")

namespace fs = std::filesystem;

// Custom Windows Messages for thread-safe UI updates
#define WM_APP_HOTKEY        (WM_APP + 1)
#define WM_APP_EVENT_ADDED   (WM_APP + 2)
#define WM_APP_PLAY_PROGRESS (WM_APP + 3)
#define WM_APP_PLAY_FINISHED (WM_APP + 4)

// Control IDs
#define IDC_BTN_RECORD        1001
#define IDC_BTN_PLAY          1002
#define IDC_BTN_STOP          1003
#define IDC_BTN_OPEN          1004
#define IDC_BTN_SAVE          1005
#define IDC_BTN_CLEAR         1006
#define IDC_LIST_EVENTS       1007
#define IDC_EDIT_LOOP         1008
#define IDC_COMBO_SPEED       1009
#define IDC_EDIT_DELAY        1010
#define IDC_CHK_MOUSE_MOVE    1011
#define IDC_STATUS_LABEL      1012
#define IDC_PROGRESS_LABEL    1013
#define IDC_COMBO_HK_RECORD   1014
#define IDC_COMBO_HK_PLAY     1015
#define IDC_COMBO_HK_STOP     1016
#define IDC_FOOTER_HINT       1017

// Global handles
static HWND g_hWnd = nullptr;
static HWND g_hBtnRecord = nullptr;
static HWND g_hBtnPlay = nullptr;
static HWND g_hBtnStop = nullptr;
static HWND g_hBtnOpen = nullptr;
static HWND g_hBtnSave = nullptr;
static HWND g_hBtnClear = nullptr;
static HWND g_hListEvents = nullptr;
static HWND g_hEditLoop = nullptr;
static HWND g_hComboSpeed = nullptr;
static HWND g_hEditDelay = nullptr;
static HWND g_hChkMouseMove = nullptr;
static HWND g_hComboHkRecord = nullptr;
static HWND g_hComboHkPlay = nullptr;
static HWND g_hComboHkStop = nullptr;
static HWND g_hStatusLabel = nullptr;
static HWND g_hProgressLabel = nullptr;
static HWND g_hFooterHint = nullptr;

static HFONT g_hFontNormal = nullptr;
static HFONT g_hFontBold = nullptr;
static HFONT g_hFontStatus = nullptr;

static MacroK::HookManager g_hookManager;
static MacroK::MacroPlayer g_player;
static MacroK::MacroSettings g_settings;

// Supported hotkeys list
struct HotkeyOption {
    const wchar_t* name;
    uint32_t vkCode;
};

static const HotkeyOption kAvailableHotkeys[] = {
    { L"F1", VK_F1 },
    { L"F2", VK_F2 },
    { L"F3", VK_F3 },
    { L"F4", VK_F4 },
    { L"F5", VK_F5 },
    { L"F6", VK_F6 },
    { L"F7", VK_F7 },
    { L"F8", VK_F8 },
    { L"F9", VK_F9 },
    { L"F10", VK_F10 },
    { L"F11", VK_F11 },
    { L"F12", VK_F12 },
    { L"ESC", VK_ESCAPE },
    { L"Insert", VK_INSERT },
    { L"Delete", VK_DELETE },
    { L"Home", VK_HOME },
    { L"End", VK_END },
    { L"Page Up", VK_PRIOR },
    { L"Page Down", VK_NEXT },
    { L"Pause/Break", VK_PAUSE },
    { L"Scroll Lock", VK_SCROLL },
    { L"` ~ (Tilde)", VK_OEM_3 },
    { L"Num 0", VK_NUMPAD0 },
    { L"Num 1", VK_NUMPAD1 },
    { L"Num 2", VK_NUMPAD2 },
    { L"Num 3", VK_NUMPAD3 },
    { L"Num 4", VK_NUMPAD4 },
    { L"Num 5", VK_NUMPAD5 },
    { L"Num 6", VK_NUMPAD6 },
    { L"Num 7", VK_NUMPAD7 },
    { L"Num 8", VK_NUMPAD8 },
    { L"Num 9", VK_NUMPAD9 },
};

static const size_t kHotkeyCount = sizeof(kAvailableHotkeys) / sizeof(kAvailableHotkeys[0]);

static int FindHotkeyIndex(uint32_t vkCode) {
    for (size_t i = 0; i < kHotkeyCount; ++i) {
        if (kAvailableHotkeys[i].vkCode == vkCode) return static_cast<int>(i);
    }
    return 0;
}

static std::wstring GetHotkeyName(uint32_t vkCode) {
    for (size_t i = 0; i < kHotkeyCount; ++i) {
        if (kAvailableHotkeys[i].vkCode == vkCode) return kAvailableHotkeys[i].name;
    }
    return L"Key_" + std::to_wstring(vkCode);
}

// Helper to set Segoe UI font to controls
static void SetControlFont(HWND hCtrl, HFONT hFont) {
    if (hCtrl && hFont) {
        SendMessage(hCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }
}

// Convert wide string to std::string UTF-8
static std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], size, nullptr, nullptr);
    return str;
}

// Convert std::string to std::wstring
static std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], size);
    return wstr;
}

// Forward declarations
static void UpdateUIState();
static void RefreshListView();

// Read settings from GUI controls
static void SyncSettingsFromUI() {
    wchar_t buf[64];

    // Loop count (0 = infinite)
    GetWindowTextW(g_hEditLoop, buf, 64);
    try {
        g_settings.loopCount = static_cast<uint32_t>(std::stoul(buf));
    } catch (...) {
        g_settings.loopCount = 0;
    }

    // Loop delay
    GetWindowTextW(g_hEditDelay, buf, 64);
    try {
        g_settings.loopDelayMs = static_cast<uint32_t>(std::stoul(buf));
    } catch (...) {
        g_settings.loopDelayMs = 500;
    }

    // Speed multiplier from combo
    int sel = (int)SendMessage(g_hComboSpeed, CB_GETCURSEL, 0, 0);
    switch (sel) {
        case 0: g_settings.speedMultiplier = 0.5; break;
        case 1: g_settings.speedMultiplier = 1.0; break;
        case 2: g_settings.speedMultiplier = 1.5; break;
        case 3: g_settings.speedMultiplier = 2.0; break;
        case 4: g_settings.speedMultiplier = 3.0; break;
        case 5: g_settings.speedMultiplier = 5.0; break;
        case 6: g_settings.speedMultiplier = 10.0; break;
        default: g_settings.speedMultiplier = 1.0; break;
    }

    // Checkbox mouse moves
    g_settings.recordMouseMoves = (SendMessage(g_hChkMouseMove, BM_GETCHECK, 0, 0) == BST_CHECKED);

    // Custom Hotkeys
    int idxRec = (int)SendMessage(g_hComboHkRecord, CB_GETCURSEL, 0, 0);
    if (idxRec >= 0 && idxRec < (int)kHotkeyCount) {
        g_settings.hotkeyRecord = kAvailableHotkeys[idxRec].vkCode;
    }

    int idxPlay = (int)SendMessage(g_hComboHkPlay, CB_GETCURSEL, 0, 0);
    if (idxPlay >= 0 && idxPlay < (int)kHotkeyCount) {
        g_settings.hotkeyPlay = kAvailableHotkeys[idxPlay].vkCode;
    }

    int idxStop = (int)SendMessage(g_hComboHkStop, CB_GETCURSEL, 0, 0);
    if (idxStop >= 0 && idxStop < (int)kHotkeyCount) {
        g_settings.hotkeyStop = kAvailableHotkeys[idxStop].vkCode;
    }

    // Update HookManager ignored keys
    g_hookManager.setIgnoredKeys({g_settings.hotkeyRecord, g_settings.hotkeyPlay, g_settings.hotkeyStop});
}

// Update GUI settings controls from g_settings
static void SyncSettingsToUI() {
    std::wstring loopStr = std::to_wstring(g_settings.loopCount);
    SetWindowTextW(g_hEditLoop, loopStr.c_str());

    std::wstring delayStr = std::to_wstring(g_settings.loopDelayMs);
    SetWindowTextW(g_hEditDelay, delayStr.c_str());

    int sel = 1;
    if (g_settings.speedMultiplier == 0.5) sel = 0;
    else if (g_settings.speedMultiplier == 1.0) sel = 1;
    else if (g_settings.speedMultiplier == 1.5) sel = 2;
    else if (g_settings.speedMultiplier == 2.0) sel = 3;
    else if (g_settings.speedMultiplier == 3.0) sel = 4;
    else if (g_settings.speedMultiplier == 5.0) sel = 5;
    else if (g_settings.speedMultiplier == 10.0) sel = 6;
    SendMessage(g_hComboSpeed, CB_SETCURSEL, sel, 0);

    SendMessage(g_hChkMouseMove, BM_SETCHECK, g_settings.recordMouseMoves ? BST_CHECKED : BST_UNCHECKED, 0);

    // Set hotkey comboboxes
    SendMessage(g_hComboHkRecord, CB_SETCURSEL, FindHotkeyIndex(g_settings.hotkeyRecord), 0);
    SendMessage(g_hComboHkPlay, CB_SETCURSEL, FindHotkeyIndex(g_settings.hotkeyPlay), 0);
    SendMessage(g_hComboHkStop, CB_SETCURSEL, FindHotkeyIndex(g_settings.hotkeyStop), 0);

    g_hookManager.setIgnoredKeys({g_settings.hotkeyRecord, g_settings.hotkeyPlay, g_settings.hotkeyStop});
}

// Add an event to the ListView
static void AddEventToListView(size_t index, const MacroK::MacroEvent& e) {
    LVITEMW lvi{};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = static_cast<int>(index);
    lvi.iSubItem = 0;

    std::wstring idxStr = std::to_wstring(index + 1);
    lvi.pszText = &idxStr[0];
    ListView_InsertItem(g_hListEvents, &lvi);

    // Event type
    std::wstring typeStr = StringToWString(MacroK::MacroStorage::eventTypeToString(e.type));
    ListView_SetItemText(g_hListEvents, lvi.iItem, 1, &typeStr[0]);

    // Delay
    std::wstring delayStr = std::to_wstring(e.delayMs) + L" ms";
    ListView_SetItemText(g_hListEvents, lvi.iItem, 2, &delayStr[0]);

    // Details
    std::wstring detail;
    if (e.type == MacroK::EventType::MouseMove) {
        detail = L"X: " + std::to_wstring(e.x) + L", Y: " + std::to_wstring(e.y);
    } else if (e.type == MacroK::EventType::MouseDown || e.type == MacroK::EventType::MouseUp) {
        detail = StringToWString(MacroK::MacroStorage::mouseButtonToString(e.button)) + L" (" + std::to_wstring(e.x) + L", " + std::to_wstring(e.y) + L")";
    } else if (e.type == MacroK::EventType::KeyDown || e.type == MacroK::EventType::KeyUp) {
        detail = L"Phím: " + StringToWString(MacroK::MacroStorage::getKeyName(e.vkCode, e.scanCode, e.isExtendedKey)) + L" (VK: " + std::to_wstring(e.vkCode) + L")";
    } else if (e.type == MacroK::EventType::MouseWheel) {
        detail = L"Cuộn dọc: " + std::to_wstring(e.wheelDelta);
    } else if (e.type == MacroK::EventType::MouseHWheel) {
        detail = L"Cuộn ngang: " + std::to_wstring(e.wheelDelta);
    }
    ListView_SetItemText(g_hListEvents, lvi.iItem, 3, &detail[0]);

    // Timestamp
    std::wstring timeStr = std::to_wstring(e.timestampMs) + L" ms";
    ListView_SetItemText(g_hListEvents, lvi.iItem, 4, &timeStr[0]);

    // Auto-scroll to latest item
    ListView_EnsureVisible(g_hListEvents, lvi.iItem, FALSE);
}

// Reload the entire ListView from recorded events
static void RefreshListView() {
    SendMessage(g_hListEvents, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_hListEvents);

    const auto& events = g_hookManager.getRecordedEvents();
    for (size_t i = 0; i < events.size(); ++i) {
        AddEventToListView(i, events[i]);
    }

    SendMessage(g_hListEvents, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hListEvents, nullptr, TRUE);
}

// UI State Updates
static void UpdateUIState() {
    bool recording = g_hookManager.isRecording();
    bool playing = g_player.isPlaying();
    size_t eventCount = g_hookManager.getRecordedEvents().size();

    std::wstring hkRecName = GetHotkeyName(g_settings.hotkeyRecord);
    std::wstring hkPlayName = GetHotkeyName(g_settings.hotkeyPlay);
    std::wstring hkStopName = GetHotkeyName(g_settings.hotkeyStop);

    // Update button text with current hotkeys
    SetWindowTextW(g_hBtnStop, (L"⏹ Dừng (" + hkStopName + L")").c_str());

    if (recording) {
        SetWindowTextW(g_hBtnRecord, (L"■ DỪNG GHI (" + hkRecName + L")").c_str());
        SetWindowTextW(g_hBtnPlay, (L"▶ Phát Macro (" + hkPlayName + L")").c_str());
        EnableWindow(g_hBtnPlay, FALSE);
        EnableWindow(g_hBtnOpen, FALSE);
        EnableWindow(g_hBtnSave, FALSE);
        EnableWindow(g_hBtnClear, FALSE);

        std::wstring stat = L"[🔴 ĐANG GHI MACRO...] Hãy thao tác chuột & phím | Bấm " + hkRecName + L" hoặc " + hkStopName + L" để dừng";
        SetWindowTextW(g_hStatusLabel, stat.c_str());

        std::wstring prog = L"Đã bắt: " + std::to_wstring(eventCount) + L" sự kiện (Đã lọc rung chuột đứng im)";
        SetWindowTextW(g_hProgressLabel, prog.c_str());
    } else if (playing) {
        SetWindowTextW(g_hBtnRecord, (L"● Bắt đầu Ghi (" + hkRecName + L")").c_str());
        SetWindowTextW(g_hBtnPlay, (L"■ DỪNG PHÁT (" + hkPlayName + L" / " + hkStopName + L")").c_str());
        EnableWindow(g_hBtnRecord, FALSE);
        EnableWindow(g_hBtnOpen, FALSE);
        EnableWindow(g_hBtnSave, FALSE);
        EnableWindow(g_hBtnClear, FALSE);

        std::wstring stat;
        if (g_settings.loopCount == 0) {
            stat = L"[▶️ ĐANG PHÁT LẠI...] Lặp vô hạn (0) - Bấm " + hkStopName + L" hoặc " + hkPlayName + L" để dừng!";
        } else {
            stat = L"[▶️ ĐANG PHÁT LẠI...] Số vòng lặp: " + std::to_wstring(g_settings.loopCount) + L" - Bấm " + hkStopName + L" để dừng!";
        }
        SetWindowTextW(g_hStatusLabel, stat.c_str());
    } else {
        SetWindowTextW(g_hBtnRecord, (L"● Bắt đầu Ghi (" + hkRecName + L")").c_str());
        SetWindowTextW(g_hBtnPlay, (L"▶ Phát Macro (" + hkPlayName + L")").c_str());
        EnableWindow(g_hBtnRecord, TRUE);
        EnableWindow(g_hBtnPlay, (eventCount > 0) ? TRUE : FALSE);
        EnableWindow(g_hBtnOpen, TRUE);
        EnableWindow(g_hBtnSave, (eventCount > 0) ? TRUE : FALSE);
        EnableWindow(g_hBtnClear, (eventCount > 0) ? TRUE : FALSE);

        std::wstring stat = L"[SẴN SÀNG] Tổng số sự kiện trong bộ nhớ: " + std::to_wstring(eventCount);
        SetWindowTextW(g_hStatusLabel, stat.c_str());

        std::wstring prog;
        if (g_settings.loopCount == 0) {
            prog = L"Cài đặt: Lặp VÔ HẠN (cho tới khi bấm " + hkStopName + L") | Tốc độ: " + std::to_wstring(g_settings.speedMultiplier).substr(0, 3) + L"x";
        } else {
            prog = L"Cài đặt: Lặp " + std::to_wstring(g_settings.loopCount) + L" lần | Tốc độ: " + std::to_wstring(g_settings.speedMultiplier).substr(0, 3) + L"x";
        }
        SetWindowTextW(g_hProgressLabel, prog.c_str());
    }

    // Update footer hint bar
    std::wstring footer = L"💡 Phím tắt toàn cục: [" + hkRecName + L"] Ghi/Dừng  |  [" + hkPlayName + L"] Phát/Dừng  |  [" + hkStopName + L"] Dừng khẩn cấp (Hủy lặp vô hạn)";
    SetWindowTextW(g_hFooterHint, footer.c_str());
}

// Action: Toggle Recording
static void OnToggleRecord() {
    if (g_player.isPlaying()) return;

    if (!g_hookManager.isRecording()) {
        SyncSettingsFromUI();
        ListView_DeleteAllItems(g_hListEvents);
        g_hookManager.startRecording(g_settings);
    } else {
        g_hookManager.stopRecording();
        RefreshListView();
    }
    UpdateUIState();
}

// Action: Toggle Playback
static void OnTogglePlay() {
    if (g_hookManager.isRecording()) return;

    if (g_player.isPlaying()) {
        g_player.stop();
        UpdateUIState();
    } else {
        const auto& events = g_hookManager.getRecordedEvents();
        if (events.empty()) {
            MessageBoxW(g_hWnd, L"Chưa có sự kiện nào trong bộ nhớ để phát!\nHãy bấm 'Bắt đầu Ghi' hoặc mở file macro.", L"Thông báo", MB_OK | MB_ICONINFORMATION);
            return;
        }

        SyncSettingsFromUI();
        UpdateUIState();
        g_player.play(events, g_settings);
    }
}

// Action: Emergency Stop (Stop key / ESC)
static void OnEmergencyStop() {
    bool stoppedSomething = false;
    if (g_hookManager.isRecording()) {
        g_hookManager.stopRecording();
        RefreshListView();
        stoppedSomething = true;
    }
    if (g_player.isPlaying()) {
        g_player.stop();
        stoppedSomething = true;
    }
    if (stoppedSomething) {
        UpdateUIState();
    }
}

// Action: Open Macro File
static void OnOpenFile() {
    fs::create_directories("macros");

    wchar_t filename[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hWnd;
    ofn.lpstrFilter = L"Macro Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = L"macros";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::string filePath = WStringToString(filename);
        std::vector<MacroK::MacroEvent> loadedEvents;
        MacroK::MacroSettings loadedSettings;

        if (MacroK::MacroStorage::loadFromFile(filePath, loadedEvents, loadedSettings)) {
            g_hookManager.setRecordedEvents(loadedEvents);
            g_settings = loadedSettings;
            SyncSettingsToUI();
            RefreshListView();
            UpdateUIState();
            MessageBoxW(g_hWnd, (L"Đã nạp thành công: " + std::to_wstring(loadedEvents.size()) + L" sự kiện!").c_str(), L"Thành công", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(g_hWnd, L"Không thể đọc file hoặc định dạng file không hợp lệ!", L"Lỗi", MB_OK | MB_ICONERROR);
        }
    }
}

// Action: Save Macro File
static void OnSaveFile() {
    const auto& events = g_hookManager.getRecordedEvents();
    if (events.empty()) {
        MessageBoxW(g_hWnd, L"Không có sự kiện nào để lưu!", L"Thông báo", MB_OK | MB_ICONWARNING);
        return;
    }

    fs::create_directories("macros");

    wchar_t filename[MAX_PATH] = L"my_macro.json";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hWnd;
    ofn.lpstrFilter = L"Macro Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = L"macros";
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (GetSaveFileNameW(&ofn)) {
        SyncSettingsFromUI();
        std::string filePath = WStringToString(filename);
        if (MacroK::MacroStorage::saveToFile(filePath, events, g_settings)) {
            MessageBoxW(g_hWnd, L"Đã lưu file macro thành công!", L"Thành công", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(g_hWnd, L"Không thể ghi file!", L"Lỗi", MB_OK | MB_ICONERROR);
        }
    }
}

// Action: Clear All
static void OnClearEvents() {
    if (g_hookManager.isRecording() || g_player.isPlaying()) return;

    if (g_hookManager.getRecordedEvents().empty()) return;

    if (MessageBoxW(g_hWnd, L"Bạn có chắc muốn xóa toàn bộ sự kiện đang có trong danh sách?", L"Xác nhận", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        g_hookManager.clearEvents();
        RefreshListView();
        UpdateUIState();
    }
}

// Main Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hWnd = hWnd;

            // Create modern Segoe UI fonts
            g_hFontNormal = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            g_hFontBold = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            g_hFontStatus = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            // --- Toolbar Buttons ---
            g_hBtnRecord = CreateWindowW(L"BUTTON", L"● Bắt đầu Ghi (F8)",
                                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                        15, 12, 160, 36, hWnd, (HMENU)IDC_BTN_RECORD, nullptr, nullptr);

            g_hBtnPlay = CreateWindowW(L"BUTTON", L"▶ Phát Macro (F9)",
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      185, 12, 160, 36, hWnd, (HMENU)IDC_BTN_PLAY, nullptr, nullptr);

            g_hBtnStop = CreateWindowW(L"BUTTON", L"⏹ Dừng (ESC)",
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      355, 12, 125, 36, hWnd, (HMENU)IDC_BTN_STOP, nullptr, nullptr);

            g_hBtnOpen = CreateWindowW(L"BUTTON", L"📂 Mở File...",
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      490, 12, 95, 36, hWnd, (HMENU)IDC_BTN_OPEN, nullptr, nullptr);

            g_hBtnSave = CreateWindowW(L"BUTTON", L"💾 Lưu File...",
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      595, 12, 95, 36, hWnd, (HMENU)IDC_BTN_SAVE, nullptr, nullptr);

            g_hBtnClear = CreateWindowW(L"BUTTON", L"🗑️ Xóa",
                                       WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                       700, 12, 65, 36, hWnd, (HMENU)IDC_BTN_CLEAR, nullptr, nullptr);

            // --- Panel 1: Settings Group Box ---
            HWND hGroupSettings = CreateWindowW(L"BUTTON", L" Cấu hình Phát lại & Vòng lặp ",
                                               WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                               15, 55, 750, 72, hWnd, nullptr, nullptr, nullptr);
            SetControlFont(hGroupSettings, g_hFontBold);

            // Loop Count
            HWND hLblLoop = CreateWindowW(L"STATIC", L"Số vòng lặp (0 = Vô hạn):",
                                         WS_CHILD | WS_VISIBLE | SS_LEFT,
                                         30, 76, 165, 20, hWnd, nullptr, nullptr, nullptr);
            g_hEditLoop = CreateWindowW(L"EDIT", L"0",
                                       WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
                                       200, 74, 55, 22, hWnd, (HMENU)IDC_EDIT_LOOP, nullptr, nullptr);

            HWND hLblLoopHint = CreateWindowW(L"STATIC", L"(0 = lặp vô hạn cho tới khi bấm phím Dừng)",
                                             WS_CHILD | WS_VISIBLE | SS_LEFT,
                                             30, 100, 240, 18, hWnd, nullptr, nullptr, nullptr);

            // Speed
            HWND hLblSpeed = CreateWindowW(L"STATIC", L"Tốc độ:",
                                          WS_CHILD | WS_VISIBLE | SS_LEFT,
                                          275, 76, 50, 20, hWnd, nullptr, nullptr, nullptr);
            g_hComboSpeed = CreateWindowW(L"COMBOBOX", L"",
                                         WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                         330, 73, 105, 180, hWnd, (HMENU)IDC_COMBO_SPEED, nullptr, nullptr);
            SendMessageW(g_hComboSpeed, CB_ADDSTRING, 0, (LPARAM)L"0.5x (Chậm)");
            SendMessageW(g_hComboSpeed, CB_ADDSTRING, 0, (LPARAM)L"1.0x (Chuẩn)");
            SendMessageW(g_hComboSpeed, CB_ADDSTRING, 0, (LPARAM)L"1.5x");
            SendMessageW(g_hComboSpeed, CB_ADDSTRING, 0, (LPARAM)L"2.0x (Nhanh)");
            SendMessageW(g_hComboSpeed, CB_ADDSTRING, 0, (LPARAM)L"3.0x");
            SendMessageW(g_hComboSpeed, CB_ADDSTRING, 0, (LPARAM)L"5.0x");
            SendMessageW(g_hComboSpeed, CB_ADDSTRING, 0, (LPARAM)L"10.0x");
            SendMessageW(g_hComboSpeed, CB_SETCURSEL, 1, 0);

            // Loop delay
            HWND hLblDelay = CreateWindowW(L"STATIC", L"Nghỉ giữa vòng:",
                                          WS_CHILD | WS_VISIBLE | SS_LEFT,
                                          275, 100, 95, 20, hWnd, nullptr, nullptr, nullptr);
            g_hEditDelay = CreateWindowW(L"EDIT", L"500",
                                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
                                        375, 98, 50, 22, hWnd, (HMENU)IDC_EDIT_DELAY, nullptr, nullptr);
            HWND hLblMs = CreateWindowW(L"STATIC", L"ms",
                                       WS_CHILD | WS_VISIBLE | SS_LEFT,
                                       430, 100, 25, 20, hWnd, nullptr, nullptr, nullptr);

            // Mouse moves checkbox
            g_hChkMouseMove = CreateWindowW(L"BUTTON", L"Ghi chuyển động chuột (Tự lọc khi đứng im)",
                                           WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                           470, 75, 285, 22, hWnd, (HMENU)IDC_CHK_MOUSE_MOVE, nullptr, nullptr);
            SendMessage(g_hChkMouseMove, BM_SETCHECK, BST_CHECKED, 0);

            // --- Panel 2: Custom Hotkeys Group Box ---
            HWND hGroupHotkeys = CreateWindowW(L"BUTTON", L" Tùy chỉnh Phím tắt Toàn cục (Global Hotkeys) ",
                                              WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                              15, 132, 750, 62, hWnd, nullptr, nullptr, nullptr);
            SetControlFont(hGroupHotkeys, g_hFontBold);

            // Hotkey Record
            HWND hLblHkRec = CreateWindowW(L"STATIC", L"Phím Ghi:",
                                          WS_CHILD | WS_VISIBLE | SS_LEFT,
                                          30, 157, 65, 20, hWnd, nullptr, nullptr, nullptr);
            g_hComboHkRecord = CreateWindowW(L"COMBOBOX", L"",
                                            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                            100, 154, 110, 200, hWnd, (HMENU)IDC_COMBO_HK_RECORD, nullptr, nullptr);

            // Hotkey Play
            HWND hLblHkPlay = CreateWindowW(L"STATIC", L"Phím Phát:",
                                           WS_CHILD | WS_VISIBLE | SS_LEFT,
                                           235, 157, 70, 20, hWnd, nullptr, nullptr, nullptr);
            g_hComboHkPlay = CreateWindowW(L"COMBOBOX", L"",
                                          WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                          310, 154, 110, 200, hWnd, (HMENU)IDC_COMBO_HK_PLAY, nullptr, nullptr);

            // Hotkey Stop
            HWND hLblHkStop = CreateWindowW(L"STATIC", L"Phím Dừng:",
                                           WS_CHILD | WS_VISIBLE | SS_LEFT,
                                           445, 157, 75, 20, hWnd, nullptr, nullptr, nullptr);
            g_hComboHkStop = CreateWindowW(L"COMBOBOX", L"",
                                          WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                          525, 154, 110, 200, hWnd, (HMENU)IDC_COMBO_HK_STOP, nullptr, nullptr);

            // Populate Hotkey ComboBoxes
            for (size_t i = 0; i < kHotkeyCount; ++i) {
                SendMessageW(g_hComboHkRecord, CB_ADDSTRING, 0, (LPARAM)kAvailableHotkeys[i].name);
                SendMessageW(g_hComboHkPlay, CB_ADDSTRING, 0, (LPARAM)kAvailableHotkeys[i].name);
                SendMessageW(g_hComboHkStop, CB_ADDSTRING, 0, (LPARAM)kAvailableHotkeys[i].name);
            }

            // Defaults: F8, F9, ESC
            SendMessage(g_hComboHkRecord, CB_SETCURSEL, FindHotkeyIndex(VK_F8), 0);
            SendMessage(g_hComboHkPlay, CB_SETCURSEL, FindHotkeyIndex(VK_F9), 0);
            SendMessage(g_hComboHkStop, CB_SETCURSEL, FindHotkeyIndex(VK_ESCAPE), 0);

            // --- Status Banner ---
            g_hStatusLabel = CreateWindowW(L"STATIC", L"[SẴN SÀNG] Tổng số sự kiện trong bộ nhớ: 0",
                                          WS_CHILD | WS_VISIBLE | SS_LEFT,
                                          15, 200, 750, 22, hWnd, (HMENU)IDC_STATUS_LABEL, nullptr, nullptr);

            g_hProgressLabel = CreateWindowW(L"STATIC", L"Cài đặt: Lặp VÔ HẠN (0) | Tốc độ: 1.0x",
                                            WS_CHILD | WS_VISIBLE | SS_LEFT,
                                            15, 224, 750, 20, hWnd, (HMENU)IDC_PROGRESS_LABEL, nullptr, nullptr);

            // --- Event ListView (Table) ---
            g_hListEvents = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                                           WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_VSCROLL,
                                           15, 248, 750, 320, hWnd, (HMENU)IDC_LIST_EVENTS, nullptr, nullptr);

            ListView_SetExtendedListViewStyle(g_hListEvents, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

            // Add columns to ListView
            LVCOLUMNW lvc{};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

            lvc.pszText = (LPWSTR)L"#";
            lvc.cx = 45;
            ListView_InsertColumn(g_hListEvents, 0, &lvc);

            lvc.pszText = (LPWSTR)L"Loại sự kiện";
            lvc.cx = 125;
            ListView_InsertColumn(g_hListEvents, 1, &lvc);

            lvc.pszText = (LPWSTR)L"Độ trễ";
            lvc.cx = 85;
            ListView_InsertColumn(g_hListEvents, 2, &lvc);

            lvc.pszText = (LPWSTR)L"Chi tiết (Tọa độ / Phím bấm)";
            lvc.cx = 340;
            ListView_InsertColumn(g_hListEvents, 3, &lvc);

            lvc.pszText = (LPWSTR)L"Thời điểm";
            lvc.cx = 135;
            ListView_InsertColumn(g_hListEvents, 4, &lvc);

            // --- Footer Hint Bar ---
            g_hFooterHint = CreateWindowW(L"STATIC",
                                         L"💡 Phím tắt toàn cục: [F8] Ghi/Dừng  |  [F9] Phát/Dừng  |  [ESC] Dừng khẩn cấp (Hủy lặp vô hạn)",
                                         WS_CHILD | WS_VISIBLE | SS_LEFT,
                                         15, 576, 750, 20, hWnd, (HMENU)IDC_FOOTER_HINT, nullptr, nullptr);

            // Apply fonts to all controls
            SetControlFont(g_hBtnRecord, g_hFontBold);
            SetControlFont(g_hBtnPlay, g_hFontBold);
            SetControlFont(g_hBtnStop, g_hFontBold);
            SetControlFont(g_hBtnOpen, g_hFontNormal);
            SetControlFont(g_hBtnSave, g_hFontNormal);
            SetControlFont(g_hBtnClear, g_hFontNormal);

            SetControlFont(hLblLoop, g_hFontNormal);
            SetControlFont(g_hEditLoop, g_hFontBold);
            SetControlFont(hLblLoopHint, g_hFontNormal);
            SetControlFont(hLblSpeed, g_hFontNormal);
            SetControlFont(g_hComboSpeed, g_hFontNormal);
            SetControlFont(hLblDelay, g_hFontNormal);
            SetControlFont(g_hEditDelay, g_hFontNormal);
            SetControlFont(hLblMs, g_hFontNormal);
            SetControlFont(g_hChkMouseMove, g_hFontNormal);

            SetControlFont(hLblHkRec, g_hFontNormal);
            SetControlFont(g_hComboHkRecord, g_hFontNormal);
            SetControlFont(hLblHkPlay, g_hFontNormal);
            SetControlFont(g_hComboHkPlay, g_hFontNormal);
            SetControlFont(hLblHkStop, g_hFontNormal);
            SetControlFont(g_hComboHkStop, g_hFontNormal);

            SetControlFont(g_hStatusLabel, g_hFontStatus);
            SetControlFont(g_hProgressLabel, g_hFontNormal);
            SetControlFont(g_hListEvents, g_hFontNormal);
            SetControlFont(g_hFooterHint, g_hFontNormal);

            SyncSettingsFromUI();
            UpdateUIState();
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == IDC_BTN_RECORD && code == BN_CLICKED) {
                OnToggleRecord();
            } else if (id == IDC_BTN_PLAY && code == BN_CLICKED) {
                OnTogglePlay();
            } else if (id == IDC_BTN_STOP && code == BN_CLICKED) {
                OnEmergencyStop();
            } else if (id == IDC_BTN_OPEN && code == BN_CLICKED) {
                OnOpenFile();
            } else if (id == IDC_BTN_SAVE && code == BN_CLICKED) {
                OnSaveFile();
            } else if (id == IDC_BTN_CLEAR && code == BN_CLICKED) {
                OnClearEvents();
            } else if ((id == IDC_EDIT_LOOP || id == IDC_COMBO_SPEED || id == IDC_EDIT_DELAY || id == IDC_CHK_MOUSE_MOVE ||
                        id == IDC_COMBO_HK_RECORD || id == IDC_COMBO_HK_PLAY || id == IDC_COMBO_HK_STOP) &&
                       (code == EN_CHANGE || code == CBN_SELCHANGE || code == BN_CLICKED)) {
                SyncSettingsFromUI();
                UpdateUIState();
            }
            return 0;
        }

        case WM_KEYDOWN: {
            uint32_t vk = static_cast<uint32_t>(wParam);
            if (vk == g_settings.hotkeyStop) {
                OnEmergencyStop();
                return 0;
            } else if (vk == g_settings.hotkeyRecord) {
                OnToggleRecord();
                return 0;
            } else if (vk == g_settings.hotkeyPlay) {
                OnTogglePlay();
                return 0;
            }
            break;
        }

        // Custom Message: Global Hotkey Pressed
        case WM_APP_HOTKEY: {
            uint32_t vkCode = static_cast<uint32_t>(wParam);
            if (vkCode == g_settings.hotkeyRecord) {
                OnToggleRecord();
            } else if (vkCode == g_settings.hotkeyPlay) {
                OnTogglePlay();
            } else if (vkCode == g_settings.hotkeyStop) {
                OnEmergencyStop();
            }
            return 0;
        }

        // Custom Message: New Event Recorded in Hook Thread
        case WM_APP_EVENT_ADDED: {
            const auto& events = g_hookManager.getRecordedEvents();
            if (!events.empty()) {
                AddEventToListView(events.size() - 1, events.back());
                std::wstring prog = L"Đã bắt: " + std::to_wstring(events.size()) + L" sự kiện";
                SetWindowTextW(g_hProgressLabel, prog.c_str());
            }
            return 0;
        }

        // Custom Message: Playback Progress
        case WM_APP_PLAY_PROGRESS: {
            size_t step = LOWORD(wParam);
            size_t total = HIWORD(wParam);
            uint32_t currentLoop = static_cast<uint32_t>(lParam);

            std::wstring stat;
            std::wstring hkStopName = GetHotkeyName(g_settings.hotkeyStop);
            if (g_settings.loopCount == 0) {
                stat = L"[▶️ ĐANG PHÁT LẠI...] Vòng lặp: " + std::to_wstring(currentLoop) + L"/Vô hạn  -  Bước: " + std::to_wstring(step) + L"/" + std::to_wstring(total) + L" (Bấm " + hkStopName + L" để dừng)";
            } else {
                stat = L"[▶️ ĐANG PHÁT LẠI...] Vòng lặp: " + std::to_wstring(currentLoop) + L"/" + std::to_wstring(g_settings.loopCount) + L"  -  Bước: " + std::to_wstring(step) + L"/" + std::to_wstring(total);
            }
            SetWindowTextW(g_hStatusLabel, stat.c_str());
            return 0;
        }

        // Custom Message: Playback Finished
        case WM_APP_PLAY_FINISHED: {
            UpdateUIState();
            return 0;
        }

        case WM_DESTROY: {
            g_player.stop();
            g_hookManager.shutdown();

            if (g_hFontNormal) DeleteObject(g_hFontNormal);
            if (g_hFontBold) DeleteObject(g_hFontBold);
            if (g_hFontStatus) DeleteObject(g_hFontStatus);

            PostQuitMessage(0);
            return 0;
        }

        default:
            break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Initialize Windows Common Controls
    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Register Window Class
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"MacroRecorderKWindowClass";

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Không thể đăng ký Window Class!", L"Lỗi", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Window size
    const int winWidth = 795;
    const int winHeight = 650;

    // Center window on screen
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winWidth) / 2;
    int posY = (screenH - winHeight) / 2;

    HWND hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Macro Recorder K - (Win32 High-Precision Edition)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winWidth, winHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) {
        MessageBoxW(nullptr, L"Không thể tạo cửa sổ giao diện!", L"Lỗi", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Callback when any global hotkey is pressed
    g_hookManager.setHotkeyCallback([hWnd](uint32_t vkCode) {
        PostMessage(hWnd, WM_APP_HOTKEY, static_cast<WPARAM>(vkCode), 0);
    });

    // Callback when an event is recorded
    g_hookManager.setEventCallback([hWnd](const MacroK::MacroEvent&) {
        PostMessage(hWnd, WM_APP_EVENT_ADDED, 0, 0);
    });

    // Configure MacroPlayer
    g_player.setProgressCallback([hWnd](size_t cur, size_t tot, uint32_t loop, uint32_t) {
        PostMessage(hWnd, WM_APP_PLAY_PROGRESS, MAKEWPARAM(cur, tot), loop);
    });

    g_player.setFinishedCallback([hWnd](bool ok) {
        PostMessage(hWnd, WM_APP_PLAY_FINISHED, ok ? 1 : 0, 0);
    });

    // Initialize hooks
    if (!g_hookManager.init()) {
        MessageBoxW(hWnd, L"Cảnh báo: Không thể khởi tạo Low-Level Hook!", L"Lỗi", MB_OK | MB_ICONWARNING);
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Main Message Loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
