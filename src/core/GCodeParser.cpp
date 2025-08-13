/**
 * core/GCodeParser.cpp
 * Comprehensive G-code parser implementation with proper state machine
 */

#include "GCodeParser.h"
#include "SimpleLogger.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cmath>
#include <cctype>

// Static member initialization
std::map<int, CommandType> GCodeParser::s_gcodeLookup;
std::map<int, CommandType> GCodeParser::s_mcodeLookup;
bool GCodeParser::s_tablesInitialized = false;

// Constants
constexpr double EPSILON = 1e-6;
constexpr double PI = 3.14159265359;

// State reset methods
void GCodeState::reset() {
    currentPosition = Position();
    workOffset = Position();
    motionMode = CommandType::RAPID_MOVE;
    units = Units::MILLIMETERS;
    coordinateSystem = CoordinateSystem::G54;
    plane = Plane::XY;
    positionMode = MotionMode::ABSOLUTE;
    feedRateMode = FeedRateMode::UNITS_PER_MINUTE;
    spindleState = SpindleState::OFF;
    coolantState = CoolantState();
    currentTool = 0;
    feedRate = 0.0;
    spindleSpeed = 0.0;
    dwellTime = 0.0;
    retractHeight = 0.0;
    cycleDepth = 0.0;
    peckIncrement = 0.0;
    programRunning = true;
    lineNumber = 0;
}

Position GCodeState::getAbsolutePosition(const Position& pos) const {
    Position result = pos;
    if (pos.hasX) result.x += workOffset.x;
    if (pos.hasY) result.y += workOffset.y;
    if (pos.hasZ) result.z += workOffset.z;
    if (pos.hasA) result.a += workOffset.a;
    if (pos.hasB) result.b += workOffset.b;
    if (pos.hasC) result.c += workOffset.c;
    return result;
}

void GCodeStatistics::reset() {
    totalLines = commandLines = commentLines = errorLines = 0;
    rapidMoves = linearMoves = arcMoves = toolChanges = 0;
    totalDistance = rapidDistance = cuttingDistance = estimatedTime = 0.0;
    minBounds = maxBounds = Position();
    boundsValid = false;
    toolsUsed.clear();
    feedRates.clear();
    spindleSpeeds.clear();
}

// Constructor/Destructor
GCodeParser::GCodeParser() {
    initializeLookupTables();
    resetState();
}

GCodeParser::~GCodeParser() = default;

void GCodeParser::initializeLookupTables() {
    if (s_tablesInitialized) return;
    
    // G-code lookup table
    s_gcodeLookup[0] = CommandType::RAPID_MOVE;
    s_gcodeLookup[1] = CommandType::LINEAR_MOVE;
    s_gcodeLookup[2] = CommandType::CW_ARC;
    s_gcodeLookup[3] = CommandType::CCW_ARC;
    s_gcodeLookup[4] = CommandType::DWELL;
    s_gcodeLookup[17] = CommandType::PLANE_XY;
    s_gcodeLookup[18] = CommandType::PLANE_XZ;
    s_gcodeLookup[19] = CommandType::PLANE_YZ;
    s_gcodeLookup[20] = CommandType::INCHES;
    s_gcodeLookup[21] = CommandType::MILLIMETERS;
    s_gcodeLookup[28] = CommandType::RETURN_HOME;
    s_gcodeLookup[30] = CommandType::RETURN_PREDEFINED;
    s_gcodeLookup[54] = CommandType::WORK_COORD_1;
    s_gcodeLookup[55] = CommandType::WORK_COORD_2;
    s_gcodeLookup[56] = CommandType::WORK_COORD_3;
    s_gcodeLookup[57] = CommandType::WORK_COORD_4;
    s_gcodeLookup[58] = CommandType::WORK_COORD_5;
    s_gcodeLookup[59] = CommandType::WORK_COORD_6;
    s_gcodeLookup[80] = CommandType::CANCEL_CYCLE;
    s_gcodeLookup[81] = CommandType::CANNED_CYCLE_DRILL;
    s_gcodeLookup[82] = CommandType::CANNED_CYCLE_DWELL;
    s_gcodeLookup[83] = CommandType::CANNED_CYCLE_PECK;
    s_gcodeLookup[84] = CommandType::CANNED_CYCLE_TAP;
    s_gcodeLookup[85] = CommandType::CANNED_CYCLE_BORE;
    s_gcodeLookup[90] = CommandType::ABSOLUTE_MODE;
    s_gcodeLookup[91] = CommandType::INCREMENTAL_MODE;
    s_gcodeLookup[92] = CommandType::COORDINATE_OFFSET;
    
    // M-code lookup table
    s_mcodeLookup[0] = CommandType::PROGRAM_STOP;
    s_mcodeLookup[1] = CommandType::OPTIONAL_STOP;
    s_mcodeLookup[2] = CommandType::PROGRAM_END;
    s_mcodeLookup[3] = CommandType::SPINDLE_CW;
    s_mcodeLookup[4] = CommandType::SPINDLE_CCW;
    s_mcodeLookup[5] = CommandType::SPINDLE_STOP;
    s_mcodeLookup[6] = CommandType::TOOL_CHANGE;
    s_mcodeLookup[7] = CommandType::COOLANT_MIST;
    s_mcodeLookup[8] = CommandType::COOLANT_FLOOD;
    s_mcodeLookup[9] = CommandType::COOLANT_OFF;
    s_mcodeLookup[30] = CommandType::PROGRAM_END;
    
    s_tablesInitialized = true;
}

