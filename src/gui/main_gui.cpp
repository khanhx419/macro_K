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
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <limits>

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
#define IDC_BTN_DELETE_SEL    1007
#define IDC_LIST_EVENTS       1008
#define IDC_EDIT_LOOP         1009
#define IDC_COMBO_SPEED       1010
#define IDC_EDIT_DELAY        1011
#define IDC_CHK_KEYBOARD      1012
#define IDC_CHK_MOUSE         1013
#define IDC_CHK_MOUSE_MOVE    1014
#define IDC_STATUS_LABEL      1015
#define IDC_PROGRESS_LABEL    1016
#define IDC_COMBO_HK_RECORD   1017
#define IDC_COMBO_HK_PLAY     1018
#define IDC_COMBO_HK_STOP     1019
#define IDC_FOOTER_HINT       1020
#define IDC_AUTO_TOGGLE       1021
#define IDC_AUTO_BUTTON       1022
#define IDC_AUTO_INTERVAL     1023
#define IDC_AUTO_COUNT        1024
#define IDC_VIEW_TABS         1025
#define IDM_SETTINGS          1026
#define IDC_SETTINGS_DONE     1027
static constexpr UINT_PTR kAutoTimer = 1;
static constexpr int kAutoHotkey = 1;
static HWND g_autoToggle, g_autoButton, g_autoInterval, g_autoCount;
static bool g_autoRunning = false;
static uint64_t g_autoClicks = 0;
static uint32_t g_autoLimit = 0;
static DWORD g_autoDown = 0, g_autoUp = 0;

// Global handles
static HWND g_hWnd = nullptr;
static HWND g_hBtnRecord = nullptr;
static HWND g_hBtnPlay = nullptr;
static HWND g_hBtnStop = nullptr;
static HWND g_hBtnOpen = nullptr;
static HWND g_hBtnSave = nullptr;
static HWND g_hBtnDeleteSel = nullptr;
static HWND g_hBtnClear = nullptr;
static HWND g_hListEvents = nullptr;
static HWND g_hEditLoop = nullptr;
static HWND g_hComboSpeed = nullptr;
static HWND g_hEditDelay = nullptr;
static HWND g_hChkKeyboard = nullptr;
static HWND g_hChkMouse = nullptr;
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

static HWND g_tabs = nullptr;
static int g_view = 0, g_previousView = 0, g_buildPage = -1;
static UINT g_dpi = 96;
static int Scale(int value) { return MulDiv(value, g_dpi, 96); }
struct ControlLayout {
    HWND handle;
    int x, y, width, height;
    bool bold;
    int page;
};
static std::vector<ControlLayout> g_layout;

