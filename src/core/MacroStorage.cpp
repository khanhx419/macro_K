#include "MacroStorage.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <algorithm>

namespace MacroK {

std::string MacroStorage::getKeyName(uint32_t vkCode, uint32_t scanCode, bool isExtended) {
    LONG lParam = static_cast<LONG>(scanCode << 16);
    if (isExtended) {
        lParam |= (1 << 24);
    }

    char name[128] = {0};
    int len = GetKeyNameTextA(lParam, name, sizeof(name));
    if (len > 0) {
        return std::string(name);
    }

    // Fallbacks for known keys if GetKeyNameText fails
    if (vkCode >= 'A' && vkCode <= 'Z') return std::string(1, static_cast<char>(vkCode));
    if (vkCode >= '0' && vkCode <= '9') return std::string(1, static_cast<char>(vkCode));
    if (vkCode >= VK_F1 && vkCode <= VK_F24) return "F" + std::to_string(vkCode - VK_F1 + 1);

    switch (vkCode) {
        case VK_LBUTTON: return "Left Click";
        case VK_RBUTTON: return "Right Click";
        case VK_MBUTTON: return "Middle Click";
        case VK_SPACE: return "Space";
        case VK_RETURN: return "Enter";
        case VK_BACK: return "Backspace";
        case VK_TAB: return "Tab";
        case VK_SHIFT:
        case VK_LSHIFT: return "Left Shift";
        case VK_RSHIFT: return "Right Shift";
        case VK_CONTROL:
        case VK_LCONTROL: return "Left Ctrl";
        case VK_RCONTROL: return "Right Ctrl";
        case VK_MENU:
        case VK_LMENU: return "Left Alt";
        case VK_RMENU: return "Right Alt";
        case VK_ESCAPE: return "Esc";
        case VK_CAPITAL: return "Caps Lock";
        case VK_UP: return "Up Arrow";
        case VK_DOWN: return "Down Arrow";
        case VK_LEFT: return "Left Arrow";
        case VK_RIGHT: return "Right Arrow";
        default: return "VK_" + std::to_string(vkCode);
    }
}

std::string MacroStorage::eventTypeToString(EventType type) {
    switch (type) {
        case EventType::MouseMove: return "MouseMove";
        case EventType::MouseDown: return "MouseDown";
        case EventType::MouseUp: return "MouseUp";
        case EventType::MouseWheel: return "MouseWheel";
        case EventType::MouseHWheel: return "MouseHWheel";
        case EventType::KeyDown: return "KeyDown";
        case EventType::KeyUp: return "KeyUp";
        default: return "Unknown";
    }
}

EventType MacroStorage::stringToEventType(const std::string& str) {
    if (str == "MouseMove") return EventType::MouseMove;
    if (str == "MouseDown") return EventType::MouseDown;
    if (str == "MouseUp") return EventType::MouseUp;
    if (str == "MouseWheel") return EventType::MouseWheel;
    if (str == "MouseHWheel") return EventType::MouseHWheel;
    if (str == "KeyDown") return EventType::KeyDown;
    if (str == "KeyUp") return EventType::KeyUp;
    return EventType::MouseMove;
}

std::string MacroStorage::mouseButtonToString(MouseButton btn) {
    switch (btn) {
        case MouseButton::Left: return "Left";
        case MouseButton::Right: return "Right";
        case MouseButton::Middle: return "Middle";
        case MouseButton::XButton1: return "XButton1";
        case MouseButton::XButton2: return "XButton2";
        default: return "None";
    }
}

MouseButton MacroStorage::stringToMouseButton(const std::string& str) {
    if (str == "Left") return MouseButton::Left;
    if (str == "Right") return MouseButton::Right;
    if (str == "Middle") return MouseButton::Middle;
    if (str == "XButton1") return MouseButton::XButton1;
    if (str == "XButton2") return MouseButton::XButton2;
    return MouseButton::None;
}

bool MacroStorage::saveToFile(const std::string& filePath,
                             const std::vector<MacroEvent>& events,
                             const MacroSettings& settings) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        return false;
    }

    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"settings\": {\n";
    out << "    \"speedMultiplier\": " << std::fixed << std::setprecision(2) << settings.speedMultiplier << ",\n";
    out << "    \"loopCount\": " << settings.loopCount << ",\n";
    out << "    \"loopDelayMs\": " << settings.loopDelayMs << ",\n";
    out << "    \"recordMouseMoves\": " << (settings.recordMouseMoves ? "true" : "false") << ",\n";
    out << "    \"mouseMoveIntervalMs\": " << settings.mouseMoveIntervalMs << ",\n";
    out << "    \"mouseMoveThresholdPx\": " << settings.mouseMoveThresholdPx << ",\n";
    out << "    \"hotkeyRecord\": " << settings.hotkeyRecord << ",\n";
    out << "    \"hotkeyPlay\": " << settings.hotkeyPlay << ",\n";
    out << "    \"hotkeyStop\": " << settings.hotkeyStop << "\n";
    out << "  },\n";
    out << "  \"events\": [\n";

    for (size_t i = 0; i < events.size(); ++i) {
        const auto& e = events[i];
        out << "    {\n";
        out << "      \"type\": \"" << eventTypeToString(e.type) << "\",\n";
        out << "      \"delayMs\": " << e.delayMs << ",\n";
        out << "      \"timestampMs\": " << e.timestampMs;

        if (e.type == EventType::MouseMove || e.type == EventType::MouseDown || e.type == EventType::MouseUp) {
            out << ",\n      \"x\": " << e.x << ",\n";
            out << "      \"y\": " << e.y;
            if (e.type != EventType::MouseMove) {
                out << ",\n      \"button\": \"" << mouseButtonToString(e.button) << "\"";
            }
        } else if (e.type == EventType::MouseWheel || e.type == EventType::MouseHWheel) {
            out << ",\n      \"wheelDelta\": " << e.wheelDelta;
        } else if (e.type == EventType::KeyDown || e.type == EventType::KeyUp) {
            out << ",\n      \"vkCode\": " << e.vkCode << ",\n";
            out << "      \"scanCode\": " << e.scanCode << ",\n";
            out << "      \"isExtendedKey\": " << (e.isExtendedKey ? "true" : "false") << ",\n";
            out << "      \"keyName\": \"" << getKeyName(e.vkCode, e.scanCode, e.isExtendedKey) << "\"";
        }

        out << "\n    }" << (i + 1 < events.size() ? "," : "") << "\n";
    }

    out << "  ]\n";
    out << "}\n";
    return true;
}