// Main parsing methods
bool GCodeParser::parseFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        reportError("Cannot open file: " + filename, 0, ParseError::FATAL);
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    return parseString(content);
}

bool GCodeParser::parseString(const std::string& gcode) {
    resetState();
    
    std::istringstream stream(gcode);
    std::string line;
    int lineNumber = 0;
    int totalLines = std::count(gcode.begin(), gcode.end(), '\n') + 1;
    
    while (std::getline(stream, line) && m_errors.size() < m_maxErrors) {
        lineNumber++;
        m_state.lineNumber = lineNumber;
        
        if (m_progressCallback) {
            m_progressCallback(lineNumber, totalLines);
        }
        
        ParsedLine parsed = parseLine(line, lineNumber);
        
        m_statistics.totalLines++;
        if (parsed.hasError) {
            m_statistics.errorLines++;
        } else if (!parsed.commands.empty()) {
            m_statistics.commandLines++;
        } else if (!parsed.comment.empty()) {
            m_statistics.commentLines++;
        }
        
        // Process each command in the line
        for (const auto& command : parsed.commands) {
            processCommand(command);
        }
    }
    
    return m_errors.empty() || (!m_strictMode && m_statistics.errorLines == 0);
}

ParsedLine GCodeParser::parseLine(const std::string& line, int lineNumber) {
    ParsedLine result;
    result.originalLine = line;
    result.lineNumber = lineNumber;
    
    // Extract comment first
    result.comment = extractComment(line);
    
    // Clean the line (remove comments, whitespace)
    std::string cleanedLine = cleanLine(line);
    
    if (cleanedLine.empty()) {
        return result; // Empty line or comment-only line
    }
    
    try {
        // Parse line into tokens
        std::regex tokenRegex(R"(([GMSTFPXYZIJKRABCUVWDEFHLNQR])([+-]?\d*\.?\d*))");
        std::sregex_iterator iter(cleanedLine.begin(), cleanedLine.end(), tokenRegex);
        std::sregex_iterator end;
        
        std::map<char, double> parameters;
        std::vector<int> gcodes, mcodes;
        
        // Extract all tokens
        for (; iter != end; ++iter) {
            const std::smatch& match = *iter;
            char letter = std::toupper(match[1].str()[0]);
            std::string valueStr = match[2].str();
            
            if (valueStr.empty()) {
                reportError("Missing value for " + std::string(1, letter), lineNumber);
                result.hasError = true;
                continue;
            }
            
            double value = std::stod(valueStr);
            
            if (letter == 'G') {
                gcodes.push_back(static_cast<int>(value));
            } else if (letter == 'M') {
                mcodes.push_back(static_cast<int>(value));
            } else {
                parameters[letter] = value;
            }
        }
        
        // Create commands for G-codes
        for (int gcode : gcodes) {
            GCodeCommand command;
            command.lineNumber = lineNumber;
            command.originalLine = line;
            command.comment = result.comment;
            
            if (parseGCode(gcode, command)) {
                parseParameters(cleanedLine, command);
                result.commands.push_back(command);
            }
        }
        
        // Create commands for M-codes
        for (int mcode : mcodes) {
            GCodeCommand command;
            command.lineNumber = lineNumber;
            command.originalLine = line;
            command.comment = result.comment;
            
            if (parseMCode(mcode, command)) {
                parseParameters(cleanedLine, command);
                result.commands.push_back(command);
            }
        }
        
        // If no G/M codes, but parameters exist, create motion command
        if (gcodes.empty() && mcodes.empty() && !parameters.empty()) {
            bool hasMovement = parameters.count('X') || parameters.count('Y') || 
                              parameters.count('Z') || parameters.count('A') || 
                              parameters.count('B') || parameters.count('C');
            
            if (hasMovement) {
                GCodeCommand command;
                command.type = m_state.motionMode; // Use current modal motion mode
                command.lineNumber = lineNumber;
                command.originalLine = line;
                command.comment = result.comment;
                
                parseParameters(cleanedLine, command);
                result.commands.push_back(command);
            }
        }
        
    } catch (const std::exception& e) {
        reportError("Parse error: " + std::string(e.what()), lineNumber);
        result.hasError = true;
        result.errorMessage = e.what();
    }
    
    return result;
}

