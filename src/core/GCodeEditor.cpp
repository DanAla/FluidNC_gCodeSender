/**
 * core/GCodeEditor.cpp
 * G-code editing implementation (stub)
 */

#include "GCodeEditor.h"
#include <sstream>

GCodeEditor::GCodeEditor()
{
}

void GCodeEditor::loadFromString(const std::string& gcode)
{
    m_lines.clear();
    std::istringstream iss(gcode);
    std::string line;
    
    while (std::getline(iss, line)) {
        GCodeLine gcodeLine;
        
        // Simple parsing - find comment
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            gcodeLine.command = line.substr(0, commentPos);
            gcodeLine.comment = line.substr(commentPos);
        } else {
            gcodeLine.command = line;
        }
        
        m_lines.push_back(gcodeLine);
    }
}

void GCodeEditor::insertLine(size_t index, const GCodeLine& line)
{
    if (index <= m_lines.size()) {
        m_lines.insert(m_lines.begin() + index, line);
    }
}

void GCodeEditor::removeLine(size_t index)
{
    if (index < m_lines.size()) {
        m_lines.erase(m_lines.begin() + index);
    }
}

void GCodeEditor::moveLine(size_t from, size_t to)
{
    if (from < m_lines.size() && to <= m_lines.size() && from != to) {
        GCodeLine line = m_lines[from];
        m_lines.erase(m_lines.begin() + from);
        if (to > from) to--; // Adjust for removed element
        m_lines.insert(m_lines.begin() + to, line);
    }
}

std::string GCodeEditor::toString() const
{
    std::ostringstream oss;
    for (const auto& line : m_lines) {
        if (line.enabled) {
            oss << line.command;
            if (!line.comment.empty()) {
                oss << " " << line.comment;
            }
            oss << "\n";
        }
    }
    return oss.str();
}