// Simple and robust parser for our JSON structure
namespace {
    std::string extractStringValue(const std::string& block, const std::string& key) {
        std::string pattern = "\"" + key + "\"";
        size_t pos = block.find(pattern);
        if (pos == std::string::npos) return "";
        pos = block.find(':', pos);
        if (pos == std::string::npos) return "";
        pos = block.find('\"', pos);
        if (pos == std::string::npos) return "";
        size_t endPos = block.find('\"', pos + 1);
        if (endPos == std::string::npos) return "";
        return block.substr(pos + 1, endPos - pos - 1);
    }

    double extractDoubleValue(const std::string& block, const std::string& key, double defaultVal = 0.0) {
        std::string pattern = "\"" + key + "\"";
        size_t pos = block.find(pattern);
        if (pos == std::string::npos) return defaultVal;
        pos = block.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        size_t start = pos + 1;
        while (start < block.size() && (block[start] == ' ' || block[start] == '\t' || block[start] == '\r' || block[start] == '\n')) {
            start++;
        }
        size_t end = start;
        while (end < block.size() && (std::isdigit(block[end]) || block[end] == '.' || block[end] == '-')) {
            end++;
        }
        if (start == end) return defaultVal;
        try {
            return std::stod(block.substr(start, end - start));
        } catch (...) {
            return defaultVal;
        }
    }

    int64_t extractInt64Value(const std::string& block, const std::string& key, int64_t defaultVal = 0) {
        std::string pattern = "\"" + key + "\"";
        size_t pos = block.find(pattern);
        if (pos == std::string::npos) return defaultVal;
        pos = block.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        size_t start = pos + 1;
        while (start < block.size() && (block[start] == ' ' || block[start] == '\t' || block[start] == '\r' || block[start] == '\n')) {
            start++;
        }
        size_t end = start;
        while (end < block.size() && (std::isdigit(block[end]) || block[end] == '-')) {
            end++;
        }
        if (start == end) return defaultVal;
        try {
            return std::stoll(block.substr(start, end - start));
        } catch (...) {
            return defaultVal;
        }
    }