bool GCodeParser::parseGCode(int gcode, GCodeCommand& command) {
    auto it = s_gcodeLookup.find(gcode);
    if (it != s_gcodeLookup.end()) {
        command.type = it->second;
        return true;
    }
    
    reportError("Unknown G-code: G" + std::to_string(gcode), command.lineNumber);
    command.type = CommandType::UNKNOWN;
    return !m_strictMode;
}

bool GCodeParser::parseMCode(int mcode, GCodeCommand& command) {
    auto it = s_mcodeLookup.find(mcode);
    if (it != s_mcodeLookup.end()) {
        command.type = it->second;
        return true;
    }
    
    reportError("Unknown M-code: M" + std::to_string(mcode), command.lineNumber);
    command.type = CommandType::UNKNOWN;
    return !m_strictMode;
}

bool GCodeParser::parseParameters(const std::string& line, GCodeCommand& command) {
    std::regex paramRegex(R"(([XYZABCIJKRFSTPQUVWDEFHLN])([+-]?\d*\.?\d*))");
    std::sregex_iterator iter(line.begin(), line.end(), paramRegex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        const std::smatch& match = *iter;
        char letter = std::toupper(match[1].str()[0]);
        double value = std::stod(match[2].str());
        
        switch (letter) {
            case 'X': command.position.x = value; command.position.hasX = true; break;
            case 'Y': command.position.y = value; command.position.hasY = true; break;
            case 'Z': command.position.z = value; command.position.hasZ = true; break;
            case 'A': command.position.a = value; command.position.hasA = true; break;
            case 'B': command.position.b = value; command.position.hasB = true; break;
            case 'C': command.position.c = value; command.position.hasC = true; break;
            case 'I': command.arc.i = value; command.arc.hasI = true; break;
            case 'J': command.arc.j = value; command.arc.hasJ = true; break;
            case 'K': command.arc.k = value; command.arc.hasK = true; break;
            case 'R': 
                if (command.type == CommandType::CW_ARC || command.type == CommandType::CCW_ARC) {
                    command.arc.r = value; 
                    command.arc.hasR = true; 
                } else {
                    command.retractHeight = value;
                }
                break;
            case 'F': command.feedRate = value; break;
            case 'S': command.spindleSpeed = value; break;
            case 'T': command.toolNumber = static_cast<int>(value); break;
            case 'P': command.dwellTime = value; break;
            case 'Q': command.peckIncrement = value; break;
        }
    }
    
    return true;
}

std::string GCodeParser::extractComment(const std::string& line) {
    std::string comment;
    
    // Extract semicolon comments
    size_t semicolon = line.find(';');
    if (semicolon != std::string::npos) {
        comment = line.substr(semicolon + 1);
    }
    
    // Extract parentheses comments
    size_t openParen = line.find('(');
    size_t closeParen = line.find(')', openParen);
    if (openParen != std::string::npos && closeParen != std::string::npos) {
        std::string parenComment = line.substr(openParen + 1, closeParen - openParen - 1);
        if (!comment.empty()) comment += " ";
        comment += parenComment;
    }
    
    // Trim whitespace
    comment.erase(0, comment.find_first_not_of(" \t"));
    comment.erase(comment.find_last_not_of(" \t") + 1);
    
    return comment;
}

