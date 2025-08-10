/**
 * core/GCodeEditor.h
 * G-code editing and manipulation (stub)
 */

#pragma once

#include <string>
#include <vector>

struct GCodeLine {
    std::string command;
    std::string comment;
    bool enabled = true;
};

class GCodeEditor
{
public:
    GCodeEditor();
    
    // Load G-code from string
    void loadFromString(const std::string& gcode);
    
    // Get all lines
    const std::vector<GCodeLine>& getLines() const { return m_lines; }
    
    // Modify lines
    void insertLine(size_t index, const GCodeLine& line);
    void removeLine(size_t index);
    void moveLine(size_t from, size_t to);
    
    // Export to string
    std::string toString() const;

private:
    std::vector<GCodeLine> m_lines;
};