static void UpdateFontsAndLayout() {
    HFONT normal = CreateFontW(-MulDiv(10, g_dpi, 72), 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Tahoma");
    HFONT bold = CreateFontW(-MulDiv(10, g_dpi, 72), 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Tahoma");
    for (const auto& item : g_layout) {
        SetWindowPos(item.handle, nullptr, Scale(item.x), Scale(item.y),
            Scale(item.width), Scale(item.height), SWP_NOZORDER | SWP_NOACTIVATE);
        SendMessage(item.handle, WM_SETFONT, reinterpret_cast<WPARAM>(item.bold ? bold : normal), TRUE);
    }
    const int widths[] = {32, 95, 65, 216, 90};
    if (g_hListEvents) {
        for (int i = 0; i < 5; ++i) ListView_SetColumnWidth(g_hListEvents, i, Scale(widths[i]));
    }
    if (g_hFontNormal) DeleteObject(g_hFontNormal);
    if (g_hFontBold) DeleteObject(g_hFontBold);
    if (g_hFontStatus) DeleteObject(g_hFontStatus);
    g_hFontNormal = normal;
    g_hFontBold = bold;
    g_hFontStatus = nullptr;
}

// Each view exposes only the controls needed for the current task.
static void ShowView(int view) {
    if (view < 0 || view > 2) return;
    if (view == 2 && (g_autoRunning || g_player.isPlaying() || g_hookManager.isRecording())) return;
    g_view = view;
    TabCtrl_SetCurSel(g_tabs, view == 2 ? -1 : view);
    for (auto& item : g_layout) {
        bool visible = item.page == -1 || item.page == view;
        if (item.handle == g_hStatusLabel || item.handle == g_hProgressLabel) {
            visible = view != 2;
            item.y = (view == 0 ? 190 : 352) + (item.handle == g_hProgressLabel ? 24 : 0);
        }
        if (item.handle == g_hBtnStop) {
            visible = view != 2;
            item.x = view == 0 ? 378 : 364;
            item.y = view == 0 ? 140 : 52;
        }
        ShowWindow(item.handle, visible ? SW_SHOW : SW_HIDE);
        SetWindowPos(item.handle, nullptr, Scale(item.x), Scale(item.y),
            Scale(item.width), Scale(item.height), SWP_NOZORDER | SWP_NOACTIVATE);
    }
    RECT frame{0, 0, Scale(540), Scale(view == 0 ? 246 : 408)};
    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(g_hWnd, GWL_STYLE));
    using AdjustForDpi = BOOL (WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
    auto adjust = reinterpret_cast<AdjustForDpi>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi"));
    if (adjust) adjust(&frame, style, TRUE, 0, g_dpi);
    else AdjustWindowRectEx(&frame, style, TRUE, 0);
    SetWindowPos(g_hWnd, nullptr, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(g_hWnd, nullptr, TRUE);
}

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

// Auto click sends each press/release together at the current cursor position.
static void StopAutoClick() {
    KillTimer(g_hWnd, kAutoTimer);
    g_autoRunning = false;
    UpdateUIState();
}

static bool ReadAutoNumber(HWND control, uint32_t minimum, uint32_t& value) {
    wchar_t text[64]{};
    GetWindowTextW(control, text, 64);
    std::wstring input(text);
    if (input.empty() || input.find_first_not_of(L"0123456789") != std::wstring::npos) return false;
    try {
        auto number = std::stoull(input);
        if (number < minimum || number > (std::numeric_limits<uint32_t>::max)()) return false;
        value = static_cast<uint32_t>(number);
        return true;
    } catch (...) { return false; }
}

static void ToggleAutoClick() {
    if (g_autoRunning) { StopAutoClick(); return; }
    if (g_hookManager.isRecording() || g_player.isPlaying() || g_autoRunning) return;
    uint32_t interval;
    if (!ReadAutoNumber(g_autoInterval, 10, interval) || interval > USER_TIMER_MAXIMUM ||
        !ReadAutoNumber(g_autoCount, 0, g_autoLimit)) {
        MessageBoxW(g_hWnd, L"Khoảng nghỉ phải từ 10 đến 2147483647 ms. Số click từ 0 đến 4294967295 (0 = vô hạn).",
                    L"Cài đặt Auto Click không hợp lệ", MB_OK | MB_ICONWARNING);
        return;
    }
    int button = static_cast<int>(SendMessage(g_autoButton, CB_GETCURSEL, 0, 0));
    g_autoDown = button == 1 ? MOUSEEVENTF_RIGHTDOWN : button == 2 ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_LEFTDOWN;
    g_autoUp = button == 1 ? MOUSEEVENTF_RIGHTUP : button == 2 ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_LEFTUP;
    g_autoClicks = 0;
    // Give the user time to move away from the Start button.
    if (!SetTimer(g_hWnd, kAutoTimer, 1000, nullptr)) {
        MessageBoxW(g_hWnd, L"Không thể khởi tạo Auto Click.", L"Lỗi", MB_OK | MB_ICONERROR);
        return;
    }
    ShowView(0);
    g_autoRunning = true;
    UpdateUIState();
}

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

    // Recording source checkboxes
    g_settings.recordKeyboard = (SendMessage(g_hChkKeyboard, BM_GETCHECK, 0, 0) == BST_CHECKED);
    g_settings.recordMouse = (SendMessage(g_hChkMouse, BM_GETCHECK, 0, 0) == BST_CHECKED);
    g_settings.recordMouseMoves = g_settings.recordMouse && (SendMessage(g_hChkMouseMove, BM_GETCHECK, 0, 0) == BST_CHECKED);

    // If mouse is unchecked, disable mouse moves checkbox visually
    EnableWindow(g_hChkMouseMove, g_settings.recordMouse ? TRUE : FALSE);

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

    // Immediately synchronize settings and ignored keys with HookManager
    g_hookManager.setSettings(g_settings);
    g_hookManager.setIgnoredKeys({g_settings.hotkeyRecord, g_settings.hotkeyPlay, g_settings.hotkeyStop});
}

// Update GUI settings controls from g_settings
static void SyncSettingsToUI() {
    // Older macro files may have assigned the now-reserved Auto Click key.
    if (g_settings.hotkeyRecord == VK_F6) g_settings.hotkeyRecord = VK_F8;
    if (g_settings.hotkeyPlay == VK_F6) g_settings.hotkeyPlay = VK_F9;
    if (g_settings.hotkeyStop == VK_F6) g_settings.hotkeyStop = VK_ESCAPE;
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

    SendMessage(g_hChkKeyboard, BM_SETCHECK, g_settings.recordKeyboard ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(g_hChkMouse, BM_SETCHECK, g_settings.recordMouse ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(g_hChkMouseMove, BM_SETCHECK, g_settings.recordMouseMoves ? BST_CHECKED : BST_UNCHECKED, 0);
    EnableWindow(g_hChkMouseMove, g_settings.recordMouse ? TRUE : FALSE);

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
    bool playing = g_player.isPlaying() || g_autoRunning;
    size_t eventCount = g_hookManager.getRecordedEvents().size();

    std::wstring hkRecName = GetHotkeyName(g_settings.hotkeyRecord);
    std::wstring hkPlayName = GetHotkeyName(g_settings.hotkeyPlay);
    std::wstring hkStopName = GetHotkeyName(g_settings.hotkeyStop);

    // Update button text with current hotkeys
    SetWindowTextW(g_hBtnStop, (L"Dừng · " + hkStopName).c_str());

    if (recording) {
        SetWindowTextW(g_hBtnRecord, (L"Dừng ghi · " + hkRecName).c_str());
        SetWindowTextW(g_hBtnPlay, (L"Phát · " + hkPlayName).c_str());
        EnableWindow(g_hBtnPlay, FALSE);
        EnableWindow(g_hBtnOpen, FALSE);
        EnableWindow(g_hBtnSave, FALSE);
        EnableWindow(g_hBtnDeleteSel, FALSE);
        EnableWindow(g_hBtnClear, FALSE);

        std::wstring stat = L"Đang ghi macro  ·  Nhấn " + hkRecName + L" hoặc " + hkStopName + L" để dừng";
        SetWindowTextW(g_hStatusLabel, stat.c_str());

        std::wstring prog = L"Đã ghi " + std::to_wstring(eventCount) + L" sự kiện  ·  Tự lọc rung chuột";
        SetWindowTextW(g_hProgressLabel, prog.c_str());
    } else if (playing) {
        SetWindowTextW(g_hBtnRecord, (L"Ghi · " + hkRecName).c_str());
        SetWindowTextW(g_hBtnPlay, (L"Dừng phát · " + hkPlayName).c_str());
        EnableWindow(g_hBtnRecord, FALSE);
        EnableWindow(g_hBtnOpen, FALSE);
        EnableWindow(g_hBtnSave, FALSE);
        EnableWindow(g_hBtnDeleteSel, FALSE);
        EnableWindow(g_hBtnClear, FALSE);

        std::wstring stat;
        if (g_settings.loopCount == 0) {
            stat = L"Đang phát  ·  Lặp vô hạn  ·  Nhấn " + hkStopName + L" hoặc " + hkPlayName + L" để dừng";
        } else {
            stat = L"Đang phát  ·  Số vòng: " + std::to_wstring(g_settings.loopCount) + L" - Bấm " + hkStopName + L" để dừng!";
        }
        SetWindowTextW(g_hStatusLabel, stat.c_str());
    } else {
        SetWindowTextW(g_hBtnRecord, (L"Ghi · " + hkRecName).c_str());
        SetWindowTextW(g_hBtnPlay, (L"Phát · " + hkPlayName).c_str());
        EnableWindow(g_hBtnRecord, TRUE);
        EnableWindow(g_hBtnPlay, (eventCount > 0) ? TRUE : FALSE);
        EnableWindow(g_hBtnOpen, TRUE);
        EnableWindow(g_hBtnSave, (eventCount > 0) ? TRUE : FALSE);
        EnableWindow(g_hBtnDeleteSel, (eventCount > 0) ? TRUE : FALSE);
        EnableWindow(g_hBtnClear, (eventCount > 0) ? TRUE : FALSE);

        std::wstring stat = L"Sẵn sàng  ·  Sự kiện: " + std::to_wstring(eventCount);
        SetWindowTextW(g_hStatusLabel, stat.c_str());

        std::wstring prog;
        if (g_settings.loopCount == 0) {
            prog = L"Lặp vô hạn  ·  Dừng: " + hkStopName + L"  ·  Tốc độ: " + std::to_wstring(g_settings.speedMultiplier).substr(0, 3) + L"x";
        } else {
            prog = L"Lặp " + std::to_wstring(g_settings.loopCount) + L" lần  ·  Tốc độ: " + std::to_wstring(g_settings.speedMultiplier).substr(0, 3) + L"x";
        }
        SetWindowTextW(g_hProgressLabel, prog.c_str());
    }

    EnableWindow(g_autoToggle, !recording && !g_player.isPlaying());
    SetWindowTextW(g_autoToggle, g_autoRunning ? L"Dừng · F6" : L"Bắt đầu · F6");
    for (HWND control : {g_autoButton, g_autoInterval, g_autoCount}) EnableWindow(control, !g_autoRunning);
    if (g_autoRunning) {
        SetWindowTextW(g_hStatusLabel, L"Auto Click đang chạy  ·  Nhấn F6 hoặc phím Dừng để kết thúc");
        std::wstring progress = L"Đã click: " + std::to_wstring(g_autoClicks) +
            (g_autoLimit ? L" / " + std::to_wstring(g_autoLimit) : L" / Vô hạn");
        SetWindowTextW(g_hProgressLabel, progress.c_str());
    }

    HMENU menu = GetMenu(g_hWnd);
    bool idle = !recording && !playing;
    EnableMenuItem(menu, IDC_BTN_OPEN, MF_BYCOMMAND | (idle ? MF_ENABLED : MF_GRAYED));
    for (UINT id : {IDC_BTN_SAVE, IDC_BTN_DELETE_SEL, IDC_BTN_CLEAR})
        EnableMenuItem(menu, id, MF_BYCOMMAND | (idle && eventCount ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(menu, IDM_SETTINGS, MF_BYCOMMAND | (idle ? MF_ENABLED : MF_GRAYED));
    if (g_view == 0 && !recording && !g_player.isPlaying()) {
        SetWindowTextW(g_hStatusLabel, g_autoRunning ? L"Đang click · F6 để dừng" : L"Sẵn sàng · F6 để bắt đầu");
        std::wstring count = L"Đã click: " + std::to_wstring(g_autoClicks);
        if (g_autoLimit) count += L" / " + std::to_wstring(g_autoLimit);
        SetWindowTextW(g_hProgressLabel, count.c_str());
    }

    // Update footer hint bar
    std::wstring footer = L"F6  Auto Click bật/tắt    ·    " + hkRecName + L"  Ghi    ·    " + hkPlayName + L"  Phát    ·    " + hkStopName + L"  Dừng    ·    Del  Xóa chọn";
    SetWindowTextW(g_hFooterHint, footer.c_str());
}

// Action: Toggle Recording
static void OnToggleRecord() {
    if (g_player.isPlaying() || g_autoRunning) return;

    if (!g_hookManager.isRecording()) {
        ShowView(1);
        SyncSettingsFromUI();
        ListView_DeleteAllItems(g_hListEvents);
        g_hookManager.startRecording(g_settings);
    } else {
        g_hookManager.stopRecording();
        const auto& evts = g_hookManager.getRecordedEvents();
        if (!evts.empty()) {
            const auto& lastEvt = evts.back();
            if (lastEvt.type == MacroK::EventType::MouseDown || lastEvt.type == MacroK::EventType::MouseUp) {
                RECT rcBtnRec{}, rcBtnStop{};
                GetWindowRect(g_hBtnRecord, &rcBtnRec);
                GetWindowRect(g_hBtnStop, &rcBtnStop);
                bool inBtn = (lastEvt.x >= rcBtnRec.left && lastEvt.x <= rcBtnRec.right &&
                              lastEvt.y >= rcBtnRec.top && lastEvt.y <= rcBtnRec.bottom) ||
                             (lastEvt.x >= rcBtnStop.left && lastEvt.x <= rcBtnStop.right &&
                              lastEvt.y >= rcBtnStop.top && lastEvt.y <= rcBtnStop.bottom);
                if (inBtn) {
                    g_hookManager.removeEvent(evts.size() - 1);
                }
            }
        }
        RefreshListView();
    }
    UpdateUIState();
}

// Action: Toggle Playback
static void OnTogglePlay() {
    if (g_hookManager.isRecording() || g_autoRunning) return;

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
        ShowView(1);
        g_player.play(events, g_settings);
        UpdateUIState();
    }
}

// Action: Emergency Stop (Stop key / ESC)
static void OnEmergencyStop() {
    bool stoppedSomething = false;
    if (g_autoRunning) { StopAutoClick(); stoppedSomething = true; }
    if (g_hookManager.isRecording()) {
        g_hookManager.stopRecording();
        const auto& evts = g_hookManager.getRecordedEvents();
        if (!evts.empty()) {
            const auto& lastEvt = evts.back();
            if (lastEvt.type == MacroK::EventType::MouseDown || lastEvt.type == MacroK::EventType::MouseUp) {
                RECT rcBtnStop{};
                GetWindowRect(g_hBtnStop, &rcBtnStop);
                if (lastEvt.x >= rcBtnStop.left && lastEvt.x <= rcBtnStop.right &&
                    lastEvt.y >= rcBtnStop.top && lastEvt.y <= rcBtnStop.bottom) {
                    g_hookManager.removeEvent(evts.size() - 1);
                }
            }
        }
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

// Action: Delete Selected Event(s)
static void OnDeleteSelectedEvents() {
    if (g_hookManager.isRecording() || g_player.isPlaying() || g_autoRunning) return;

    std::vector<size_t> selectedIndices;
    int item = -1;
    while ((item = ListView_GetNextItem(g_hListEvents, item, LVNI_SELECTED)) != -1) {
        selectedIndices.push_back(static_cast<size_t>(item));
    }

    if (selectedIndices.empty()) {
        MessageBoxW(g_hWnd, L"Vui lòng nhấp chọn một hoặc nhiều hành động trong bảng để xóa!", L"Thông báo", MB_OK | MB_ICONINFORMATION);
        return;
    }

    int nextSel = static_cast<int>(selectedIndices.front());

    g_hookManager.removeEvents(selectedIndices);
    RefreshListView();

    // Re-select nearest item
    int count = ListView_GetItemCount(g_hListEvents);
    if (count > 0) {
        if (nextSel >= count) nextSel = count - 1;
        ListView_SetItemState(g_hListEvents, nextSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(g_hListEvents, nextSel, FALSE);
    }

    UpdateUIState();
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
    if (g_hookManager.isRecording() || g_player.isPlaying() || g_autoRunning) return;

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
            using WindowDpi = UINT (WINAPI*)(HWND);
            auto windowDpi = reinterpret_cast<WindowDpi>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
            if (windowDpi) g_dpi = windowDpi(hWnd);

            auto control = [&](const wchar_t* cls, const wchar_t* text, DWORD style,
                               int x, int y, int w, int h, int id = 0, bool bold = false) {
                HWND result = CreateWindowW(cls, text, WS_CHILD | WS_VISIBLE | style,
                    Scale(x), Scale(y), Scale(w), Scale(h), hWnd, (HMENU)(INT_PTR)id, nullptr, nullptr);
                g_layout.push_back({result, x, y, w, h, bold, g_buildPage});
                return result;
            };
            auto label = [&](const wchar_t* text, int x, int y, int w, bool bold = false) {
                return control(L"STATIC", text, SS_LEFT, x, y, w, 22, 0, bold);
            };
            auto button = [&](const wchar_t* text, int x, int y, int w, int id) {
                return control(L"BUTTON", text, WS_TABSTOP | BS_PUSHBUTTON, x, y, w, 32, id);
            };
            auto edit = [&](const wchar_t* text, int x, int y, int w, int id) {
                return control(L"EDIT", text, WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER,
                    x, y, w, 24, id);
            };
            auto combo = [&](int x, int y, int w, int id) {
                return control(L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                    x, y, w, 240, id);
            };

            HMENU menu = CreateMenu(), files = CreatePopupMenu();
            AppendMenuW(files, MF_STRING, IDC_BTN_OPEN, L"Mở macro...");
            AppendMenuW(files, MF_STRING, IDC_BTN_SAVE, L"Lưu macro...");
            AppendMenuW(files, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(files, MF_STRING, IDC_BTN_DELETE_SEL, L"Xóa sự kiện đã chọn\tDel");
            AppendMenuW(files, MF_STRING, IDC_BTN_CLEAR, L"Xóa hết sự kiện");
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(files), L"Tệp");
            AppendMenuW(menu, MF_STRING, IDM_SETTINGS, L"Cài đặt");
            SetMenu(hWnd, menu);

            g_buildPage = -1;
            g_tabs = control(WC_TABCONTROLW, L"", WS_TABSTOP | TCS_BUTTONS | TCS_FIXEDWIDTH,
                16, 12, 508, 28, IDC_VIEW_TABS);
            for (auto title : {L"Auto Click", L"Macro"}) {
                TCITEMW item{}; item.mask = TCIF_TEXT; item.pszText = const_cast<LPWSTR>(title);
                TabCtrl_InsertItem(g_tabs, TabCtrl_GetItemCount(g_tabs), &item);
            }
            g_hBtnStop = button(L"Dừng", 378, 140, 146, IDC_BTN_STOP);
            g_hStatusLabel = label(L"Sẵn sàng", 16, 190, 508);
            g_hProgressLabel = label(L"", 16, 214, 508);

            g_buildPage = 0;
            label(L"Nút chuột", 16, 58, 110);
            label(L"Khoảng nghỉ (ms)", 172, 58, 155);
            label(L"Số click (0 = vô hạn)", 348, 58, 176);
            g_autoButton = combo(16, 84, 132, IDC_AUTO_BUTTON);
            for (auto text : {L"Trái", L"Phải", L"Giữa"}) SendMessageW(g_autoButton, CB_ADDSTRING, 0, (LPARAM)text);
            SendMessage(g_autoButton, CB_SETCURSEL, 0, 0);
            g_autoInterval = edit(L"100", 172, 84, 152, IDC_AUTO_INTERVAL);
            g_autoCount = edit(L"0", 348, 84, 176, IDC_AUTO_COUNT);
            g_autoToggle = button(L"Bắt đầu · F6", 16, 140, 180, IDC_AUTO_TOGGLE);
            label(L"Click theo con trỏ, bắt đầu sau 1 giây.", 16, 115, 508);

            g_buildPage = 1;
            g_hBtnRecord = button(L"Ghi · F8", 16, 52, 158, IDC_BTN_RECORD);
            g_hBtnPlay = button(L"Phát · F9", 190, 52, 158, IDC_BTN_PLAY);
            g_hListEvents = control(WC_LISTVIEWW, L"", WS_TABSTOP | LVS_REPORT | WS_VSCROLL,
                16, 100, 508, 238, IDC_LIST_EVENTS);
            ListView_SetExtendedListViewStyle(g_hListEvents, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            ListView_SetBkColor(g_hListEvents, GetSysColor(COLOR_WINDOW));
            ListView_SetTextBkColor(g_hListEvents, GetSysColor(COLOR_WINDOW));
            ListView_SetTextColor(g_hListEvents, GetSysColor(COLOR_WINDOWTEXT));
            const wchar_t* columns[] = {L"#", L"Sự kiện", L"Trễ (ms)", L"Tọa độ / Phím", L"Thời điểm"};
            const int widths[] = {32, 95, 65, 216, 90};
            for (int i = 0; i < 5; ++i) {
                LVCOLUMNW column{}; column.mask = LVCF_TEXT | LVCF_WIDTH;
                column.pszText = const_cast<LPWSTR>(columns[i]); column.cx = Scale(widths[i]);
                ListView_InsertColumn(g_hListEvents, i, &column);
            }

            g_buildPage = 2;
            label(L"Cài đặt macro", 16, 56, 300, true);
            label(L"Số vòng (0 = vô hạn)", 16, 92, 195);
            g_hEditLoop = edit(L"0", 230, 88, 110, IDC_EDIT_LOOP);
            label(L"Tốc độ phát", 16, 126, 195);
            g_hComboSpeed = combo(230, 122, 110, IDC_COMBO_SPEED);
            for (auto text : {L"0.5x", L"1.0x", L"1.5x", L"2.0x", L"3.0x", L"5.0x", L"10.0x"})
                SendMessageW(g_hComboSpeed, CB_ADDSTRING, 0, (LPARAM)text);
            SendMessage(g_hComboSpeed, CB_SETCURSEL, 1, 0);
            label(L"Nghỉ giữa vòng (ms)", 16, 160, 195);
            g_hEditDelay = edit(L"500", 230, 156, 110, IDC_EDIT_DELAY);
            label(L"Ghi lại", 16, 198, 90);
            g_hChkKeyboard = control(L"BUTTON", L"Bàn phím", BS_AUTOCHECKBOX | WS_TABSTOP, 112, 194, 110, 24, IDC_CHK_KEYBOARD);
            g_hChkMouse = control(L"BUTTON", L"Click, cuộn", BS_AUTOCHECKBOX | WS_TABSTOP, 234, 194, 120, 24, IDC_CHK_MOUSE);
            g_hChkMouseMove = control(L"BUTTON", L"Di chuyển", BS_AUTOCHECKBOX | WS_TABSTOP, 368, 194, 145, 24, IDC_CHK_MOUSE_MOVE);
            for (HWND item : {g_hChkKeyboard, g_hChkMouse, g_hChkMouseMove}) SendMessage(item, BM_SETCHECK, BST_CHECKED, 0);
            label(L"Phím tắt", 16, 238, 180, true);
            label(L"Ghi", 16, 270, 45);
            g_hComboHkRecord = combo(65, 266, 100, IDC_COMBO_HK_RECORD);
            label(L"Phát", 188, 270, 45);
            g_hComboHkPlay = combo(237, 266, 100, IDC_COMBO_HK_PLAY);
            label(L"Dừng", 360, 270, 45);
            g_hComboHkStop = combo(409, 266, 115, IDC_COMBO_HK_STOP);
            for (HWND item : {g_hComboHkRecord, g_hComboHkPlay, g_hComboHkStop})
                for (const auto& key : kAvailableHotkeys) SendMessageW(item, CB_ADDSTRING, 0, (LPARAM)key.name);
            SendMessage(g_hComboHkRecord, CB_SETCURSEL, FindHotkeyIndex(VK_F8), 0);
            SendMessage(g_hComboHkPlay, CB_SETCURSEL, FindHotkeyIndex(VK_F9), 0);
            SendMessage(g_hComboHkStop, CB_SETCURSEL, FindHotkeyIndex(VK_ESCAPE), 0);
            label(L"F6 dành riêng cho Auto Click. Thay đổi áp dụng ngay.", 16, 308, 508);
            button(L"Xong", 404, 354, 120, IDC_SETTINGS_DONE);

            UpdateFontsAndLayout();
            SyncSettingsFromUI();
            ShowView(0);
            UpdateUIState();
            return 0;
        }

        case WM_DPICHANGED: {
            g_dpi = HIWORD(wParam);
            UpdateFontsAndLayout();
            const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(hWnd, nullptr, suggested->left, suggested->top,
                suggested->right - suggested->left, suggested->bottom - suggested->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            ShowView(g_view);
            return 0;
        }

        case WM_HOTKEY:
            if (wParam == kAutoHotkey) ToggleAutoClick();
            return 0;

        case WM_TIMER:
            if (wParam == kAutoTimer && g_autoRunning) {
                if (g_autoClicks == 0) {
                    uint32_t interval = 100;
                    ReadAutoNumber(g_autoInterval, 10, interval);
                    if (!SetTimer(hWnd, kAutoTimer, interval, nullptr)) { StopAutoClick(); return 0; }
                }
                INPUT inputs[2]{};
                inputs[0].type = inputs[1].type = INPUT_MOUSE;
                inputs[0].mi.dwFlags = g_autoDown;
                inputs[1].mi.dwFlags = g_autoUp;
                UINT sent = SendInput(2, inputs, sizeof(INPUT));
                if (sent != 2) {
                    if (sent == 1) SendInput(1, &inputs[1], sizeof(INPUT));
                    StopAutoClick();
                    MessageBoxW(hWnd, L"Không thể gửi click đến ứng dụng đích.", L"Auto Click", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                ++g_autoClicks;
                if (g_autoLimit && g_autoClicks >= g_autoLimit) StopAutoClick();
                else UpdateUIState();
            }
            return 0;

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == IDM_SETTINGS) {
                if (g_view != 2) g_previousView = g_view;
                ShowView(2);
                UpdateUIState();
            } else if (id == IDC_SETTINGS_DONE) {
                ShowView(g_previousView);
                UpdateUIState();
            } else if (id == IDC_AUTO_TOGGLE && code == BN_CLICKED) {
                ToggleAutoClick();
            } else if (id == IDC_BTN_RECORD && code == BN_CLICKED) {
                OnToggleRecord();
            } else if (id == IDC_BTN_PLAY && code == BN_CLICKED) {
                OnTogglePlay();
            } else if (id == IDC_BTN_STOP && code == BN_CLICKED) {
                OnEmergencyStop();
            } else if (id == IDC_BTN_OPEN && code == BN_CLICKED) {
                OnOpenFile();
            } else if (id == IDC_BTN_SAVE && code == BN_CLICKED) {
                OnSaveFile();
            } else if (id == IDC_BTN_DELETE_SEL && code == BN_CLICKED) {
                OnDeleteSelectedEvents();
            } else if (id == IDC_BTN_CLEAR && code == BN_CLICKED) {
                OnClearEvents();
            } else if ((id == IDC_EDIT_LOOP || id == IDC_COMBO_SPEED || id == IDC_EDIT_DELAY ||
                        id == IDC_CHK_KEYBOARD || id == IDC_CHK_MOUSE || id == IDC_CHK_MOUSE_MOVE ||
                        id == IDC_COMBO_HK_RECORD || id == IDC_COMBO_HK_PLAY || id == IDC_COMBO_HK_STOP) &&
                       (code == EN_CHANGE || code == CBN_SELCHANGE || code == BN_CLICKED)) {
                SyncSettingsFromUI();
                UpdateUIState();
            }
            return 0;
        }

        case WM_NOTIFY: {
            LPNMHDR pnm = reinterpret_cast<LPNMHDR>(lParam);
            if (pnm && pnm->idFrom == IDC_VIEW_TABS && pnm->code == TCN_SELCHANGE) {
                ShowView(TabCtrl_GetCurSel(g_tabs));
                UpdateUIState();
                return 0;
            }
            if (pnm && pnm->idFrom == IDC_LIST_EVENTS) {
                if (pnm->code == LVN_KEYDOWN) {
                    LPNMLVKEYDOWN pnkd = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
                    if (pnkd->wVKey == VK_DELETE) {
                        OnDeleteSelectedEvents();
                        return 0;
                    }
                }
            }
            break;
        }

        case WM_KEYDOWN: {
            uint32_t vk = static_cast<uint32_t>(wParam);
            if (vk == VK_DELETE) {
                OnDeleteSelectedEvents();
                return 0;
            } else if (vk == g_settings.hotkeyStop) {
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
                stat = L"Đang phát  ·  Vòng: " + std::to_wstring(currentLoop) + L"/Vô hạn  -  Bước: " + std::to_wstring(step) + L"/" + std::to_wstring(total) + L" (Bấm " + hkStopName + L" để dừng)";
            } else {
                stat = L"Đang phát  ·  Vòng: " + std::to_wstring(currentLoop) + L"/" + std::to_wstring(g_settings.loopCount) + L"  -  Bước: " + std::to_wstring(step) + L"/" + std::to_wstring(total);
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
            KillTimer(hWnd, kAutoTimer);
            UnregisterHotKey(hWnd, kAutoHotkey);
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
    // Opt into native DPI rendering even for builds without an embedded manifest.
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    using SetDpiContext = BOOL (WINAPI*)(HANDLE);
    auto setDpiContext = reinterpret_cast<SetDpiContext>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (setDpiContext) setDpiContext(reinterpret_cast<HANDLE>(-4));
    else SetProcessDPIAware();
    HDC screenDC = GetDC(nullptr);
    if (screenDC) {
        g_dpi = static_cast<UINT>(GetDeviceCaps(screenDC, LOGPIXELSX));
        ReleaseDC(nullptr, screenDC);
    }

    // Initialize Windows Common Controls
    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    // Register Window Class
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = L"MacroRecorderKWindowClass";

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Không thể đăng ký Window Class!", L"Lỗi", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Window size
    RECT frame{0, 0, Scale(540), Scale(246)};
    constexpr DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    using AdjustForDpi = BOOL (WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
    auto adjustForDpi = reinterpret_cast<AdjustForDpi>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
    if (adjustForDpi) adjustForDpi(&frame, windowStyle, TRUE, 0, g_dpi);
    else AdjustWindowRectEx(&frame, windowStyle, TRUE, 0);

    // Center window on screen
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - (frame.right - frame.left)) / 2;
    int posY = (screenH - (frame.bottom - frame.top)) / 2;

    HWND hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Macro K",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        posX, posY, frame.right - frame.left, frame.bottom - frame.top,
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

    g_player.setFinishedCallback([hWnd](bool) {
        PostMessage(hWnd, WM_APP_PLAY_FINISHED, 0, 0);
    });

    // Initialize hooks
    if (!g_hookManager.init()) {
        MessageBoxW(hWnd, L"Cảnh báo: Không thể khởi tạo Low-Level Hook!", L"Lỗi", MB_OK | MB_ICONWARNING);
    }

    if (!RegisterHotKey(hWnd, kAutoHotkey, MOD_NOREPEAT, VK_F6)) {
        MessageBoxW(hWnd, L"F6 đang được ứng dụng khác sử dụng. Bạn vẫn có thể bật/tắt Auto Click bằng nút trên giao diện.",
                    L"Phím tắt Auto Click", MB_OK | MB_ICONWARNING);
    }
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Main Message Loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return (int)msg.wParam;
}
