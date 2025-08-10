/**
 * core/GCodeGenerator.h
 * Converts SVG paths to GCode
 * Supports engraving, cutting, pocketing
 */

#pragma once

#include <string>
#include <vector>

class GCodeGenerator
{
public:
    enum class Operation {
        Engrave,
        Cut,
        Pocket
    };
    
    struct Settings {
        float feedRate = 1000.0f;
        float spindleSpeed = 10000.0f;
        float safeZ = 5.0f;
        float workZ = -1.0f;
        float depthPerPass = 0.5f;
    };
    
    GCodeGenerator();
    
    // SVG to G-code conversion (stub for now)
    std::vector<std::string> convertSVG(const std::string& svgPath, 
                                        Operation op, 
                                        const Settings& settings);

private:
    Settings m_defaultSettings;
};
