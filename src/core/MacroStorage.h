#pragma once

#include "Types.h"
#include <string>
#include <vector>

namespace MacroK {

class MacroStorage {
public:
    static bool saveToFile(const std::string& filePath,
                           const std::vector<MacroEvent>& events,
                           const MacroSettings& settings = MacroSettings{});

    static bool loadFromFile(const std::string& filePath,
                             std::vector<MacroEvent>& outEvents,
                             MacroSettings& outSettings);

    static std::string getKeyName(uint32_t vkCode, uint32_t scanCode, bool isExtended);
    static std::string eventTypeToString(EventType type);
    static EventType stringToEventType(const std::string& str);
    static std::string mouseButtonToString(MouseButton btn);
    static MouseButton stringToMouseButton(const std::string& str);
};

} // namespace MacroK