std::string GCodeParser::cleanLine(const std::string& line) {
    std::string cleaned = line;
    
    // Remove comments
    size_t semicolon = cleaned.find(';');
    if (semicolon != std::string::npos) {
        cleaned = cleaned.substr(0, semicolon);
    }
    
    size_t openParen = cleaned.find('(');
    while (openParen != std::string::npos) {
        size_t closeParen = cleaned.find(')', openParen);
        if (closeParen != std::string::npos) {
            cleaned.erase(openParen, closeParen - openParen + 1);
        } else {
            cleaned.erase(openParen);
        }
        openParen = cleaned.find('(', openParen);
    }
    
    // Convert to uppercase and remove extra whitespace
    std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(), ::toupper);
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(), ::isspace), cleaned.end());
    
    return cleaned;
}

// Command processing
void GCodeParser::processCommand(const GCodeCommand& command) {
    updateModalState(command);
    
    if (m_generateToolpath) {
        generateToolpathSegment(command);
    }
    
    if (m_calculateStatistics) {
        updateStatistics(command);
    }
    
    std::string error;
    if (!validateCommand(command, error)) {
        reportError(error, command.lineNumber);
    }
}

void GCodeParser::updateModalState(const GCodeCommand& command) {
    switch (command.type) {
        case CommandType::RAPID_MOVE:
        case CommandType::LINEAR_MOVE:
        case CommandType::CW_ARC:
        case CommandType::CCW_ARC:
            m_state.motionMode = command.type;
            break;
            
        case CommandType::ABSOLUTE_MODE:
            m_state.positionMode = MotionMode::ABSOLUTE;
            break;
            
        case CommandType::INCREMENTAL_MODE:
            m_state.positionMode = MotionMode::INCREMENTAL;
            break;
            
        case CommandType::MILLIMETERS:
            m_state.units = Units::MILLIMETERS;
            break;
            
        case CommandType::INCHES:
            m_state.units = Units::INCHES;
            break;
            
        case CommandType::PLANE_XY:
            m_state.plane = Plane::XY;
            break;
            
        case CommandType::PLANE_XZ:
            m_state.plane = Plane::XZ;
            break;
            
        case CommandType::PLANE_YZ:
            m_state.plane = Plane::YZ;
            break;
            
        case CommandType::WORK_COORD_1:
            m_state.coordinateSystem = CoordinateSystem::G54;
            break;
            
        case CommandType::WORK_COORD_2:
            m_state.coordinateSystem = CoordinateSystem::G55;
            break;
            
        case CommandType::WORK_COORD_3:
            m_state.coordinateSystem = CoordinateSystem::G56;
            break;
            
        case CommandType::WORK_COORD_4:
            m_state.coordinateSystem = CoordinateSystem::G57;
            break;
            
        case CommandType::WORK_COORD_5:
            m_state.coordinateSystem = CoordinateSystem::G58;
            break;
            
        case CommandType::WORK_COORD_6:
            m_state.coordinateSystem = CoordinateSystem::G59;
            break;
            
        case CommandType::SPINDLE_CW:
            m_state.spindleState = SpindleState::CW;
            break;
            
        case CommandType::SPINDLE_CCW:
            m_state.spindleState = SpindleState::CCW;
            break;
            
        case CommandType::SPINDLE_STOP:
            m_state.spindleState = SpindleState::OFF;
            break;
            
        case CommandType::COOLANT_MIST:
            m_state.coolantState.mist = true;
            break;
            
        case CommandType::COOLANT_FLOOD:
            m_state.coolantState.flood = true;
            break;
            
        case CommandType::COOLANT_OFF:
            m_state.coolantState.mist = false;
            m_state.coolantState.flood = false;
            break;
            
        case CommandType::TOOL_CHANGE:
            if (command.toolNumber >= 0) {
                m_state.currentTool = command.toolNumber;
            }
            break;
            
        case CommandType::COORDINATE_OFFSET:
            // G92 - Set coordinate system offset
            if (command.position.hasX) m_state.workOffset.x = m_state.currentPosition.x - command.position.x;
            if (command.position.hasY) m_state.workOffset.y = m_state.currentPosition.y - command.position.y;
            if (command.position.hasZ) m_state.workOffset.z = m_state.currentPosition.z - command.position.z;
            break;
            
        case CommandType::PROGRAM_END:
        case CommandType::PROGRAM_STOP:
            m_state.programRunning = false;
            break;
            
        default:
            break;
    }
    
    // Update modal values
    if (command.feedRate >= 0) {
        m_state.feedRate = command.feedRate;
    }
    if (command.spindleSpeed >= 0) {
        m_state.spindleSpeed = command.spindleSpeed;
    }
    if (command.dwellTime >= 0) {
        m_state.dwellTime = command.dwellTime;
    }
    
    // Update position for motion commands
    if (isMotionCommand(command.type) || command.position.hasX || command.position.hasY || command.position.hasZ) {
        Position newPos = m_state.currentPosition;
        
        if (m_state.positionMode == MotionMode::ABSOLUTE) {
            if (command.position.hasX) newPos.x = command.position.x;
            if (command.position.hasY) newPos.y = command.position.y;
            if (command.position.hasZ) newPos.z = command.position.z;
            if (command.position.hasA) newPos.a = command.position.a;
            if (command.position.hasB) newPos.b = command.position.b;
            if (command.position.hasC) newPos.c = command.position.c;
        } else {
            // Incremental mode
            if (command.position.hasX) newPos.x += command.position.x;
            if (command.position.hasY) newPos.y += command.position.y;
            if (command.position.hasZ) newPos.z += command.position.z;
            if (command.position.hasA) newPos.a += command.position.a;
            if (command.position.hasB) newPos.b += command.position.b;
            if (command.position.hasC) newPos.c += command.position.c;
        }
        
        m_state.currentPosition = newPos;
    }
}

