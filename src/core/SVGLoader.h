/**
 * core/SVGLoader.h
 * SVG file parsing and manipulation (stub)
 */

#pragma once

#include <string>
#include <vector>

struct SVGPath {
    std::string pathData;
    // TODO: Add path properties like stroke, fill, etc.
};

class SVGLoader
{
public:
    SVGLoader();
    
    // Load SVG from file
    bool loadFromFile(const std::string& filename);
    
    // Get parsed paths
    const std::vector<SVGPath>& getPaths() const { return m_paths; }

private:
    std::vector<SVGPath> m_paths;
};
