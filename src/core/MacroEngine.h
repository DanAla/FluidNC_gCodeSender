/**
 * core/MacroEngine.h
 * Macro system with scripting support (stub)
 */

#pragma once

#include <string>
#include <vector>
#include <map>

class MacroEngine
{
public:
    MacroEngine();
    
    // Load macros from resources/macros/
    void loadMacros();
    
    // Execute a macro by name
    bool executeMacro(const std::string& name);
    
    // Get list of available macros
    std::vector<std::string> getMacroNames() const;

private:
    std::map<std::string, std::vector<std::string>> m_macros;
};