void GCodeParser::generateToolpathSegment(const GCodeCommand& command) {
    if (!isMotionCommand(command.type)) {
        return;
    }
    
    ToolpathSegment segment;
    segment.start = m_state.currentPosition;
    segment.feedRate = m_state.feedRate;
    segment.spindleSpeed = m_state.spindleSpeed;
    segment.spindleOn = (m_state.spindleState != SpindleState::OFF);
    segment.coolantOn = (m_state.coolantState.mist || m_state.coolantState.flood);
    segment.toolNumber = m_state.currentTool;
    
    Position targetPos = m_state.currentPosition;
    if (m_state.positionMode == MotionMode::ABSOLUTE) {
        if (command.position.hasX) targetPos.x = command.position.x;
        if (command.position.hasY) targetPos.y = command.position.y;
        if (command.position.hasZ) targetPos.z = command.position.z;
    } else {
        if (command.position.hasX) targetPos.x += command.position.x;
        if (command.position.hasY) targetPos.y += command.position.y;
        if (command.position.hasZ) targetPos.z += command.position.z;
    }
    
    segment.end = targetPos;
    
    switch (command.type) {
        case CommandType::RAPID_MOVE:
            segment.type = ToolpathSegment::RAPID;
            break;
            
        case CommandType::LINEAR_MOVE:
            segment.type = ToolpathSegment::LINEAR;
            break;
            
        case CommandType::CW_ARC:
            segment.type = ToolpathSegment::ARC_CW;
            calculateArcCenter(command, segment.center, segment.radius);
            break;
            
        case CommandType::CCW_ARC:
            segment.type = ToolpathSegment::ARC_CCW;
            calculateArcCenter(command, segment.center, segment.radius);
            break;
            
        default:
            return; // Not a motion command
    }
    
    // Calculate segment length
    if (segment.type == ToolpathSegment::RAPID || segment.type == ToolpathSegment::LINEAR) {
        double dx = segment.end.x - segment.start.x;
        double dy = segment.end.y - segment.start.y;
        double dz = segment.end.z - segment.start.z;
        segment.length = std::sqrt(dx*dx + dy*dy + dz*dz);
    } else if (segment.type == ToolpathSegment::ARC_CW || segment.type == ToolpathSegment::ARC_CCW) {
        // Calculate arc length (simplified, assumes 2D arc)
        double startAngle = std::atan2(segment.start.y - segment.center.y, segment.start.x - segment.center.x);
        double endAngle = std::atan2(segment.end.y - segment.center.y, segment.end.x - segment.center.x);
        double angleSpan = std::abs(endAngle - startAngle);
        if (angleSpan > PI) angleSpan = 2*PI - angleSpan;
        segment.length = segment.radius * angleSpan;
    }
    
    // Calculate estimated time
    if (segment.feedRate > 0 && segment.type != ToolpathSegment::RAPID) {
        segment.estimatedTime = (segment.length / segment.feedRate) * 60.0; // Convert to seconds
    } else {
        // Assume rapid rate of 10000 mm/min for time estimation
        segment.estimatedTime = (segment.length / 10000.0) * 60.0;
    }
    
    m_toolpath.push_back(segment);
    
    if (m_segmentCallback) {
        m_segmentCallback(segment);
    }
}