    bool extractBoolValue(const std::string& block, const std::string& key, bool defaultVal = false) {
        std::string pattern = "\"" + key + "\"";
        size_t pos = block.find(pattern);
        if (pos == std::string::npos) return defaultVal;
        pos = block.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        size_t truePos = block.find("true", pos);
        size_t falsePos = block.find("false", pos);
        size_t commaPos = block.find(',', pos);
        size_t bracePos = block.find('}', pos);
        size_t bound = std::min(commaPos, bracePos);

        if (truePos != std::string::npos && truePos < bound) return true;
        if (falsePos != std::string::npos && falsePos < bound) return false;
        return defaultVal;
    }
}

bool MacroStorage::loadFromFile(const std::string& filePath,
                               std::vector<MacroEvent>& outEvents,
                               MacroSettings& outSettings) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();

    // Parse settings
    size_t settingsPos = content.find("\"settings\"");
    if (settingsPos != std::string::npos) {
        size_t openBrace = content.find('{', settingsPos);
        size_t closeBrace = content.find('}', openBrace);
        if (openBrace != std::string::npos && closeBrace != std::string::npos) {
            std::string setBlock = content.substr(openBrace, closeBrace - openBrace + 1);
            outSettings.speedMultiplier = extractDoubleValue(setBlock, "speedMultiplier", 1.0);
            outSettings.loopCount = static_cast<uint32_t>(extractInt64Value(setBlock, "loopCount", 0));
            outSettings.loopDelayMs = static_cast<uint32_t>(extractInt64Value(setBlock, "loopDelayMs", 500));
            outSettings.recordMouseMoves = extractBoolValue(setBlock, "recordMouseMoves", true);
            outSettings.mouseMoveIntervalMs = static_cast<uint32_t>(extractInt64Value(setBlock, "mouseMoveIntervalMs", 15));
            outSettings.mouseMoveThresholdPx = static_cast<int32_t>(extractInt64Value(setBlock, "mouseMoveThresholdPx", 3));
            outSettings.hotkeyRecord = static_cast<uint32_t>(extractInt64Value(setBlock, "hotkeyRecord", VK_F8));
            outSettings.hotkeyPlay = static_cast<uint32_t>(extractInt64Value(setBlock, "hotkeyPlay", VK_F9));
            outSettings.hotkeyStop = static_cast<uint32_t>(extractInt64Value(setBlock, "hotkeyStop", VK_ESCAPE));
        }
    }

    // Parse events array
    size_t eventsPos = content.find("\"events\"");
    if (eventsPos == std::string::npos) {
        return false;
    }

    size_t arrayStart = content.find('[', eventsPos);
    size_t arrayEnd = content.rfind(']');
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
        return false;
    }

    outEvents.clear();
    size_t cur = arrayStart + 1;
    while (cur < arrayEnd) {
        size_t objStart = content.find('{', cur);
        if (objStart == std::string::npos || objStart >= arrayEnd) break;
        size_t objEnd = content.find('}', objStart);
        if (objEnd == std::string::npos || objEnd >= arrayEnd) break;

        std::string objBlock = content.substr(objStart, objEnd - objStart + 1);
        std::string typeStr = extractStringValue(objBlock, "type");

        MacroEvent evt{};
        evt.type = stringToEventType(typeStr);
        evt.delayMs = static_cast<uint32_t>(extractInt64Value(objBlock, "delayMs", 0));
        evt.timestampMs = static_cast<uint64_t>(extractInt64Value(objBlock, "timestampMs", 0));

        if (evt.type == EventType::MouseMove || evt.type == EventType::MouseDown || evt.type == EventType::MouseUp) {
            evt.x = static_cast<int32_t>(extractInt64Value(objBlock, "x", 0));
            evt.y = static_cast<int32_t>(extractInt64Value(objBlock, "y", 0));
            if (evt.type != EventType::MouseMove) {
                evt.button = stringToMouseButton(extractStringValue(objBlock, "button"));
            }
        } else if (evt.type == EventType::MouseWheel || evt.type == EventType::MouseHWheel) {
            evt.wheelDelta = static_cast<int32_t>(extractInt64Value(objBlock, "wheelDelta", 0));
        } else if (evt.type == EventType::KeyDown || evt.type == EventType::KeyUp) {
            evt.vkCode = static_cast<uint32_t>(extractInt64Value(objBlock, "vkCode", 0));
            evt.scanCode = static_cast<uint32_t>(extractInt64Value(objBlock, "scanCode", 0));
            evt.isExtendedKey = extractBoolValue(objBlock, "isExtendedKey", false);
        }

        outEvents.push_back(evt);
        cur = objEnd + 1;
    }

    return true;
}

} // namespace MacroK
