/**
 * core/MacroEngine.cpp
 * Macro system implementation (stub)
 */

#include "MacroEngine.h"

MacroEngine::MacroEngine()
{
}

void MacroEngine::loadMacros()
{
    // TODO: Load macros from resources/macros/
    // For now, create some dummy macros
    m_macros["Home All"] = {"$H"};
    m_macros["Zero Work"] = {"G10 L20 P1 X0 Y0 Z0"};
    m_macros["Park"] = {"G0 X0 Y0", "G0 Z10"};
}

bool MacroEngine::executeMacro(const std::string& name)
{
    auto it = m_macros.find(name);
    if (it == m_macros.end()) {
        return false;
    }
    
    // TODO: Execute the macro commands
    // This would need access to FluidNCClient
    return true;
}

std::vector<std::string> MacroEngine::getMacroNames() const
{
    std::vector<std::string> names;
    for (const auto& pair : m_macros) {
        names.push_back(pair.first);
    }
    return names;
}