void GCodeParser::calculateArcCenter(const GCodeCommand& command, Position& center, double& radius) {
    // Calculate arc center and radius
    if (command.arc.hasR) {
        // Radius format (R parameter)
        radius = std::abs(command.arc.r);
        // Calculate center from radius and endpoints
        // This is simplified - full implementation would handle all cases
        center.x = (m_state.currentPosition.x + command.position.x) / 2.0;
        center.y = (m_state.currentPosition.y + command.position.y) / 2.0;
        center.z = m_state.currentPosition.z;
    } else if (command.arc.hasI || command.arc.hasJ) {
        // Center format (I, J parameters)
        center.x = m_state.currentPosition.x + command.arc.i;
        center.y = m_state.currentPosition.y + command.arc.j;
        center.z = m_state.currentPosition.z + (command.arc.hasK ? command.arc.k : 0.0);
        
        double dx = m_state.currentPosition.x - center.x;
        double dy = m_state.currentPosition.y - center.y;
        radius = std::sqrt(dx*dx + dy*dy);
    }
}

void GCodeParser::updateStatistics(const GCodeCommand& command) {
    switch (command.type) {
        case CommandType::RAPID_MOVE:
            m_statistics.rapidMoves++;
            break;
        case CommandType::LINEAR_MOVE:
            m_statistics.linearMoves++;
            break;
        case CommandType::CW_ARC:
        case CommandType::CCW_ARC:
            m_statistics.arcMoves++;
            break;
        case CommandType::TOOL_CHANGE:
            m_statistics.toolChanges++;
            if (command.toolNumber >= 0) {
                m_statistics.toolsUsed.insert(command.toolNumber);
            }
            break;
        default:
            break;
    }
    
    // Update bounds
    if (command.position.hasX || command.position.hasY || command.position.hasZ) {
        updateBounds(m_state.currentPosition);
    }
    
    // Update feed rate statistics
    if (command.feedRate >= 0) {
        // This would be more complex in a full implementation
        m_statistics.feedRates[command.feedRate] = 0.0;
    }
}

void GCodeParser::updateBounds(const Position& pos) {
    if (!m_statistics.boundsValid) {
        m_statistics.minBounds = m_statistics.maxBounds = pos;
        m_statistics.boundsValid = true;
    } else {
        m_statistics.minBounds.x = std::min(m_statistics.minBounds.x, pos.x);
        m_statistics.minBounds.y = std::min(m_statistics.minBounds.y, pos.y);
        m_statistics.minBounds.z = std::min(m_statistics.minBounds.z, pos.z);
        m_statistics.maxBounds.x = std::max(m_statistics.maxBounds.x, pos.x);
        m_statistics.maxBounds.y = std::max(m_statistics.maxBounds.y, pos.y);
        m_statistics.maxBounds.z = std::max(m_statistics.maxBounds.z, pos.z);
    }
}

bool GCodeParser::validateCommand(const GCodeCommand& command, std::string& error) {
    // Basic validation
    switch (command.type) {
        case CommandType::CW_ARC:
        case CommandType::CCW_ARC:
            if (!command.arc.hasI && !command.arc.hasJ && !command.arc.hasR) {
                error = "Arc command missing I/J or R parameter";
                return false;
            }
            if (command.arc.hasR && (command.arc.hasI || command.arc.hasJ)) {
                error = "Arc command cannot have both R and I/J parameters";
                return false;
            }
            break;
            
        case CommandType::DWELL:
            if (command.dwellTime < 0) {
                error = "Dwell command missing P parameter";
                return false;
            }
            break;
            
        default:
            break;
    }
    
    return true;
}

void GCodeParser::reportError(const std::string& message, int lineNumber, ParseError::Severity severity) {
    ParseError error;
    error.lineNumber = lineNumber;
    error.message = message;
    error.severity = severity;
    
    m_errors.push_back(error);
    
    if (m_errorCallback) {
        m_errorCallback(error);
    }
    
    LOG_ERROR(wxString::Format("G-code parse error at line %d: %s", lineNumber, message).ToStdString());
}

void GCodeParser::resetState() {
    m_state.reset();
    m_statistics.reset();
    m_toolpath.clear();
    m_errors.clear();
}

// Utility methods
std::string GCodeParser::commandTypeToString(CommandType type) {
    switch (type) {
        case CommandType::RAPID_MOVE: return "G0 (Rapid Move)";
        case CommandType::LINEAR_MOVE: return "G1 (Linear Move)";
        case CommandType::CW_ARC: return "G2 (Clockwise Arc)";
        case CommandType::CCW_ARC: return "G3 (Counter-clockwise Arc)";
        case CommandType::DWELL: return "G4 (Dwell)";
        case CommandType::PLANE_XY: return "G17 (XY Plane)";
        case CommandType::PLANE_XZ: return "G18 (XZ Plane)";
        case CommandType::PLANE_YZ: return "G19 (YZ Plane)";
        case CommandType::INCHES: return "G20 (Inches)";
        case CommandType::MILLIMETERS: return "G21 (Millimeters)";
        case CommandType::RETURN_HOME: return "G28 (Return Home)";
        case CommandType::ABSOLUTE_MODE: return "G90 (Absolute Mode)";
        case CommandType::INCREMENTAL_MODE: return "G91 (Incremental Mode)";
        case CommandType::COORDINATE_OFFSET: return "G92 (Coordinate Offset)";
        case CommandType::SPINDLE_CW: return "M3 (Spindle CW)";
        case CommandType::SPINDLE_CCW: return "M4 (Spindle CCW)";
        case CommandType::SPINDLE_STOP: return "M5 (Spindle Stop)";
        case CommandType::TOOL_CHANGE: return "M6 (Tool Change)";
        case CommandType::COOLANT_MIST: return "M7 (Coolant Mist)";
        case CommandType::COOLANT_FLOOD: return "M8 (Coolant Flood)";
        case CommandType::COOLANT_OFF: return "M9 (Coolant Off)";
        case CommandType::PROGRAM_STOP: return "M0 (Program Stop)";
        case CommandType::PROGRAM_END: return "M2/M30 (Program End)";
        default: return "Unknown";
    }
}

bool GCodeParser::isModalCommand(CommandType type) {
    switch (type) {
        case CommandType::RAPID_MOVE:
        case CommandType::LINEAR_MOVE:
        case CommandType::CW_ARC:
        case CommandType::CCW_ARC:
        case CommandType::PLANE_XY:
        case CommandType::PLANE_XZ:
        case CommandType::PLANE_YZ:
        case CommandType::INCHES:
        case CommandType::MILLIMETERS:
        case CommandType::ABSOLUTE_MODE:
        case CommandType::INCREMENTAL_MODE:
        case CommandType::WORK_COORD_1:
        case CommandType::WORK_COORD_2:
        case CommandType::WORK_COORD_3:
        case CommandType::WORK_COORD_4:
        case CommandType::WORK_COORD_5:
        case CommandType::WORK_COORD_6:
            return true;
        default:
            return false;
    }
}

bool GCodeParser::isMotionCommand(CommandType type) {
    switch (type) {
        case CommandType::RAPID_MOVE:
        case CommandType::LINEAR_MOVE:
        case CommandType::CW_ARC:
        case CommandType::CCW_ARC:
        case CommandType::CANNED_CYCLE_DRILL:
        case CommandType::CANNED_CYCLE_DWELL:
        case CommandType::CANNED_CYCLE_PECK:
        case CommandType::CANNED_CYCLE_TAP:
        case CommandType::CANNED_CYCLE_BORE:
            return true;
        default:
            return false;
    }
}

Position GCodeParser::transformToWorkCoordinates(const Position& machinePos) const {
    Position workPos = machinePos;
    workPos.x -= m_state.workOffset.x;
    workPos.y -= m_state.workOffset.y;
    workPos.z -= m_state.workOffset.z;
    workPos.a -= m_state.workOffset.a;
    workPos.b -= m_state.workOffset.b;
    workPos.c -= m_state.workOffset.c;
    return workPos;
}

Position GCodeParser::transformToMachineCoordinates(const Position& workPos) const {
    Position machinePos = workPos;
    machinePos.x += m_state.workOffset.x;
    machinePos.y += m_state.workOffset.y;
    machinePos.z += m_state.workOffset.z;
    machinePos.a += m_state.workOffset.a;
    machinePos.b += m_state.workOffset.b;
    machinePos.c += m_state.workOffset.c;
    return machinePos;
}
